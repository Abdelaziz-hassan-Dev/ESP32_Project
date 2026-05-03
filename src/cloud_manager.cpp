// =============================================================================
//  cloud_manager.cpp — مدير Google Sheets (نسخة RTOS)
//  ─────────────────────────────────────────────────────
//  هذا الملف موجود في:  src/cloud_manager.cpp
//
//  مقارنة سريعة بين القديم والجديد:
//  ──────────────────────────────────
//
//  القديم (super-loop):
//  ┌──────────────────────────────────────────────────────────────┐
//  │ loop() كل LOG_INTERVAL:                                      │
//  │   logDataToGoogleSheet(temp, hum, isFire)                    │
//  │     └─ if WiFi disconnected → return (بدون retry)            │
//  │     └─ http.begin() → SSL handshake (~1-3s) ← يحجب loop()   │
//  │     └─ http.GET() → ينتظر response                           │
//  │     └─ http.end()                                            │
//  │                                                              │
//  │  المشاكل:                                                    │
//  │   • SSL يحجب الحساسات والتنبيهات طول فترة الإرسال           │
//  │   • secureClient global → تعارض محتمل مع Telegram SSL       │
//  │   • لا retry لو WiFi منقطع                                   │
//  │   • millis() coupling: التوقيت مرتبط بـ loop() speed         │
//  └──────────────────────────────────────────────────────────────┘
//
//  الجديد (RTOS):
//  ┌──────────────────────────────────────────────────────────────┐
//  │ vTaskHistoryLogger تشتغل بشكل مستقل على Core 0:             │
//  │                                                              │
//  │   xEventGroupWaitBits(BIT_WIFI_CONNECTED)  ← ينتظر WiFi     │
//  │     └─ xSensorMutex   → اقرأ g_latestSensorData             │
//  │     └─ xWiFiMutex     → http.begin()..GET()..end()          │
//  │     └─ vTaskDelay(LOG_INTERVAL) ← ينام بدون أكل CPU         │
//  │                                                              │
//  │  الحلول:                                                     │
//  │   • SSL في مهمتها الخاصة = الحساسات والتنبيهات لا تتأخر     │
//  │   • xWiFiMutex = لا تعارض مع Telegram SSL (Issue #2 في الخطة)│
//  │   • BIT_WIFI_CONNECTED = retry تلقائي لو WiFi انقطع وعاد    │
//  │   • vTaskDelay = توقيت نظيف بدون millis()                    │
//  └──────────────────────────────────────────────────────────────┘
// =============================================================================

#include "cloud_manager.h"

// =============================================================================
//  المتغير الـ Global — secureClient
//  ─────────────────────────────────
//  لماذا لا يزال global؟
//  WiFiClientSecure ثقيل على الـ stack (~4 KB) — لو وضعناه داخل المهمة
//  سيُضاف على 12 KB المخصصة ويقترب من حد الـ overflow.
//  الأمان: محمي الآن بـ xWiFiMutex — لا أحد يلمسه إلا هذه المهمة.
//
//  ⚠️ ملاحظة OQ-2 من الخطة:
//  Telegram يستخدم WiFiClientSecure منفصل (في telegram_manager.cpp).
//  الاثنان محميان بنفس xWiFiMutex → لن يشتغلا في وقت واحد.
// =============================================================================
static WiFiClientSecure secureClient;


// =============================================================================
//  initCloud() — تُستدعى مرة واحدة في setup()
//  ─────────────────────────────────────────────
//  لا تتغير عن القديم — فقط تهيئة secureClient.
//  الـ setInsecure() يتجاوز التحقق من شهادة Google Apps Script
//  (مقبول في prototype — للإنتاج يجب إضافة CA certificate).
// =============================================================================
void initCloud() {
    secureClient.setInsecure();
    Serial.println("[Cloud] secureClient initialized (insecure mode).");
}


// =============================================================================
//  buildRequestURL() — دالة مساعدة داخلية
//  ─────────────────────────────────────────
//  تبني URL الطلب من بيانات الحساسات.
//  منفصلة للاختبار المستقل وسهولة التعديل لاحقاً.
//
//  مثال الناتج:
//  https://script.google.com/.../exec?temp=32.5&hum=65.2&fire=Safe
// =============================================================================
static String buildRequestURL(const SensorData_t& pkt) {
    String flameStr = pkt.flameDetected ? "FIRE!" : "Safe";
    String url      = String(G_SCRIPT_URL);

    url += "?temp=" + (isnan(pkt.temperature) ? "ERR" : String(pkt.temperature, 1));
    url += "&hum="  + (isnan(pkt.humidity)    ? "ERR" : String(pkt.humidity,    1));
    url += "&fire=" + flameStr;

    return url;
}


