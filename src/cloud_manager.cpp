#include "cloud_manager.h"

WiFiClientSecure secureClient;

void initCloud() {
    // Allow insecure connection for Google Scripts (simplifies SSL handling)
    secureClient.setInsecure(); 
}

void logDataToGoogleSheet(float temp, float hum, bool flameStatus) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi Disconnected! Cannot send to Google Sheets.");
        return;
    }

    HTTPClient http;
    
    // Construct GET request URL with query parameters
    String url = String(G_SCRIPT_URL) + "?temp=" + String(temp, 1) + 
                 "&hum=" + String(hum, 1) + 
                 "&fire=" + (flameStatus ? "FIRE!" : "Safe");

    Serial.print("Sending data to Google Sheets...");
    
    http.begin(secureClient, url); 
    
    // Essential for Google Apps Script as it redirects requests
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    
    int httpCode = http.GET(); 
    
    if (httpCode > 0) {
        String payload = http.getString();
        Serial.println("Done. Response: " + payload);
    } else {
        Serial.println("Failed. Error: " + http.errorToString(httpCode));
    }
    
    http.end();
}