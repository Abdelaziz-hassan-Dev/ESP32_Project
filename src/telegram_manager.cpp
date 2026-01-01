#include "telegram_manager.h"

WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

// Cooldown timers to prevent alert flooding
unsigned long lastTempAlert = 0;
unsigned long lastHumAlert = 0;
unsigned long lastFlameAlert = 0;

void initTelegram() {
    // Bypass SSL certificate validation for simplicity in this prototype
    client.setInsecure();
}

void sendTelegramMessage(String message) {
    bot.sendMessage(CHAT_ID, message, "");
}

void checkSystemConditions(float temp, float hum, bool flame) {
    unsigned long currentMillis = millis();

    // 1. Fire Logic (High Priority)
    if (flame) {
        if (currentMillis - lastFlameAlert > FLAME_COOLDOWN) {
            String msg = "⚠️ Fire Detected! ⚠️\n";
            sendTelegramMessage(msg);
            lastFlameAlert = currentMillis;
        }
    }

    // 2. Temperature Logic
    if (!isnan(temp) && temp > TEMP_HIGH_LIMIT) {
        if (currentMillis - lastTempAlert > ALARM_COOLDOWN) {
            String msg = "⚠️ High Temperature Alert!⚠️\n";
            msg += "Current Temp: " + String(temp, 1) + "°C\n";
            msg += "Threshold: " + String(TEMP_HIGH_LIMIT, 1) + "°C";
            sendTelegramMessage(msg);
            lastTempAlert = currentMillis;
        }
    }

    // 3. Humidity Logic
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