// =============================================================================
//  doHTTPRequest() — دالة مساعدة داخلية
//  ─────────────────────────────────────
//  تُنفّذ الطلب الفعلي إلى Google Sheets.
//
//  ⚠️ تُستدعى فقط بعد أخذ xWiFiMutex من المهمة الرئيسية.
//     لا تستدعيها مباشرة من أي مكان آخر.
//
//  لماذا HTTPC_STRICT_FOLLOW_REDIRECTS؟
//  ──────────────────────────────────────
//  Google Apps Script يعمل redirect من URL الـ deployment إلى URL آخر.
//  بدون هذا الخيار، HTTPClient يتوقف عند الـ redirect ويُرجع 302
//  بدل الـ response الحقيقي.
// =============================================================================
static bool doHTTPRequest(const String& url) {
    HTTPClient http;
    bool       success = false;

    // http.begin() تبدأ الاتصال — secureClient محمي بالـ Mutex خارجياً
    if (!http.begin(secureClient, url)) {
        Serial.println("[Cloud] http.begin() failed");
        return false;
    }

    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    int httpCode = http.GET();

    if (httpCode > 0) {
        // أي كود إيجابي = وصل الطلب للسيرفر (حتى لو 302 redirect)
        String payload = http.getString();
        Serial.printf("[Cloud] HTTP %d → %s\n", httpCode, payload.c_str());
        success = (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY);
    } else {
        // كود سالب = فشل على مستوى الشبكة (timeout، SSL error، إلخ)
        Serial.printf("[Cloud] HTTP error: %s\n",
                      http.errorToString(httpCode).c_str());
    }

    http.end();   // حرر الـ connection — مهم جداً لتجنب memory leak
    return success;
}


