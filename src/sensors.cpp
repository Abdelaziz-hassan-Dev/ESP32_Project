#include "sensors.h"

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

String readTemperature() {
    float t = dhtSensor.readTemperature();
    return isnan(t) ? "--" : String(t);
}

String readHumidity() {
    float h = dhtSensor.readHumidity();
    return isnan(h) ? "--" : String(h);
}

String readFlameStatus() {
    int sensorState = digitalRead(FLAME_PIN);
    return (sensorState == LOW) ? "DETECTED! 🔥" : "Safe ✅";
}

DHT* getDHTInstance() {
    return &dhtSensor;
}

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