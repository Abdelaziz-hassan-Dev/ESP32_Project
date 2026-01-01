#include "firebase_manager.h"
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#include <time.h> 

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

// دالة التحديث اللحظي (للشاشة) - لا تحتاج تاريخ
void sendDataToFirebase(float t, float h, String flameStatus) {
    // نرسل فقط إذا مر الوقت المحدد أو لم نرسل من قبل
    if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 2000 || sendDataPrevMillis == 0)) {
        sendDataPrevMillis = millis();

        // إنشاء كائن JSON لجمع البيانات وإرسالها مرة واحدة
        FirebaseJson json;
        
        if (!isnan(t)) json.set("temperature", t);
        if (!isnan(h)) json.set("humidity", h);
        json.set("flame", flameStatus);

        // استخدام updateNode بدلاً من set المتعددة
        // هذا يقلل وقت الاتصال بالإنترنت إلى الثلث
        if (!Firebase.RTDB.updateNode(&fbdo, "/sensor", &json)) {
            Serial.println(fbdo.errorReason());
        }
    }
}

// دالة مساعدة للحصول على الوقت الحالي كنص منسق
String getFormattedTime() {
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
        return "Time Error";
    }
    char timeStringBuff[50];
    // الصيغة: السنة-الشهر-اليوم الساعة:الدقيقة:الثانية
    // مثال: 2024-05-20 14:30:05
    strftime(timeStringBuff, sizeof(timeStringBuff), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return String(timeStringBuff);
}

// دالة الأرشفة الجديدة
void logHistoryToFirebase(float t, float h, String flameStatus) {
    if (Firebase.ready() && signupOK) {
        FirebaseJson json;
        
        // البيانات
        if (!isnan(t)) json.set("temperature", t);
        if (!isnan(h)) json.set("humidity", h);
        json.set("flame", flameStatus);
        
        // الزمن (أهم إضافة)
        // الآن سيرسل التاريخ كنص مقروء بدلاً من timestamp غامض
        String timestamp = getFormattedTime();
        json.set("datetime", timestamp); 

        Serial.print("Appending to History (at " + timestamp + ")... ");
        
        // استخدام push لحفظ السجل الجديد
        if (Firebase.RTDB.pushJSON(&fbdo, "/history", &json)) {
            Serial.println("Done!");
        } else {
            Serial.print("Error: ");
            Serial.println(fbdo.errorReason());
        }
    }
}