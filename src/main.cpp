// =============================================================================
//  main.cpp — RTOS Version (Phase 1 → 4)
//  المشروع: IoT Safety Monitor — ESP3Z
//  التغيير الجوهري عن النسخة القديمة:
//  ────────────────────────────────────
//  القديم: كل شيء يشتغل تسلسلياً في loop() على core واحد
//  الجديد: كل مسؤولية في مهمة (Task) مستقلة، تشتغل بالتوازي على core مخصص
// =============================================================================

#include <Arduino.h>
#include <WiFi.h>
#include "time.h"
#include "config.h"

// ─── Headers للملفات الجديدة ───────────────────────────────────────────────
#include "rtos_shared.h"        // ← جديد: SensorData_t + كل الـ handles
#include "sensor_manager.h"     // ← تحتوي الآن على vTaskSensorRead
#include "telegram_manager.h"   // ← تحتوي الآن على vTaskAlertEngine
#include "firebase_manager.h"   // ← تحتوي الآن على vTaskFirebaseManager
#include "cloud_manager.h"      // ← تحتوي الآن على vTaskHistoryLogger
#include "wifi_watchdog.h"      // ← جديد كلياً: vTaskWiFiWatchdog


// =============================================================================
//  HOOK: Stack Overflow Detection
//  ────────────────────────────────
//  لو أي مهمة تعدّت الـ stack المخصص لها، FreeRTOS يستدعي هذه الدالة تلقائياً
//  بدلها لو ما عرّفناها: crash صامت يصعب تشخيصه
//  المهمة هنا تطبع اسم المهمة التي فاض stack-ها على Serial
// =============================================================================
extern "C" void vApplicationStackOverflowHook(TaskHandle_t xTask,
                                               char* pcTaskName) {
    Serial.print("[FATAL] Stack overflow in task: ");
    Serial.println(pcTaskName);
    // نوقف الشغل بشكل آمن — في الإنتاج ممكن تضيف ESP.restart()
    while (true) { vTaskDelay(pdMS_TO_TICKS(1000)); }
}


// =============================================================================
//  Callback للـ Software Timer (xLogTimer)
//  ─────────────────────────────────────────
//  هذه الدالة لا تشتغل هي بنفسها — FreeRTOS يستدعيها كل LOG_INTERVAL ثانية
//  من Timer Daemon Task (ذو أولوية عالية)
//
//  ⚠️ قاعدة مهمة: callback الـ timer يجب أن يكون سريعاً جداً
//     لا تسوي HTTP، لا SSL، لا أي شغل ثقيل هنا
//     الشغل الحقيقي يصير في vTaskHistoryLogger — نحن فقط ننبّهها
// =============================================================================
void vLogTimerCallback(TimerHandle_t xTimer) {
    // نرسل byte واحد (قيمته 1) إلى xHistoryQueue كإشارة "وقت التسجيل"
    // fromISR لأن callback الـ timer يشتغل من سياق interrupt-like
    uint8_t trigger = 1;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    xQueueSendFromISR(xHistoryQueue, &trigger, &xHigherPriorityTaskWoken);

    // لو الإرسال "أيقظ" مهمة أعلى أولوية، اطلب من المجدول أن يتحوّل إليها فوراً
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}


