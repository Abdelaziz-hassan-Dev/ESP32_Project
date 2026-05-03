// الملف الجديد — sensor_manager.cpp
#include "sensor_manager.h"
#include "rtos_shared.h"   // ← جديد: للوصول إلى SensorData_t والـ handles

static DHT dhtSensor(DHTPIN, DHTTYPE);
static bool sensorsInitialized = false;

void initSensors() {
    if (!sensorsInitialized) {
        pinMode(FLAME_PIN, INPUT);
        dhtSensor.begin();
        sensorsInitialized = true;
    }
}

// ======================================================
// vTaskSensorRead — المهمة الجديدة (تشتغل كل 2 ثانية)
// ======================================================
void vTaskSensorRead(void* pvParameters) {

    for (;;) {   // ← حلقة لا نهائية، هذا هو "loop()" الخاص بالمهمة

        // 1. اقرأ الحرارة والرطوبة بشكل ذري (بدون أي yield بينهما)
        float temp  = dhtSensor.readTemperature();
        float hum   = dhtSensor.readHumidity();
        bool  flame = (digitalRead(FLAME_PIN) == LOW);

        // 2. احزم البيانات في struct واحد
        SensorData_t packet;
        packet.temperature  = temp;
        packet.humidity     = hum;
        packet.flameDetected = flame;
        packet.capturedAt   = xTaskGetTickCount(); // ← بدل millis()

        // 3. احفظ آخر قراءة في المتغير العام (تحت حماية Mutex)
        if (xSemaphoreTake(xSensorMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            g_latestSensorData = packet;
            xSemaphoreGive(xSensorMutex);
        }

        // 4. أرسل نسخة إلى Firebase (لو Queue ممتلئ → تجاهل، ولا تنتظر)
        xQueueSend(xFirebaseQueue, &packet, pdMS_TO_TICKS(50));

        // 5. أرسل نسخة إلى مهمة التنبيهات
        xQueueSend(xAlertQueue, &packet, pdMS_TO_TICKS(50));

        // 6. انتظر 2 ثانية قبل القراءة التالية
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
