#include "telegram_manager.h"

WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

// متغيرات لحفظ وقت آخر تنبيه (لمنع الإزعاج)
unsigned long lastTempAlert = 0;
unsigned long lastHumAlert = 0;
unsigned long lastFlameAlert = 0;

void initTelegram() {
    // التلقرام يتطلب اتصال آمن، ولتسهيل الأمر في المشاريع نجعل الشهادة غير مطلوبة
    client.setInsecure();
    // رسالة ترحيبية عند تشغيل الجهاز
    // bot.sendMessage(CHAT_ID, "🚀 System Started: Monitoring Active...", "");
}

void sendTelegramMessage(String message) {
    bot.sendMessage(CHAT_ID, message, "");
}

void checkSystemConditions(float temp, float hum, bool flame) {
    unsigned long currentMillis = millis();

    // 1. منطق الحريق (الأولوية القصوى)
    if (flame) {
        if (currentMillis - lastFlameAlert > FLAME_COOLDOWN) {
            String msg = "⚠️ Fire Detected! ⚠️\n";
            //msg += "Please check the area immediately.";
            sendTelegramMessage(msg);
            lastFlameAlert = currentMillis;
        }
    }

    // 2. منطق الحرارة
    if (!isnan(temp) && temp > TEMP_HIGH_LIMIT) {
        if (currentMillis - lastTempAlert > ALARM_COOLDOWN) {
            String msg = "⚠️ High Temperature Alert!⚠️\n";
            msg += "Current Temp: " + String(temp, 1) + "°C\n";
            msg += "Threshold: " + String(TEMP_HIGH_LIMIT, 1) + "°C";
            sendTelegramMessage(msg);
            lastTempAlert = currentMillis;
        }
    }

    // 3. منطق الرطوبة
    if (!isnan(hum) && hum > HUM_HIGH_LIMIT) {
        if (currentMillis - lastHumAlert > ALARM_COOLDOWN) {
            String msg = "⚠️ High Humidity Alert! ⚠️\n";
            msg += "Current Hum: " + String(hum, 1) + "%\n";
            msg += "Threshold: " + String(HUM_HIGH_LIMIT, 1) + "%";
            sendTelegramMessage(msg);
            lastHumAlert = currentMillis;
        }
    }
}