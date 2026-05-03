// src/telegram_manager.cpp
#include "telegram_manager.h"
#include "rtos_shared.h"

// WiFiClientSecure و bot يبقيان هنا لكن محميان الآن بالـ xWiFiMutex
// (راجع OQ-2 في خطة الـ RTOS — نقطة مفتوحة للمراجعة لاحقاً)
static WiFiClientSecure client;
static UniversalTelegramBot bot(BOT_TOKEN, client);

void initTelegram() {
    client.setInsecure();
}

// دالة مساعدة داخلية — تأخذ المutex وترسل
// كل المنطق الخارجي يمر منها
static void safeSendMessage(const String& message) {
    if (xSemaphoreTake(xWiFiMutex, pdMS_TO_TICKS(5000)) == pdTRUE) {
        bot.sendMessage(CHAT_ID, message, "");
        xSemaphoreGive(xWiFiMutex);
    } else {
        // لو ما أخذنا المutex في 5 ثواني، نتخطى هذا التنبيه
        Serial.println("[Alert] xWiFiMutex timeout — skipping message");
    }
}

// ======================================================
// vTaskAlertEngine — المهمة الجديدة
// تعيش على Core 0، Priority 4
// ======================================================
void vTaskAlertEngine(void* pvParameters) {

    // ✅ الحل 1: متغيرات الـ cooldown هنا داخل المهمة
    // لا أحد يعرف عنها غير هذه المهمة
    TickType_t lastTempAlert  = 0;
    TickType_t lastHumAlert   = 0;
    TickType_t lastFlameAlert = 0;

    SensorData_t pkt;  // وعاء لاستقبال البيانات من الـ Queue

    for (;;) {

        // ✅ المهمة تنتظر هنا حتى تصل بيانات جديدة
        // portMAX_DELAY = انتظر إلى الأبد (لا تستهلك CPU أثناء الانتظار)
        if (xQueueReceive(xAlertQueue, &pkt, portMAX_DELAY) == pdTRUE) {

            TickType_t now = pkt.capturedAt; // الوقت من نفس الحزمة

            // --- منطق الحريق (الأعلى أولوية) ---
            if (pkt.flameDetected) {
                // pdMS_TO_TICKS يحوّل الـ milliseconds إلى تكات FreeRTOS
                if ((now - lastFlameAlert) > pdMS_TO_TICKS(FLAME_COOLDOWN)) {
                    String msg = "Fire Detected!\n";
                    msg += "Time: " + String(now);
                    safeSendMessage(msg);       // ← SSL محمي بالـ Mutex
                    lastFlameAlert = now;

                    // أعلم بقية المهام عبر Event Group
                    xEventGroupSetBits(xSystemEventGroup, BIT_FIRE_DETECTED);
                }
            } else {
                // امسح الـ bit لو الحريق انتهى
                xEventGroupClearBits(xSystemEventGroup, BIT_FIRE_DETECTED);
            }

            // --- منطق الحرارة ---
            if (!isnan(pkt.temperature) && pkt.temperature > TEMP_HIGH_LIMIT) {
                if ((now - lastTempAlert) > pdMS_TO_TICKS(ALARM_COOLDOWN)) {
                    String msg = "High Temperature Alert!\n";
                    msg += "Temp: " + String(pkt.temperature, 1) + " C\n";
                    msg += "Limit: " + String(TEMP_HIGH_LIMIT, 1) + " C";
                    safeSendMessage(msg);
                    lastTempAlert = now;
                }
            }

            // --- منطق الرطوبة ---
            if (!isnan(pkt.humidity) && pkt.humidity > HUM_HIGH_LIMIT) {
                if ((now - lastHumAlert) > pdMS_TO_TICKS(ALARM_COOLDOWN)) {
                    String msg = "High Humidity Alert!\n";
                    msg += "Humidity: " + String(pkt.humidity, 1) + "%\n";
                    msg += "Limit: " + String(HUM_HIGH_LIMIT, 1) + "%";
                    safeSendMessage(msg);
                    lastHumAlert = now;
                }
            }
        }
    }
}