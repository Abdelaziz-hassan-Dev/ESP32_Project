// =============================================================================
//  firebase_manager.cpp — مدير Firebase (نسخة RTOS)
//  ────────────────────────────────────────────────────
//  هذا الملف موجود في:  src/firebase_manager.cpp
//
//  مقارنة سريعة بين القديم والجديد:
//  ──────────────────────────────────
//
//  القديم (super-loop):
//  ┌─────────────────────────────────────────────────────┐
//  │ loop() كل 2 ثانية:                                  │
//  │   sendDataToFirebase(temp, hum, flame)               │
//  │     └─ if millis() - prev > 2000 → updateNode()     │
//  │                                                      │
//  │ loop() كل LOG_INTERVAL:                              │
//  │   logHistoryToFirebase(temp, hum, flame)             │
//  │     └─ pushJSON() مباشرة                             │
//  └─────────────────────────────────────────────────────┘
//  المشاكل:
//   • signupOK bool عادي = لا memory fence = ممكن CPU يكش القيمة
//   • sendDataPrevMillis = millis() coupling مع cloud logic
//   • fbdo/auth/config globals = أي كود يستطيع كسرهم
//   • لا حماية ضد race condition لو استُدعيت من مكانين
//
//  الجديد (RTOS):
//  ┌─────────────────────────────────────────────────────┐
//  │ vTaskFirebaseManager يشتغل بشكل مستقل:             │
//  │                                                     │
//  │   xQueueReceive(xFirebaseQueue) ← SensorData_t     │
//  │     └─ xFirebaseMutex → updateNode("/sensor")       │
//  │                                                     │
//  │   xQueueReceive(xHistoryQueue)  ← trigger byte     │
//  │     └─ xSensorMutex (اقرأ آخر قراءة)               │
//  │     └─ xFirebaseMutex → pushJSON("/history")        │
//  └─────────────────────────────────────────────────────┘
//  الحلول:
//   • BIT_FIREBASE_READY في EventGroup = atomic thread-safe
//   • لا millis() = vTaskDelay يتحكم في الـ pacing
//   • xFirebaseMutex يحمي fbdo/auth/config
//   • كل Firebase calls في مهمة واحدة فقط = صفر race conditions
// =============================================================================

#include "firebase_manager.h"
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#include <time.h>

// =============================================================================
//  المتغيرات الـ Global (محمية الآن بـ xFirebaseMutex)
//  ────────────────────────────────────────────────────
//  لماذا لا تزال global؟
//  Firebase_ESP_Client تحتاج هذه الكائنات تبقى حية طوال عمر البرنامج.
//  الفرق: الآن لا أحد يلمسها غير vTaskFirebaseManager،
//  وكل لمسة محاطة بـ xSemaphoreTake/Give.
// =============================================================================
static FirebaseData   fbdo;    // كائن البيانات — يستخدمه كل RTDB call
static FirebaseAuth   auth;    // credentials
static FirebaseConfig config;  // api_key + database_url


// =============================================================================
//  initFirebase() — تُستدعى مرة واحدة في setup()
//  ─────────────────────────────────────────────────
//  التغيير الجوهري:
//    القديم: signupOK = true         ← bool عادي، ممكن CPU يكش القيمة
//    الجديد: xEventGroupSetBits()    ← atomic، مضمون كل المهام ترى التغيير
//
//  xSystemEventGroup ممكن يكون NULL لو استُدعيت قبل xRTOS_Init()
//  لذلك نتحقق منه أولاً.
// =============================================================================
void initFirebase() {
    config.api_key      = API_KEY;
    config.database_url = DATABASE_URL;

    if (Firebase.signUp(&config, &auth, "", "")) {
        Serial.println("[Firebase] Auth OK");

        // ← الفرق الرئيسي عن القديم
        if (xSystemEventGroup != NULL) {
            xEventGroupSetBits(xSystemEventGroup, BIT_FIREBASE_READY);
            Serial.println("[Firebase] BIT_FIREBASE_READY set");
        }
    } else {
        Serial.printf("[Firebase] Auth FAILED: %s\n",
                      config.signer.signupError.message.c_str());
        // BIT_FIREBASE_READY يبقى 0 → vTaskFirebaseManager تنتظر ولا تكتب
    }

    config.token_status_callback = tokenStatusCallback;
    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
}


