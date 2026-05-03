#ifndef WIFI_WATCHDOG_H
#define WIFI_WATCHDOG_H

// =============================================================================
//  wifi_watchdog.h — Header للـ WiFi Watchdog Task
//  ──────────────────────────────────────────────────
//  هذا الملف موجود في:  include/wifi_watchdog.h
//
//  المسؤوليات الكاملة لهذه المهمة:
//  1. مراقبة حالة WiFi كل 5 ثواني
//  2. إعادة الاتصال تلقائياً لو انقطع
//  3. انتظار مزامنة NTP ورفع BIT_NTP_SYNCED
//  4. تحديث BIT_WIFI_CONNECTED في xSystemEventGroup
//  5. حماية المهام الأخرى من الكتابة للـ cloud وهي offline
// =============================================================================

#include <Arduino.h>
#include <WiFi.h>
#include "time.h"
#include "config.h"
#include "rtos_shared.h"

// ── الدالة الوحيدة المُصدَّرة ─────────────────────────────────────────────
// تُسجَّل في main.cpp عبر xTaskCreatePinnedToCore()
// Core 0، Priority 3، Stack 3 KB
void vTaskWiFiWatchdog(void* pvParameters);

#endif
