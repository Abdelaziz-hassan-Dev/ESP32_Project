#ifndef TELEGRAM_MANAGER_H
#define TELEGRAM_MANAGER_H

#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include "config.h"
#include "rtos_shared.h"   // ← جديد

void initTelegram();

// المهمة الجديدة — تُسجَّل في main.cpp
void vTaskAlertEngine(void* pvParameters);

// checkSystemConditions حُذفت — المهمة تقرأ من Queue مباشرة

#endif