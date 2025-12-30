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

    // دالة tokenStatusCallback تأتي من TokenHelper وتعمل الآن بشكل صحيح
    if (Firebase.signUp(&config, &auth, "", "")) {
        Serial.println("Firebase Connected");
        signupOK = true;
    } else {
        Serial.printf("%s\n", config.signer.signupError.message.c_str());
    }
    
    // دالة المساعدة للطباعة
    config.token_status_callback = tokenStatusCallback;

    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
}

void sendDataToFirebase(float t, float h, String flameStatus) {
    if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 2000 || sendDataPrevMillis == 0)) {
        sendDataPrevMillis = millis();

        if (!isnan(t)) Firebase.RTDB.setFloat(&fbdo, "/sensor/temperature", t);
        if (!isnan(h)) Firebase.RTDB.setFloat(&fbdo, "/sensor/humidity", h);
        Firebase.RTDB.setString(&fbdo, "/sensor/flame", flameStatus);
    }
}