// =============================================================================
//  دوال مساعدة داخلية — static = مخفية عن بقية الملفات
// =============================================================================

// ── getFormattedTime() ────────────────────────────────────────────────────────
//  تُرجع timestamp بصيغة "YYYY-MM-DD HH:MM:SS"
//  لو NTP ما اتزامن → تُرجع "Time Error" بدل timestamp خاطئ
//  تُستدعى فقط داخل xFirebaseMutex
// =============================================================================
static String getFormattedTime() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return "Time Error";
    }
    char buf[30];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return String(buf);
}


// ── sendRealtimeUpdate() ─────────────────────────────────────────────────────
//  تُحدّث /sensor بآخر قراءة (للـ dashboard real-time)
//
//  ⚠️ تُستدعى فقط بعد أخذ xFirebaseMutex من المهمة الرئيسية
//  لا تستدعيها مباشرة من أي مكان آخر
// =============================================================================
static void sendRealtimeUpdate(const SensorData_t& pkt) {
    FirebaseJson json;

    if (!isnan(pkt.temperature)) json.set("temperature", pkt.temperature);
    if (!isnan(pkt.humidity))    json.set("humidity",    pkt.humidity);
    json.set("flame", pkt.flameDetected ? "DETECTED" : "Safe");

    if (Firebase.RTDB.updateNode(&fbdo, "/sensor", &json)) {
        Serial.printf("[Firebase] /sensor OK: %.1fC  %.1f%%  %s\n",
                      pkt.temperature, pkt.humidity,
                      pkt.flameDetected ? "FIRE" : "Safe");
    } else {
        Serial.println("[Firebase] /sensor FAIL: " + fbdo.errorReason());
    }
}


// ── sendHistoryEntry() ───────────────────────────────────────────────────────
//  تُضيف entry جديدة في /history بـ timestamp
//
//  تتحقق من BIT_NTP_SYNCED أولاً:
//  لو الوقت غير متزامن → تتخطى الكتابة
//  timestamp خاطئ (مثل 1970-01-01) في السجل أسوأ من فراغ
//
//  ⚠️ تُستدعى فقط بعد أخذ xFirebaseMutex من المهمة الرئيسية
// =============================================================================
static void sendHistoryEntry(const SensorData_t& pkt) {

    // تحقق من NTP — لو ما اتزامن بعد، تخطَّ
    EventBits_t bits = xEventGroupGetBits(xSystemEventGroup);
    if (!(bits & BIT_NTP_SYNCED)) {
        Serial.println("[Firebase] History skipped — NTP not synced");
        return;
    }

    FirebaseJson json;
    if (!isnan(pkt.temperature)) json.set("temperature", pkt.temperature);
    if (!isnan(pkt.humidity))    json.set("humidity",    pkt.humidity);
    json.set("flame",    pkt.flameDetected ? "DETECTED" : "Safe");
    json.set("datetime", getFormattedTime());

    Serial.print("[Firebase] Pushing /history... ");

    if (Firebase.RTDB.pushJSON(&fbdo, "/history", &json)) {
        Serial.println("Done!");
    } else {
        Serial.println("FAIL: " + fbdo.errorReason());
    }
}


