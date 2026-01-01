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

// Updates the Dashboard UI (Throttled to prevent flooding)
void sendDataToFirebase(float t, float h, String flameStatus) {
    if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 2000 || sendDataPrevMillis == 0)) {
        sendDataPrevMillis = millis();

        FirebaseJson json;
        
        if (!isnan(t)) json.set("temperature", t);
        if (!isnan(h)) json.set("humidity", h);
        json.set("flame", flameStatus);

        // Use updateNode to overwrite the current state at "/sensor"
        Firebase.RTDB.updateNode(&fbdo, "/sensor", &json);
    }
}

// Helper to format timestamp for logs
String getFormattedTime() {
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
        return "Time Error";
    }
    char timeStringBuff[50];
    // Format: YYYY-MM-DD HH:MM:SS
    strftime(timeStringBuff, sizeof(timeStringBuff), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return String(timeStringBuff);
}

// Appends data to historical logs
void logHistoryToFirebase(float t, float h, String flameStatus) {
    if (Firebase.ready() && signupOK) {
        FirebaseJson json;
        
        if (!isnan(t)) json.set("temperature", t);
        if (!isnan(h)) json.set("humidity", h);
        json.set("flame", flameStatus);
        
        // Add server-side readable timestamp
        String timestamp = getFormattedTime();
        json.set("datetime", timestamp); 

        Serial.print("Appending to History (at " + timestamp + ")... ");
        
        // Use pushJSON to append a new child node to "/history"
        if (Firebase.RTDB.pushJSON(&fbdo, "/history", &json)) {
            Serial.println("Done!");
        } else {
            Serial.print("Error: ");
            Serial.println(fbdo.errorReason());
        }
    }
}