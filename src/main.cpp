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

////////////////////////////////////////////////////////


// #include <Arduino.h>
// #include <WiFi.h>
// #include "config.h"
// #include "sensors.h"
// #include "telegram_manager.h"
// #include "cloud_manager.h"

// // مكتبات Firebase
// #include <Firebase_ESP_Client.h>
// #include <addons/TokenHelper.h>
// #include <addons/RTDBHelper.h>

// // متغير لتوقيت فحص التنبيهات (لا نريد الفحص في كل لحظة)
// unsigned long lastCheckTime = 0;
// const long CHECK_INTERVAL = 2000;
// unsigned long lastGoogleLog = 0;

// FirebaseData fbdo;
// FirebaseAuth auth;
// FirebaseConfig config;

// bool signupOK = false;
// unsigned long sendDataPrevMillis = 0;

// void setup() {
//     Serial.begin(BAUD_RATE);
//     initSensors();

//     WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
//     Serial.print("Connecting to WiFi");
//     while (WiFi.status() != WL_CONNECTED) {
//         delay(1000);
//         Serial.print(".");
//     }
//     Serial.println("\nConnected!");

//     // إعدادات Firebase
//     config.api_key = API_KEY;
//     config.database_url = DATABASE_URL;

//     // تسجيل الدخول كمستخدم مجهول (Anonymous) للتسهيل
//     if (Firebase.signUp(&config, &auth, "", "")) {
//         Serial.println("Firebase Connected");
//         signupOK = true;
//     } else {
//         Serial.printf("%s\n", config.signer.signupError.message.c_str());
//     }

//     Firebase.begin(&config, &auth);
//     Firebase.reconnectWiFi(true);

//     initTelegram();
//     initCloud(); 
// }

// void loop() {
//     // إرسال البيانات إلى Firebase كل 2 ثانية
//     if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 2000 || sendDataPrevMillis == 0)) {
//         sendDataPrevMillis = millis();

//         float t = getRawTemperature();
//         float h = getRawHumidity();
//         String flameStatus = isFlameDetected() ? "DETECTED" : "Safe";

//         // إرسال البيانات (المسارات في الداتابيس ستكون /sensor/temp وهكذا)
//         if (!isnan(t)) Firebase.RTDB.setFloat(&fbdo, "/sensor/temperature", t);
//         if (!isnan(h)) Firebase.RTDB.setFloat(&fbdo, "/sensor/humidity", h);
//         Firebase.RTDB.setString(&fbdo, "/sensor/flame", flameStatus);
        
//         Serial.println("Sent to Firebase");
//     }
//     //... كود التلقرام ...
//     unsigned long currentMillis = millis();
    
//     // نقوم بفحص القيم كل فترة محددة (Non-blocking delay)
//     if (currentMillis - lastCheckTime >= CHECK_INTERVAL) {
//         lastCheckTime = currentMillis;
        
//         // جلب القراءات الخام
//         float currentTemp = getRawTemperature();
//         float currentHum = getRawHumidity();
//         bool currentFlame = isFlameDetected();
        
//         // إرسال القراءات لمدير التنبيهات ليتصرف
//         checkSystemConditions(currentTemp, currentHum, currentFlame);
//     }
//         // 2. إرسال البيانات لجوجل شيت (كل دقيقة مثلاً) - [جديد]
//     if (currentMillis - lastGoogleLog >= LOG_INTERVAL) {
//         // نأخذ قراءات جديدة للتأكد
//         float t = getRawTemperature();
//         float h = getRawHumidity();
//         bool f = isFlameDetected(); // لاحظ أن دالة isFlameDetected ترجع true عند الخطر في كودك

//         // التحقق من صلاحية القراءات قبل الإرسال
//         if (!isnan(t) && !isnan(h)) {
//             logDataToGoogleSheet(t, h, f);
//             lastGoogleLog = currentMillis;
//         }
//     }

// }


////////////////////////////////////////////////////////////////////////////////////////////////////


// #include <Arduino.h>
// #include <WiFi.h>
// // #include <WiFiManager.h> 
// #include "config.h"
// #include "sensors.h"
// #include "webserver.h"
// #include "telegram_manager.h" // [جديد]
// #include "cloud_manager.h"

// AsyncWebServer server(SERVER_PORT);

// // متغير لتوقيت فحص التنبيهات (لا نريد الفحص في كل لحظة)
// unsigned long lastCheckTime = 0;
// const long CHECK_INTERVAL = 2000; // نفحص الحساسات للتنبيه كل 2 ثانية
// unsigned long lastGoogleLog = 0;

// void setup() {
//     Serial.begin(BAUD_RATE);
    
//     // 1. تشغيل الحساسات
//     initSensors();
    
//     // 2. الاتصال بالواي فاي
//     WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
//     Serial.print("Connecting to WiFi");
//     while (WiFi.status() != WL_CONNECTED) {
//         delay(1000);
//         Serial.print(".");
//     }
//     Serial.println("\nConnected to WiFi");
//     Serial.print("IP Address: ");
//     Serial.println(WiFi.localIP());

//     // 3. إعداد التلقرام [جديد]
// // تهيئة التلقرام والكلاود
//     initTelegram();
//     initCloud(); // [جديد]
    
//     initWebServer(&server);
// }

// void loop() {
//     // الكود هنا يجب أن يكون نظيفاً جداً
    
//     unsigned long currentMillis = millis();
    
//     // نقوم بفحص القيم كل فترة محددة (Non-blocking delay)
//     if (currentMillis - lastCheckTime >= CHECK_INTERVAL) {
//         lastCheckTime = currentMillis;
        
//         // جلب القراءات الخام
//         float currentTemp = getRawTemperature();
//         float currentHum = getRawHumidity();
//         bool currentFlame = isFlameDetected();
        
