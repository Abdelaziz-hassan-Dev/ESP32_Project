#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "sensors.h"
#include "telegram_manager.h"
#include "cloud_manager.h"
#include "firebase_manager.h" 

// توقيتات النظام
unsigned long lastSystemTick = 0;
const long SYSTEM_TICK_INTERVAL = 2000;

unsigned long lastGoogleLog = 0;

void setup() {
    Serial.begin(BAUD_RATE);
    
    // 1. تهيئة الأجهزة والاتصال
    initSensors();
    
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nConnected!");

    // 2. تهيئة الخدمات السحابية
    initTelegram();
    initCloud(); 
    initFirebase();
}

void loop() {
    unsigned long currentMillis = millis();

    if (currentMillis - lastSystemTick >= SYSTEM_TICK_INTERVAL) {
        lastSystemTick = currentMillis;

        float temp = getRawTemperature();
        float hum = getRawHumidity();
        bool isFire = isFlameDetected();
        String flameStr = isFire ? "DETECTED" : "Safe";

        // التحقق من صلاحية البيانات قبل إكمال العمليات (Error Handling)
        if (isnan(temp) || isnan(hum)) {
            Serial.println("Error: Failed to read from DHT sensor!");
            return; // تخطي هذه الدورة إذا كانت القراءة خاطئة
        }

        sendDataToFirebase(temp, hum, flameStr);
        checkSystemConditions(temp, hum, isFire);

        // ج) تسجيل البيانات في Google Sheets
        // هذا الجزء له توقيت خاص (أبطأ)، لذا نضعه داخل شرط فرعي
        if (currentMillis - lastGoogleLog >= LOG_INTERVAL) {
            logDataToGoogleSheet(temp, hum, isFire);
            lastGoogleLog = currentMillis;
        }
        
    
     //   Serial.printf("Tick: T=%.1f, H=%.1f, Fire=%s\n", temp, hum, flameStr.c_str());
    }
}

