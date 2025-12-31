#include "firebase_manager.h"
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

// تعريف الكائنات
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

bool signupOK = false;
unsigned long sendDataPrevMillis = 0;

void initFirebase() {
    config.api_key = API_KEY;
    config.database_url = DATABASE_URL;

    if (Firebase.signUp(&config, &auth, "", "")) {
        Serial.println("Firebase Connected");
        signupOK = true;
    } else {
        Serial.printf("%s\n", config.signer.signupError.message.c_str());
    }
    
    config.token_status_callback = tokenStatusCallback;

    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
}

// دالة التحديث اللحظي (تبقى كما هي لسرعة الشاشة)
void sendDataToFirebase(float t, float h, String flameStatus) {
    if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 2000 || sendDataPrevMillis == 0)) {
        sendDataPrevMillis = millis();

        if (!isnan(t)) Firebase.RTDB.setFloat(&fbdo, "/sensor/temperature", t);
        if (!isnan(h)) Firebase.RTDB.setFloat(&fbdo, "/sensor/humidity", h);
        Firebase.RTDB.setString(&fbdo, "/sensor/flame", flameStatus);
    }
}

// [جديد] دالة الأرشفة (بديل Google Sheets)
void logHistoryToFirebase(float t, float h, String flameStatus) {
    if (Firebase.ready() && signupOK) {
        FirebaseJson json;
        
        // تجهيز البيانات
        if (!isnan(t)) json.set("temperature", t);
        if (!isnan(h)) json.set("humidity", h);
        json.set("flame", flameStatus);
        
        // إضافة طابع زمني من السيرفر مباشرة
        // هذا سيحفظ الوقت بصيغة Epoch Time (مللي ثانية)
        json.set("timestamp", "timestamp"); 
        // ملاحظة: في مكتبة Firebase ESP Client الحديثة، كلمة "timestamp" كقيمة نصية داخل json.set يتم تحويلها لـ ServerValue.TIMESTAMP تلقائياً
        
        // استخدام push بدلاً من set لإنشاء سجل جديد وعدم مسح القديم
        Serial.print("Appending data to History... ");
        if (Firebase.RTDB.pushJSON(&fbdo, "/history", &json)) {
            Serial.println("Done!");
        } else {
            Serial.print("Failed: ");
            Serial.println(fbdo.errorReason());
        }
    }
}