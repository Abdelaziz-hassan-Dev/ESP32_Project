#ifndef FIREBASE_MANAGER_H
#define FIREBASE_MANAGER_H

#include <Arduino.h>
#include <Firebase_ESP_Client.h>
#include "config.h"

void initFirebase();

// Updates the real-time node for the dashboard (Overwrites previous value)
void sendDataToFirebase(float temp, float hum, String flameStatus);

// Appends a new entry to the historical log (Push operation)
void logHistoryToFirebase(float temp, float hum, String flameStatus);

#endif