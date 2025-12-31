#ifndef FIREBASE_MANAGER_H
#define FIREBASE_MANAGER_H

#include <Arduino.h>
#include <Firebase_ESP_Client.h>
#include "config.h"

// تعريف الدوال
void initFirebase();

// الدالة القديمة: للتحديث اللحظي (للشاشة)
void sendDataToFirebase(float temp, float hum, String flameStatus);

// [جديد] الدالة الجديدة: للحفظ التاريخي (بديل Google Sheets)
void logHistoryToFirebase(float temp, float hum, String flameStatus);

#endif