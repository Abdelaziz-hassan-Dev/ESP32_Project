#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "sensor_manager.h"
#include "telegram_manager.h"
#include "cloud_manager.h"
#include "firebase_manager.h" 

unsigned long lastSystemTick = 0;
const long SYSTEM_TICK_INTERVAL = 2000;

unsigned long lastGoogleLog = 0;

void setup() {
    Serial.begin(BAUD_RATE);
    initSensors();
    
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nConnected!");

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

        sendDataToFirebase(temp, hum, flameStr);
        checkSystemConditions(temp, hum, isFire);

        if (currentMillis - lastGoogleLog >= LOG_INTERVAL) {
            logDataToGoogleSheet(temp, hum, isFire);
            lastGoogleLog = currentMillis;
        }
    }
}

