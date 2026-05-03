// =============================================================================
//  wifi_watchdog.cpp — تطبيق مهمة مراقبة الشبكة
//  ────────────────────────────────────────────────
//  هذا الملف موجود في:  src/wifi_watchdog.cpp
//
//  لماذا هذه المهمة ضرورية؟
//  ────────────────────────
//  في النسخة القديمة (super-loop) ما في أي كود يتعامل مع انقطاع WiFi.
//  لو انقطع الاتصال:
//    • Firebase يفشل بصمت (يطبع error في Serial)
//    • Google Sheets يطبع "WiFi Disconnected!" ويتوقف
//    • Telegram لا يصل أي تنبيه
//    • الجهاز لا يعيد الاتصال → مستحيل يشتغل مرة ثانية بدون reset يدوي
//
//  هذه المهمة تحل كل هذا:
//    1. تراقب WiFi كل 5 ثواني
//    2. لو انقطع → تنتظر حتى لا توجد HTTP نشطة (xWiFiMutex) ثم تعيد الاتصال
//    3. تنتظر NTP ثم تعلم بقية المهام إن الوقت جاهز (BIT_NTP_SYNCED)
//    4. ترفع/تخفض BIT_WIFI_CONNECTED → المهام الأخرى تقرأ هذا الـ bit
//       قبل ما تحاول أي اتصال بدل ما تكتشف الخطأ في منتصف SSL handshake
// =============================================================================

#include "wifi_watchdog.h"

// =============================================================================
//  ثوابت داخلية — لا تُصدَّر للخارج
// =============================================================================

// كم ثانية ننتظر قبل نحاول reconnect بعد كل فشل
// نبدأ بـ 5 ثواني ونضاعف مع كل فشل (Exponential Backoff) حتى MAX
static const uint32_t RECONNECT_BASE_DELAY_MS  = 5000;
static const uint32_t RECONNECT_MAX_DELAY_MS   = 60000; // دقيقة كحد أقصى

// كم مرة نحاول NTP قبل نستسلم ونجرب مرة ثانية لاحقاً
static const uint8_t  NTP_MAX_RETRIES          = 20;
static const uint32_t NTP_RETRY_INTERVAL_MS    = 1000;

// كم ثانية بين كل دورة مراقبة (polling interval)
static const uint32_t WATCHDOG_POLL_INTERVAL_MS = 5000;


// =============================================================================
//  دوال مساعدة داخلية (static = مخفية عن بقية الملفات)
// =============================================================================

// ── waitForNTP() ──────────────────────────────────────────────────────────────
//  تحاول تتزامن مع NTP وترفع BIT_NTP_SYNCED لو نجحت
//  لو فشلت بعد MAX_RETRIES محاولة → تعود false لتجرب لاحقاً
//
//  لماذا هنا بدل setup()؟
//  ─────────────────────────
//  في setup() القديم: while(!getLocalTime()) يحجب كل شيء إلى الأبد
//  هنا: نحاول NTP_MAX_RETRIES مرة فقط، لو فشلنا نكمل الشغل
//  vTaskHistoryLogger لن تكتب timestamps حتى يرتفع BIT_NTP_SYNCED
//  بقية المهام (Firebase real-time، Telegram) لا تحتاج الوقت → تشتغل عادي
// =============================================================================
static bool waitForNTP() {
    struct tm timeinfo;
    uint8_t attempts = 0;

    Serial.print("[Watchdog] Waiting for NTP sync");

    while (attempts < NTP_MAX_RETRIES) {
        if (getLocalTime(&timeinfo)) {
            // ✅ الوقت متزامن — أعلم كل المهام
            xEventGroupSetBits(xSystemEventGroup, BIT_NTP_SYNCED);

            char timeBuf[30];
            strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", &timeinfo);
            Serial.println("\n[Watchdog] NTP synced: " + String(timeBuf));
            return true;
        }
        Serial.print(".");
        attempts++;
        vTaskDelay(pdMS_TO_TICKS(NTP_RETRY_INTERVAL_MS));
    }

    Serial.println("\n[Watchdog] NTP sync failed after retries. Will retry later.");
    return false;
}


