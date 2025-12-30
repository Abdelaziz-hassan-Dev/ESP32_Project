#ifndef FIREBASE_MANAGER_H
#define FIREBASE_MANAGER_H

#include <Arduino.h>
#include <Firebase_ESP_Client.h>
#include "config.h"

// تعريف الدوال فقط دون تضمين الـ Addons هنا
void initFirebase();
void sendDataToFirebase(float temp, float hum, String flameStatus);

#endif