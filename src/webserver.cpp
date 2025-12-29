#include "webserver.h"
#include "sensors.h"
#include "config.h"
#include <LittleFS.h> // [مهم] استدعاء مكتبة نظام الملفات

// لم نعد بحاجة لدالة processor القديمة لأننا نستخدم AJAX بالكامل

void initWebServer(AsyncWebServer* server) {
    
    // 1. تهيئة نظام الملفات
    if(!LittleFS.begin()){
        Serial.println("An Error has occurred while mounting LittleFS");
        return;
    }

    // 2. تقديم الملفات الثابتة (Static Files)
    // هذا السطر السحري يغنيك عن كتابة كود لكل ملف
    // أي طلب للمسار الرئيسي سيتم توجيهه للملفات الموجودة في الذاكرة
    server->serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

    // 3. API Endpoints (بقيت كما هي لتزويد الجافاسكربت بالبيانات)
    server->on("/temperature", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(200, "text/plain", readTemperature().c_str());
    });

    server->on("/humidity", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(200, "text/plain", readHumidity().c_str());
    });

    server->on("/flame", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(200, "text/plain", readFlameStatus().c_str());
    });

    // Start server
    server->begin();
    Serial.println("Web Server Started & LittleFS Mounted");
}