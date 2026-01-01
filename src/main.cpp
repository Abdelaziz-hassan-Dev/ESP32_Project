#include <Arduino.h>
#include <WiFi.h>
#include "time.h" 
#include "config.h"
#include "sensor_manager.h"
#include "telegram_manager.h"
#include "cloud_manager.h"    
#include "firebase_manager.h" 

unsigned long lastSystemTick = 0;
const long SYSTEM_TICK_INTERVAL = 2000;

unsigned long lastDataLog = 0;

// NTP Time settings (Adjust gmtOffset_sec for your timezone)
const long  gmtOffset_sec = 3 * 3600; 
const int   daylightOffset_sec = 0;

void setup() {
    Serial.begin(115200);
    initSensors();
    
    // WiFi Connection
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nConnected!");

    // Valid system time is critical for historical data logging
    configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org", "time.nist.gov");
    Serial.print("Waiting for NTP time sync: ");
    struct tm timeinfo;
    
    // Blocking wait until time is synced to avoid invalid timestamps in logs
    while(!getLocalTime(&timeinfo)){
        Serial.print(".");
        delay(500);
    }
    Serial.println("\nTime Synced!");

    initTelegram();
    initCloud();     
    initFirebase();  
}

void loop() {
    unsigned long currentMillis = millis();

    // Non-blocking delay for system loop
    if (currentMillis - lastSystemTick >= SYSTEM_TICK_INTERVAL) {
        lastSystemTick = currentMillis;

        float temp = getRawTemperature();
        float hum = getRawHumidity();
        bool isFire = isFlameDetected();
        String flameStr = isFire ? "DETECTED" : "Safe";

        // Real-time updates for the dashboard
        sendDataToFirebase(temp, hum, flameStr);
        checkSystemConditions(temp, hum, isFire);

        // Periodic historical logging (e.g., every 5 minutes)
        if (currentMillis - lastDataLog >= LOG_INTERVAL) {
            Serial.println("--- Logging Data ---");
            
            // 1. Log to Google Sheets (External backup)
            logDataToGoogleSheet(temp, hum, isFire);

            // 2. Log to Firebase History (Appends new node with timestamp)
            logHistoryToFirebase(temp, hum, flameStr);
            
            lastDataLog = currentMillis;
        }
    }
}