// ── attemptReconnect() ───────────────────────────────────────────────────────
//  تحاول إعادة الاتصال بالـ WiFi بشكل آمن
//
//  لماذا نأخذ xWiFiMutex قبل الـ disconnect؟
//  ─────────────────────────────────────────────
//  المشكلة: لو Firebase أو Telegram في منتصف SSL handshake
//  ونحن نستدعي WiFi.disconnect() → crash مضمون (socket يُغلق تحت الكود)
//
//  الحل: نطلب xWiFiMutex أولاً
//  • لو المهام الأخرى تستخدم WiFi → ننتظر حتى ينتهوا (max 5 ثواني)
//  • لو انتهوا → نأخذ المutex ونعمل reconnect بأمان
//  • المهام الأخرى لن تقدر تبدأ اتصال جديد لأننا نمسك المutex
//
//  ترجع: true لو اتصل بنجاح، false لو فشل
// =============================================================================
static bool attemptReconnect(uint32_t& retryDelay) {

    Serial.println("[Watchdog] WiFi lost! Attempting reconnect...");

    // أخبر كل المهام إن WiFi انقطع — لن تحاول أي HTTP جديد
    xEventGroupClearBits(xSystemEventGroup, BIT_WIFI_CONNECTED);

    // انتظر المutex بأمان — الـ timeout يمنعنا من الانتظار إلى الأبد
    // لو ما أخذناه في 5 ثواني → نكمل على أي حال (الـ disconnect أهم)
    bool mutexTaken = (xSemaphoreTake(xWiFiMutex, pdMS_TO_TICKS(5000)) == pdTRUE);

    if (!mutexTaken) {
        // ⚠️ لم نحصل على المutex — مهمة أخرى ربما علقت
        // نكمل الـ reconnect بدون المutex، الأسوأ: socket error في تلك المهمة
        Serial.println("[Watchdog] xWiFiMutex timeout — forcing reconnect anyway");
    }

    // قطع الاتصال القديم بشكل نظيف
    WiFi.disconnect(false); // false = لا تمسح credentials المحفوظة
    vTaskDelay(pdMS_TO_TICKS(1000)); // دع الـ lwIP يحرر الـ sockets

    // حاول الاتصال من جديد (credentials محفوظة من setup())
    WiFi.reconnect();

    // انتظر الاتصال — max 15 ثانية
    Serial.print("[Watchdog] Reconnecting");
    uint8_t wait = 0;
    while (WiFi.status() != WL_CONNECTED && wait < 15) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        Serial.print(".");
        wait++;
    }

    // حرر المutex لو أخذناه
    if (mutexTaken) {
        xSemaphoreGive(xWiFiMutex);
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[Watchdog] Reconnected! IP: " + WiFi.localIP().toString());

        // ✅ أعلم كل المهام إن WiFi عاد
        xEventGroupSetBits(xSystemEventGroup, BIT_WIFI_CONNECTED);

        // نعيد مزامنة NTP لأن الوقت ربما انجرف خلال الانقطاع
        waitForNTP();

        // reset delay لو نجحنا
        retryDelay = RECONNECT_BASE_DELAY_MS;
        return true;

    } else {
        Serial.println("\n[Watchdog] Reconnect failed.");

        // Exponential Backoff — كل فشل يضاعف وقت الانتظار
        // مثلاً: 5s → 10s → 20s → 40s → 60s (الحد الأقصى)
        // لماذا؟ لو الـ router انقطع كلياً، محاولات سريعة متكررة تُهدر طاقة
        // وتُغرق السجل بأخطاء غير مفيدة
        retryDelay = min(retryDelay * 2, RECONNECT_MAX_DELAY_MS);
        Serial.println("[Watchdog] Next attempt in " +
                       String(retryDelay / 1000) + "s");
        return false;
    }
}


// =============================================================================
//  vTaskWiFiWatchdog — المهمة الرئيسية
//  ──────────────────────────────────────
//  Core 0 | Priority 3 | Stack 3 KB
//
//  دورة حياة المهمة:
//  ──────────────────
//  [Boot] → انتظر NTP أول مرة
//         ↓
//  [Loop] → تحقق من WiFi كل 5 ثواني
//         → WiFi متصل  → ارفع BIT_WIFI_CONNECTED، نام 5 ثواني
//         → WiFi منقطع → استدعِ attemptReconnect()، نام retryDelay
//         → NTP غير متزامن → جرب مجدداً (ممكن يحدث بعد reconnect)
// =============================================================================
void vTaskWiFiWatchdog(void* pvParameters) {

    uint32_t retryDelay = RECONNECT_BASE_DELAY_MS;
    bool ntpSynced = false;

    // ── خطوة أولى: تزامن NTP عند الإطلاق ────────────────────────────────────
    //  WiFi متصل بالفعل من setup() — نحاول NTP مباشرة
    //  لو فشل → سنحاول مجدداً في كل دورة حتى ينجح
    ntpSynced = waitForNTP();

    // ── الحلقة الرئيسية ────────────────────────────────────────────────────
    for (;;) {

        if (WiFi.status() == WL_CONNECTED) {

            // WiFi متصل ✅

            // تأكد إن الـ bit مرفوع (قد يكون انخفض خطأً)
            xEventGroupSetBits(xSystemEventGroup, BIT_WIFI_CONNECTED);

            // لو NTP لم يتزامن بعد → حاول الآن
            // هذا يحدث لو waitForNTP() فشلت في الإطلاق
            if (!ntpSynced) {
                EventBits_t bits = xEventGroupGetBits(xSystemEventGroup);
                if (!(bits & BIT_NTP_SYNCED)) {
                    ntpSynced = waitForNTP();
                }
            }

            // استمر بالمراقبة — نام حتى الدورة القادمة
            vTaskDelay(pdMS_TO_TICKS(WATCHDOG_POLL_INTERVAL_MS));

        } else {

            // WiFi منقطع ❌ — حاول إعادة الاتصال
            bool reconnected = attemptReconnect(retryDelay);

            if (!reconnected) {
                // فشل الاتصال — انتظر retryDelay (Exponential Backoff)
                // نستخدم vTaskDelay بدل delay() لأننا في task context
                vTaskDelay(pdMS_TO_TICKS(retryDelay));
            } else {
                // نجح الاتصال — NTP يُجرَّب داخل attemptReconnect
                // نعيد ضبط ntpSynced بناءً على الـ event group
                EventBits_t bits = xEventGroupGetBits(xSystemEventGroup);
                ntpSynced = (bits & BIT_NTP_SYNCED) != 0;
            }
        }
    }
}
