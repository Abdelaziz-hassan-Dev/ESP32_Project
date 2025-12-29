#include "cloud_manager.h"
// نحتاج شهادة SSL أو نتجاهلها (الأسهل للمشاريع الطلابية التجاهل)
WiFiClientSecure secureClient;

void initCloud() {
    secureClient.setInsecure(); // السماح بالاتصال بجوجل دون التحقق من الشهادة المعقدة
}

void logDataToGoogleSheet(float temp, float hum, bool flameStatus) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi Disconnected! Cannot send to Google Sheets.");
        return;
    }

    HTTPClient http;
    
    // تجهيز الرابط مع البيانات
    // الشكل النهائي: URL?temp=25.5&hum=60.2&fire=Safe
    String url = String(G_SCRIPT_URL) + "?temp=" + String(temp, 1) + 
                 "&hum=" + String(hum, 1) + 
                 "&fire=" + (flameStatus ? "FIRE!" : "Safe");

    Serial.print("Sending data to Google Sheets...");
    
    // بدء الاتصال
    http.begin(secureClient, url); // استخدام secureClient مهم جداً لأن جوجل https
    
    // إرسال الطلب ومتابعة إعادة التوجيه (Redirection) لأن جوجل يقوم بذلك
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    
    int httpCode = http.GET(); // نستخدم GET لأننا أعددنا doGet في السكربت
    
    if (httpCode > 0) {
        // تم استلام رد
        String payload = http.getString();
        Serial.println("Done. Response: " + payload);
    } else {
        Serial.println("Failed. Error: " + http.errorToString(httpCode));
    }
    
    http.end();
}