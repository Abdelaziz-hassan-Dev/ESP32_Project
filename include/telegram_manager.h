#ifndef TELEGRAM_MANAGER_H
#define TELEGRAM_MANAGER_H

#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include "config.h"

void initTelegram();

// Evaluates sensor data against thresholds and triggers alerts if needed
void checkSystemConditions(float temp, float hum, bool flame);

void sendTelegramMessage(String message);

#endif