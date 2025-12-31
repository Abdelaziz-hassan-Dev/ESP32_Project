#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "sensor_manager.h"
#include "telegram_manager.h"
#include "cloud_manager.h"    // ضروري لـ Google Sheets
#include "firebase_manager.h" // ضروري لـ Firebase

unsigned long lastSystemTick = 0;
const long SYSTEM_TICK_INTERVAL = 2000;

// متغير واحد للتحكم في توقيت الأرشفة للنظامين (لتوحيد الوقت)
unsigned long lastDataLog = 0;

void setup() {
    Serial.begin(115200); // تأكد من الـ Baud Rate
    initSensors();
    
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nConnected!");

    initTelegram();
    initCloud();     // تهيئة Google Sheets
    initFirebase();  // تهيئة Firebase
}

void loop() {
    unsigned long currentMillis = millis();

    // 1. التحديث اللحظي للشاشة وقراءة الحساسات (كل ثانيتين)
    if (currentMillis - lastSystemTick >= SYSTEM_TICK_INTERVAL) {
        lastSystemTick = currentMillis;

        float temp = getRawTemperature();
        float hum = getRawHumidity();
        bool isFire = isFlameDetected();
        String flameStr = isFire ? "DETECTED" : "Safe";

        // إرسال البيانات اللحظية للشاشة (Real-time)
        sendDataToFirebase(temp, hum, flameStr);
        
        // فحص التنبيهات
        checkSystemConditions(temp, hum, isFire);

        // 2. الأرشفة التاريخية (كل فترة زمنية محددة مثل 5 دقائق)
        // نستخدم نفس التوقيت للنظامين لضمان تطابق البيانات
        if (currentMillis - lastDataLog >= LOG_INTERVAL) {
            Serial.println("--- Starting Data Logging ---");
            
            // أرشفة في Google Sheets
            logDataToGoogleSheet(temp, hum, isFire);

            // أرشفة في Firebase History (الدالة الجديدة)
            logHistoryToFirebase(temp, hum, flameStr);
            
            lastDataLog = currentMillis;
            Serial.println("--- Logging Complete ---");
        }
    }
}