// =============================================================================
//  vTaskFirebaseManager — المهمة الرئيسية
//  ─────────────────────────────────────────
//  Core 0 | Priority 4 | Stack 12 KB
//
//  لماذا 12 KB؟
//  Firebase_ESP_Client تخصص JSON على الـ stack أثناء serialize
//  وتعمل TLS internally. أقل من 8 KB = stack overflow مضمون.
//
//  آلية الشغل (نمط "أيهما أسبق"):
//  ────────────────────────────────
//
//    [1] xQueueReceive(xFirebaseQueue, timeout=0) → real-time؟
//          نعم → xFirebaseMutex → updateNode("/sensor")
//
//    [2] xQueueReceive(xHistoryQueue, timeout=0)  → history trigger؟
//          نعم → xSensorMutex (اقرأ آخر قراءة)
//               → xFirebaseMutex → pushJSON("/history")
//
//    [3] كلاهما فارغ → نام 100ms (CPU-free)
//
//  لماذا لا portMAX_DELAY؟
//  ──────────────────────────
//  xQueueReceive تنتظر على queue واحدة فقط.
//  لو انتظرنا على xFirebaseQueue بـ portMAX_DELAY،
//  نفوت triggers من xHistoryQueue.
//  الحل: نتحقق من كل queue بـ timeout=0، لو كلاهما فارغ → نام 100ms.
// =============================================================================
void vTaskFirebaseManager(void* pvParameters) {

    SensorData_t sensorPkt;   // استقبال real-time data
    uint8_t      histTrigger; // استقبال history trigger (byte واحد)

    // ── انتظر BIT_FIREBASE_READY ──────────────────────────────────────────────
    //  لا تبدأ أي كتابة حتى initFirebase() تنجح
    //  xEventGroupWaitBits(group, bits, clearOnExit, waitAll, timeout)
    //    pdFALSE = لا تمسح الـ bit بعد الاستيقاظ (المهمة ما تعيد تصفيره)
    //    pdFALSE = يكفي bit واحد من المطلوبين
    //    portMAX_DELAY = انتظر بلا حد (OK هنا لأن الـ task ما تشتغل بدونه)
    Serial.println("[Firebase] Waiting for BIT_FIREBASE_READY...");
    xEventGroupWaitBits(
        xSystemEventGroup,
        BIT_FIREBASE_READY,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY
    );
    Serial.println("[Firebase] Ready. Starting task loop.");

    // ── الحلقة الرئيسية ───────────────────────────────────────────────────────
    for (;;) {

        bool didWork = false;

        // ──────────────────────────────────────────────────────────────────────
        //  [1] Real-time update — من xFirebaseQueue
        //  timeout=0 → non-blocking، إما في الـ queue أو لا
        // ──────────────────────────────────────────────────────────────────────
        if (xQueueReceive(xFirebaseQueue, &sensorPkt, 0) == pdTRUE) {

            if (xSemaphoreTake(xFirebaseMutex, pdMS_TO_TICKS(3000)) == pdTRUE) {
                sendRealtimeUpdate(sensorPkt);
                xSemaphoreGive(xFirebaseMutex);
            } else {
                Serial.println("[Firebase] Mutex timeout — realtime skipped");
            }

            didWork = true;
        }

        // ──────────────────────────────────────────────────────────────────────
        //  [2] History log — من xHistoryQueue
        //  يأتي من vLogTimerCallback كل LOG_INTERVAL ثانية
        //  نحتاج آخر قراءة → نأخذها من g_latestSensorData تحت xSensorMutex
        // ──────────────────────────────────────────────────────────────────────
        if (xQueueReceive(xHistoryQueue, &histTrigger, 0) == pdTRUE) {

            // اقرأ آخر قراءة بأمان
            SensorData_t histPkt;
            bool gotData = false;

            if (xSemaphoreTake(xSensorMutex, pdMS_TO_TICKS(500)) == pdTRUE) {
                histPkt  = g_latestSensorData;
                gotData  = true;
                xSemaphoreGive(xSensorMutex);
            } else {
                Serial.println("[Firebase] xSensorMutex timeout — history skipped");
            }

            // اكتب في Firebase لو حصلنا على بيانات
            if (gotData) {
                if (xSemaphoreTake(xFirebaseMutex, pdMS_TO_TICKS(5000)) == pdTRUE) {
                    sendHistoryEntry(histPkt);
                    xSemaphoreGive(xFirebaseMutex);
                } else {
                    Serial.println("[Firebase] Mutex timeout — history skipped");
                }
            }

            didWork = true;
        }

        // ──────────────────────────────────────────────────────────────────────
        //  [3] لا شيء في أي queue — نام 100ms
        //  vTaskDelay يُنيم المهمة فعلياً، لا يستهلك CPU
        // ──────────────────────────────────────────────────────────────────────
        if (!didWork) {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}
