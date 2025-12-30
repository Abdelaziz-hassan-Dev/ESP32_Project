#include "sensor_manager.h"

// Global sensor instances
static DHT dhtSensor(DHTPIN, DHTTYPE);
static bool sensorsInitialized = false;

void initSensors() {
    if (!sensorsInitialized) {
        pinMode(FLAME_PIN, INPUT);
        dhtSensor.begin();
        sensorsInitialized = true;
    }
}

// الدوال المستخدمة للمنطق وإرسال البيانات (Logic & Firebase)
float getRawTemperature() {
    return dhtSensor.readTemperature();
}

float getRawHumidity() {
    return dhtSensor.readHumidity();
}

bool isFlameDetected() {
    // المنطق: LOW يعني تم اكتشاف الحريق في معظم الحساسات
    return (digitalRead(FLAME_PIN) == LOW); 
}