// =============================================================================
//  vTaskHistoryLogger — المهمة الرئيسية
//  ─────────────────────────────────────────
//  Core 0 | Priority 2 | Stack 12 KB
//
//  لماذا Priority 2 (الأدنى بين المهام)؟
//  ──────────────────────────────────────
//  Google Sheets logging غير حرجة زمنياً.
//  تأخير بضع ثوانٍ في السجل التاريخي مقبول تماماً،
//  لكن تأخير قراءة الحساسات أو تنبيه Telegram غير مقبول.
//  Priority المنخفضة تضمن إن المهام الأهم تعمل أولاً.
//
//  لماذا 12 KB Stack؟
//  ────────────────────
//  HTTPClient + WiFiClientSecure + SSL buffer ≈ 6-8 KB على الـ stack.
//  buildRequestURL() ينشئ String محلية.
//  12 KB يعطي هامش أمان كافٍ — راجع بـ uxTaskGetStackHighWaterMark()
//  أثناء Phase 5 (stress testing).
//
//  دورة حياة المهمة:
//  ──────────────────
//
//    ┌─────────────────────────────────────────────────────┐
//    │ [Boot] تأخير أولي 15 ثانية                          │
//    │   لماذا؟ نعطي Firebase و Telegram وقت للتهيئة       │
//    │   قبل ما نسحب xWiFiMutex أول مرة                    │
//    ├─────────────────────────────────────────────────────┤
//    │ [Loop]                                               │
//    │   1. انتظر BIT_WIFI_CONNECTED (blocking)            │
//    │   2. اقرأ g_latestSensorData تحت xSensorMutex       │
//    │   3. خذ xWiFiMutex (max 10 ثواني)                   │
//    │   4. بنّ الـ URL وأرسل HTTP GET                     │
//    │   5. حرر xWiFiMutex                                 │
//    │   6. نام LOG_INTERVAL ثانية                         │
//    └─────────────────────────────────────────────────────┘
//
//  لماذا vTaskDelay(LOG_INTERVAL) بدل queue؟
//  ────────────────────────────────────────────
//  firebase_manager يقرأ من xHistoryQueue (trigger لـ Firebase /history).
//  لو cloud_manager قرأ منه أيضاً → race condition: كل trigger يُؤخذ
//  من قِبَل مهمة واحدة فقط — إما Firebase أو Sheets، ليس الاثنان.
//  الحل: مهمة Google Sheets تتحكم في وقتها بنفسها عبر vTaskDelay.
//  النتيجة: صفر تعارض، بدون تعقيد إضافي، بدون تعديل على ملفات أخرى.
// =============================================================================
void vTaskHistoryLogger(void* pvParameters) {

    // ── تأخير أولي ───────────────────────────────────────────────────────────
    //  ننتظر 15 ثانية بعد الإقلاع قبل أول إرسال.
    //  السبب: Firebase تحتاج وقتاً لإنهاء signUp والـ token refresh،
    //  و Telegram يحتاج setup الـ SSL.
    //  لو أرسلنا مباشرة قد نسحب xWiFiMutex ونحجب هذه التهيئات.
    Serial.println("[Cloud] Task started. Initial delay 15s...");
    vTaskDelay(pdMS_TO_TICKS(15000));
    Serial.println("[Cloud] Starting history logging loop.");

    // ── الحلقة الرئيسية ───────────────────────────────────────────────────────
    for (;;) {

        // ── [1] انتظر حتى يكون WiFi متصلاً ──────────────────────────────────
        //  xEventGroupWaitBits(group, bits, clearOnExit, waitAll, timeout)
        //   pdFALSE = لا تمسح الـ bit بعد الاستيقاظ (WiFiWatchdog مسؤول عنه)
        //   pdFALSE = يكفي bit واحد فقط من القائمة
        //   portMAX_DELAY = انتظر بلا حد — وفّر CPU أثناء الانتظار
        xEventGroupWaitBits(
            xSystemEventGroup,
            BIT_WIFI_CONNECTED,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY
        );

        // ── [2] اقرأ آخر قراءة للحساسات ─────────────────────────────────────
        //  g_latestSensorData مشترك بين المهام → نحتاج xSensorMutex
        //  timeout قصير (500ms): لو xSensorMutex مشغول أكثر من هذا
        //  فهناك مشكلة في SensorRead، نتخطى هذه الدورة.
        SensorData_t pkt;
        bool gotData = false;

        if (xSemaphoreTake(xSensorMutex, pdMS_TO_TICKS(500)) == pdTRUE) {
            pkt     = g_latestSensorData;   // نسخ بالقيمة — آمن
            gotData = true;
            xSemaphoreGive(xSensorMutex);
        } else {
            Serial.println("[Cloud] xSensorMutex timeout — skipping this cycle");
        }

        // ── [3] أرسل إلى Google Sheets ────────────────────────────────────────
        if (gotData) {

            // تحقق من صحة البيانات قبل الإرسال
            if (isnan(pkt.temperature) && isnan(pkt.humidity)) {
                Serial.println("[Cloud] Invalid sensor data — skipping HTTP");

            } else {
                // خذ xWiFiMutex — يمنع التزامن مع Telegram SSL
                // timeout طويل (10 ثواني) لأن Telegram SSL قد يستغرق وقتاً
                if (xSemaphoreTake(xWiFiMutex, pdMS_TO_TICKS(10000)) == pdTRUE) {

                    String url = buildRequestURL(pkt);
                    Serial.print("[Cloud] Sending to Google Sheets... ");

                    bool ok = doHTTPRequest(url);

                    xSemaphoreGive(xWiFiMutex);

                    if (!ok) {
                        // فشل الإرسال — نسجّله فقط، النظام يكمل
                        // الإرسال التالي سيحدث بعد LOG_INTERVAL
                        Serial.println("[Cloud] Send failed. Will retry next cycle.");
                    }

                } else {
                    // xWiFiMutex مشغول لفترة طويلة — يشير لمشكلة في Telegram
                    // أو deadlock. نتخطى ولا ننتظر.
                    Serial.println("[Cloud] xWiFiMutex timeout (10s) — skipping HTTP");
                }
            }
        }

        // ── [4] نام حتى الدورة القادمة ───────────────────────────────────────
        //  vTaskDelay تُنيم المهمة فعلياً بدون أكل CPU
        //  LOG_INTERVAL من config.h (افتراضياً 30 ثانية)
        //
        //  ملاحظة: لو WiFi انقطع أثناء النوم، خطوة [1] في الدورة التالية
        //  ستنتظر حتى يعود الاتصال قبل أي محاولة إرسال جديدة.
        vTaskDelay(pdMS_TO_TICKS(LOG_INTERVAL));
    }
}