// =============================================================================
//  setup() — يشتغل مرة واحدة عند التشغيل
//  ─────────────────────────────────────────
//  المسؤوليات:
//  1. تهيئة Serial والحساسات
//  2. الاتصال بالـ WiFi (blocking هنا لأننا نحتاجه قبل كل شيء)
//  3. إطلاق مزامنة NTP (configTime فقط — الانتظار في vTaskWiFiWatchdog)
//  4. تهيئة المكتبات (Telegram, Cloud, Firebase)
//  5. xRTOS_Init() — إنشاء كل الـ queues والـ mutexes والـ event group
//  6. إنشاء المهام وتثبيت كل منها على الـ core المناسب
//  7. إنشاء xLogTimer
// =============================================================================
void setup() {
    Serial.begin(BAUD_RATE);
    Serial.println("\n\n========== System Boot ==========");

    // ── 1. تهيئة الحساسات ───────────────────────────────────────────────────
    initSensors();

    // ── 2. الاتصال بالـ WiFi ─────────────────────────────────────────────────
    //  هذا blocking لكنه ضروري — Firebase وTelegram لا يعملان بدون WiFi
    //  vTaskWiFiWatchdog ستتولى إعادة الاتصال لو انقطع لاحقاً
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nWiFi Connected! IP: " + WiFi.localIP().toString());

    // ── 3. إطلاق NTP (غير blocking الآن) ────────────────────────────────────
    //  القديم: while(!getLocalTime()) يحجب setup() إلى الأبد لو NTP فشل
    //  الجديد: configTime() تُطلق الطلب فقط، ثم vTaskWiFiWatchdog
    //          تنتظر المزامنة وترفع BIT_NTP_SYNCED لما تنجح
    //          vTaskHistoryLogger لن تكتب timestamps حتى يرتفع هذا الـ bit
    const long  gmtOffset_sec     = 3 * 3600;  // UTC+3 (Istanbul/Riyadh)
    const int   daylightOffset_sec = 0;
    configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org", "time.nist.gov");
    Serial.println("NTP sync initiated (non-blocking)...");

    // ── 4. تهيئة المكتبات ───────────────────────────────────────────────────
    //  هذه ما تتغير — نفس الدوال القديمة، لكنها الآن تهيّئ فقط
    //  الشغل الفعلي (HTTP, SSL) صار في المهام المخصصة
    initTelegram();
    initCloud();
    initFirebase();  // هذه ترفع BIT_FIREBASE_READY بعد signUp ناجح

    // ── 5. xRTOS_Init() ──────────────────────────────────────────────────────
    //  أهم خطوة — تُنشئ كل البنية التحتية:
    //  • xFirebaseQueue, xAlertQueue, xHistoryQueue
    //  • xSensorMutex, xWiFiMutex, xFirebaseMutex
    //  • xSystemEventGroup
    //  يجب أن تُستدعى قبل إنشاء أي مهمة لأن المهام تعتمد على هذه الـ handles
    xRTOS_Init();
    Serial.println("RTOS primitives created.");

    // ── 6. إنشاء المهام ──────────────────────────────────────────────────────
    //  xTaskCreatePinnedToCore(function, name, stack, param, priority, handle, core)
    //
    //  ترتيب الأولويات مهم:
    //  Priority 5 = SensorRead   (الأعلى — لا يُؤخَّر أبداً)
    //  Priority 4 = Firebase + Alert (متساويان — يتناوبان)
    //  Priority 3 = WiFiWatchdog  (متوسط)
    //  Priority 2 = HistoryLogger (الأدنى — يشتغل لما ما في شيء أهم)

    // ── Core 1: مهمة الحساسات ────────────────────────────────────────────────
    //  لماذا Core 1؟ WiFi stack يشتغل على Core 0 بأولوية عالية
    //  DHT22 يستخدم bitbanging timing حساس — أي interrupt من Core 0
    //  ممكن يخرب القراءة. Core 1 يعطينا timing نظيف ومستقر
    xTaskCreatePinnedToCore(
        vTaskSensorRead,    // الدالة
        "SensorRead",       // الاسم (يظهر في stack overflow hook)
        4096,               // Stack: 4 KB — كافي لـ DHT + packet
        NULL,               // لا نمرر parameters
        5,                  // Priority: الأعلى
        NULL,               // لا نحتاج TaskHandle
        1                   // Core 1 ← مخصص للحساسات
    );

    // ── Core 0: مهمة Firebase ────────────────────────────────────────────────
    //  لماذا 12 KB؟ Firebase library تخصص JSON documents على الـ stack
    //  وتعمل TLS internally — أقل من 8 KB = stack overflow في الاختبارات
    xTaskCreatePinnedToCore(
        vTaskFirebaseManager,
        "FirebaseManager",
        12288,              // Stack: 12 KB — Firebase TLS + JSON
        NULL,
        4,                  // Priority: عالي
        NULL,
        0                   // Core 0
    );

    // ── Core 0: مهمة التنبيهات (Telegram) ───────────────────────────────────
    //  لماذا 10 KB؟ UniversalTelegramBot تبني JSON response على الـ stack
    //  SSL handshake مع Telegram servers يحتاج buffer كبير
    xTaskCreatePinnedToCore(
        vTaskAlertEngine,
        "AlertEngine",
        10240,              // Stack: 10 KB — Telegram TLS + bot JSON
        NULL,
        4,                  // Priority: مساوي لـ Firebase (يتناوبان بعدالة)
        NULL,
        0                   // Core 0
    );

    // ── Core 0: مهمة الـ WiFi Watchdog ───────────────────────────────────────
    //  مسؤولة عن:
    //  • مراقبة حالة WiFi وإعادة الاتصال عند الانقطاع
    //  • انتظار NTP sync ورفع BIT_NTP_SYNCED
    //  • تحديث BIT_WIFI_CONNECTED في xSystemEventGroup
    xTaskCreatePinnedToCore(
        vTaskWiFiWatchdog,
        "WiFiWatchdog",
        3072,               // Stack: 3 KB — polling فقط، لا SSL
        NULL,
        3,                  // Priority: متوسط
        NULL,
        0                   // Core 0
    );

    // ── Core 0: مهمة التسجيل التاريخي ───────────────────────────────────────
    //  تنتظر trigger من xLogTimer — تشغّل Google Sheets + Firebase history
    //  أدنى priority لأن التأخير البسيط في التسجيل التاريخي مقبول
    xTaskCreatePinnedToCore(
        vTaskHistoryLogger,
        "HistoryLogger",
        12288,              // Stack: 12 KB — HTTP TLS + HTTPClient buffer
        NULL,
        2,                  // Priority: الأدنى
        NULL,
        0                   // Core 0
    );

    // ── 7. إنشاء xLogTimer ───────────────────────────────────────────────────
    //  Software Timer يُطلق callback كل LOG_INTERVAL millisecond
    //  الـ callback (vLogTimerCallback) لا يعمل شيئاً ثقيلاً —
    //  يرسل فقط byte إلى xHistoryQueue لإيقاظ vTaskHistoryLogger
    //
    //  pdTRUE = auto-reload (يتكرر تلقائياً)
    //  pdFALSE = one-shot (يُطلق مرة واحدة فقط)
    xLogTimer = xTimerCreate(
        "LogTimer",                         // الاسم
        pdMS_TO_TICKS(LOG_INTERVAL),        // الفترة (30 ثانية من config.h)
        pdTRUE,                             // auto-reload = يتكرر
        NULL,                               // Timer ID (لا نحتاجه)
        vLogTimerCallback                   // الدالة التي تُستدعى
    );

    // تأكد إن الـ timer اتنشأ بنجاح قبل تشغيله
    if (xLogTimer != NULL) {
        xTimerStart(xLogTimer, 0);
        Serial.println("History log timer started (" +
                       String(LOG_INTERVAL / 1000) + "s interval).");
    } else {
        Serial.println("[ERROR] Failed to create xLogTimer!");
    }

    Serial.println("========== All tasks launched ==========");
    Serial.println("Free heap: " + String(esp_get_free_heap_size()) + " bytes");
    // setup() ينتهي هنا — FreeRTOS scheduler يتولى الباقي
}


// =============================================================================
//  loop() — فاضية عن قصد
//  ─────────────────────────
//  في نظام RTOS، loop() تشتغل كـ "Idle Task" على Core 1
//  لو حطينا فيها كود، سيتنافس مع vTaskSensorRead على Core 1
//  نتركها فاضية ونضيف vTaskDelay لنعطي CPU للمهام الأخرى
//
//  ملاحظة: esp_get_free_heap_size() هنا مفيد لمراقبة الـ memory leak
//  خلال Phase 5 (stress testing)
// =============================================================================
void loop() {
    // لا شيء يشتغل هنا — كل الشغل في المهام
    // هذا الـ delay يمنع loop() من أكل CPU بشكل غير مفيد
    vTaskDelay(pdMS_TO_TICKS(10000));
}
