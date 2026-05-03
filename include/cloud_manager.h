#ifndef CLOUD_MANAGER_H
#define CLOUD_MANAGER_H

// =============================================================================
//  cloud_manager.h — Header لمدير Google Sheets
//  ───────────────────────────────────────────────
//  هذا الملف موجود في:  include/cloud_manager.h
//
//  المسؤولية الوحيدة لهذا الملف:
//  إرسال بيانات الحساسات إلى Google Sheets عبر HTTP GET
//  كـ backup خارجي للسجل التاريخي.
//
//  التغييرات عن النسخة القديمة:
//  ──────────────────────────────
//  القديم:
//    • logDataToGoogleSheet() تُستدعى مباشرة من loop()
//    • SSL handshake (~1-3 ثانية) يحجب قراءة الحساسات
//    • لو WiFi منقطع → Serial.println ويرجع، بدون إعادة محاولة
//    • لا حماية ضد التزامن مع Telegram (كلاهما يستخدم SSL)
//
//  الجديد:
//    • vTaskHistoryLogger مهمة مستقلة على Core 0، Priority 2
//    • تتوقف طول ما تريد في SSL بدون أن تؤثر على أي مهمة أخرى
//    • xWiFiMutex يضمن عدم التزامن مع Telegram (OQ-2 في الخطة)
//    • تنتظر BIT_WIFI_CONNECTED قبل كل محاولة إرسال
//    • تستخدم vTaskDelay(LOG_INTERVAL) للـ pacing — لا millis() ولا queue
//
//  لماذا vTaskDelay بدل queue؟
//  ──────────────────────────────
//  firebase_manager يقرأ من xHistoryQueue (للـ Firebase /history).
//  لو cloud_manager قرأ منه أيضاً → fan-out conflict: كل trigger
//  يُستهلك من قِبَل أحدهما فقط وليس الاثنين (§2.3 في الخطة).
//  الحل: مهمة Google Sheets تتحكم في وقتها بنفسها → صفر تعارض.
// =============================================================================

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "config.h"
#include "rtos_shared.h"   // ← جديد: للوصول للـ mutexes والـ event group

// ── تهيئة الـ secureClient ────────────────────────────────────────────────────
//  تُستدعى مرة واحدة في setup() قبل إطلاق المهام
void initCloud();

// ── المهمة الرئيسية ───────────────────────────────────────────────────────────
//  تُسجَّل في main.cpp عبر xTaskCreatePinnedToCore()
//  Core 0 | Priority 2 | Stack 12 KB
//
//  دورة حياة المهمة:
//  ──────────────────
//  [Boot] → انتظر استقرار النظام (تأخير أولي)
//         ↓
//  [Loop] → انتظر BIT_WIFI_CONNECTED
//         → اقرأ g_latestSensorData تحت xSensorMutex
//         → خذ xWiFiMutex
//         → أرسل HTTP GET إلى Google Sheets
//         → حرر xWiFiMutex
//         → نام LOG_INTERVAL ثانية
//         → كرر
void vTaskHistoryLogger(void* pvParameters);

#endif