//         // إرسال القراءات لمدير التنبيهات ليتصرف
//         checkSystemConditions(currentTemp, currentHum, currentFlame);
//     }

//     // 2. إرسال البيانات لجوجل شيت (كل دقيقة مثلاً) - [جديد]
//     if (currentMillis - lastGoogleLog >= LOG_INTERVAL) {
//         // نأخذ قراءات جديدة للتأكد
//         float t = getRawTemperature();
//         float h = getRawHumidity();
//         bool f = isFlameDetected(); // لاحظ أن دالة isFlameDetected ترجع true عند الخطر في كودك

//         // التحقق من صلاحية القراءات قبل الإرسال
//         if (!isnan(t) && !isnan(h)) {
//             logDataToGoogleSheet(t, h, f);
//             lastGoogleLog = currentMillis;
//         }
//     }
    
//     // ملاحظة: الويب سيرفر يعمل في الخلفية بشكل Asynchronous
//     // لا تستخدم delay() طويلة هنا أبداً
// }



// ///////////////////////////////////////////////////////////////////////////



// // #include <WiFi.h>
// // #include "config.h"
// // #include "sensors.h"
// // #include "webserver.h"

// // AsyncWebServer server(SERVER_PORT);

// // void setup() {
// //     Serial.begin(BAUD_RATE);
    
// //     // Initialize sensors
// //     initSensors();
    
// //     // Connect to WiFi
// //     WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
// //     Serial.print("Connecting to WiFi");
    
// //     while (WiFi.status() != WL_CONNECTED) {
// //         delay(1000);
// //         Serial.print(".");
// //     }
    
// //     Serial.println("\nConnected to WiFi");
// //     Serial.print("IP Address: ");
// //     Serial.println(WiFi.localIP());
    
// //     // Initialize web server
// //     initWebServer(&server);
// // }

// // void loop() {
// //     // Main loop can handle other tasks
// //     // Web server runs asynchronously
// //     delay(1000); // Prevent watchdog timer issues
// // }


// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// // #include <WiFi.h>
// // #include <WiFiManager.h> 
// // #include "config.h"
// // #include "sensors.h"
// // #include "webserver.h"

// // AsyncWebServer server(SERVER_PORT);

// // void setup() {
// //     Serial.begin(BAUD_RATE);
    
// //     // 1. تشغيل الحساسات
// //     initSensors();

// //     // 2. إعداد WiFiManager
// //     WiFiManager wifiManager;

// //     // =============== التعديل الجديد هنا ===============
// //     // هذا السطر يمسح الإعدادات المحفوظة سابقاً في كل مرة يشتغل فيها الجهاز
// //     // وبالتالي سيجبره على فتح نقطة الوصول (AP) لتختار الشبكة من جديد
// //     wifiManager.resetSettings(); 
// //     // ==================================================

// //     Serial.println("Settings reset. Starting AP for configuration...");
    
// //     // سيقوم الآن بفتح شبكة Zizo-Project-AP دائماً وينتظرك تتصل بها
// //     // ولن يكمل الكود إلا بعد أن تختار الشبكة وتتصل بنجاح
// //     bool res = wifiManager.autoConnect("Zizo-Project-AP", "password123");

// //     if(!res) {
// //         Serial.println("Failed to connect");
// //         ESP.restart();
// //     } 
    
// //     Serial.println("\nConnected to WiFi!");
// //     Serial.print("IP Address: ");
// //     Serial.println(WiFi.localIP());

// //     // 3. تشغيل الويب سيرفر
// //     initWebServer(&server);
// // }

// // void loop() {
// //     // الكود الرئيسي
// //     delay(1000); 
// // }

// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// // #include <WiFi.h>
// // #include <WiFiManager.h> // [اضافة هامة] استدعاء مكتبة مدير الواي فاي
// // #include "config.h"
// // #include "sensors.h"
// // #include "webserver.h"

// // AsyncWebServer server(SERVER_PORT);

// // void setup() {
// //     Serial.begin(BAUD_RATE);
    
// //     // 1. تشغيل الحساسات
// //     initSensors();

// //     // 2. إعداد WiFiManager
// //     WiFiManager wifiManager;

// //     // (اختياري) مسح الإعدادات القديمة للتجربة - قم بتفعيل السطر التالي إذا أردت إجبار الجهاز على دخول وضع الإعداد
// //     // wifiManager.resetSettings();

// //     // رسالة توضيحية في السيريال
// //     Serial.println("Starting WiFiManager...");
    
// //     // هذا السطر يقوم بكل السحر!
// //     // سيحاول الاتصال بآخر شبكة محفوظة.
// //     // إذا فشل، سيقوم بإنشاء نقطة وصول (AP) باسم "Zizo-Project-AP"
// //     // يمكنك الدخول عليها من الموبايل لضبط الإعدادات
// //     bool res = wifiManager.autoConnect("Zizo-Project-AP", "password123"); // الاسم والباسورد للشبكة المؤقتة

// //     if(!res) {
// //         Serial.println("Failed to connect or hit timeout");
// //         // إعادة تشغيل الجهاز في حال الفشل (أحياناً تكون مفيدة)
// //         ESP.restart();
// //     } 
    
// //     // إذا وصل الكود هنا، فهذا يعني أن الاتصال تم بنجاح
// //     Serial.println("\nConnected to WiFi via WiFiManager! :)");
// //     Serial.print("IP Address: ");
// //     Serial.println(WiFi.localIP());

// //     // 3. تشغيل الويب سيرفر الخاص بالمشروع
// //     initWebServer(&server);
// // }

// // void loop() {
// //     // الكود هنا يبقى كما هو
// //     // الويب سيرفر يعمل بشكل غير متزامن (Asynchronous)
// //     delay(1000); 
// // }






