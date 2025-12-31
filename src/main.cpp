#include <Arduino.h>
#include <WiFi.h>
#include "time.h" // مكتبة الوقت الضرورية
#include "config.h"
#include "sensor_manager.h"
#include "telegram_manager.h"
#include "cloud_manager.h"    
#include "firebase_manager.h" 

unsigned long lastSystemTick = 0;
const long SYSTEM_TICK_INTERVAL = 2000;

unsigned long lastDataLog = 0;

// إعدادات التوقيت (GMT offset)
// 3600 ثانية * عدد الساعات. 
// مثلاً للسودان/مصر (GMT+2) نضع 7200، للسعودية/شرق أفريقيا (GMT+3) نضع 10800
const long  gmtOffset_sec = 3 * 3600; // عدل الرقم 3 حسب توقيت دولتك
const int   daylightOffset_sec = 0;   // عادة 0 في الدول العربية

void setup() {
    Serial.begin(115200);
    initSensors();
    
    // 1. الاتصال بالواي فاي
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nConnected!");

    // 2. ضبط الوقت من الإنترنت (مهم جداً)
    configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org", "time.nist.gov");
    Serial.print("Waiting for NTP time sync: ");
    struct tm timeinfo;
    // ننتظر حتى يتم جلب الوقت الصحيح
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

    if (currentMillis - lastSystemTick >= SYSTEM_TICK_INTERVAL) {
        lastSystemTick = currentMillis;

        float temp = getRawTemperature();
        float hum = getRawHumidity();
        bool isFire = isFlameDetected();
        String flameStr = isFire ? "DETECTED" : "Safe";

        // تحديث الشاشة اللحظي
        sendDataToFirebase(temp, hum, flameStr);
        checkSystemConditions(temp, hum, isFire);

        // الأرشفة كل فترة (مثلاً 5 دقائق)
        if (currentMillis - lastDataLog >= LOG_INTERVAL) {
            Serial.println("--- Logging Data ---");
            
            // 1. إرسال لقوقل شيت (كما طلبت الاحتفاظ به)
            logDataToGoogleSheet(temp, hum, isFire);

            // 2. إرسال لفايربيز مع التاريخ والوقت
            logHistoryToFirebase(temp, hum, flameStr);
            
            lastDataLog = currentMillis;
        }
    }
}