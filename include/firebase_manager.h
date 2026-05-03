#ifndef FIREBASE_MANAGER_H
#define FIREBASE_MANAGER_H

// =============================================================================
//  firebase_manager.h — Header لمدير Firebase
//  ─────────────────────────────────────────────
//  هذا الملف موجود في:  include/firebase_manager.h
//
//  التغييرات عن النسخة القديمة:
//  ──────────────────────────────
//  القديم:
//    • sendDataToFirebase() تُستدعى مباشرة من loop()
//    • logHistoryToFirebase() تُستدعى مباشرة من loop()
//    • signupOK متغير global عادي بدون حماية
//    • sendDataPrevMillis مرتبط بـ millis()
//
//  الجديد:
//    • vTaskFirebaseManager مهمة مستقلة تشتغل على Core 0
//    • تقرأ من xFirebaseQueue (real-time updates → /sensor)
//    • تقرأ من xHistoryQueue  (history triggers   → /history)
//    • signupOK أصبح BIT_FIREBASE_READY في xSystemEventGroup
//    • لا millis() — الـ pacing بواسطة vTaskDelay
//    • كل استدعاء Firebase محمي بـ xFirebaseMutex
// =============================================================================

#include <Arduino.h>
#include <Firebase_ESP_Client.h>
#include "config.h"
#include "rtos_shared.h"   // ← جديد: للوصول للـ queues والـ mutexes

// ── تهيئة Firebase ───────────────────────────────────────────────────────────
//  تُستدعى مرة واحدة في setup()
//  ترفع BIT_FIREBASE_READY في xSystemEventGroup بعد signUp ناجح
void initFirebase();

// ── المهمة الرئيسية ──────────────────────────────────────────────────────────
//  تُسجَّل في main.cpp عبر xTaskCreatePinnedToCore()
//  Core 0، Priority 4، Stack 12 KB
//
//  مسؤولياتها:
//  1. تنتظر BIT_FIREBASE_READY قبل أي كتابة
//  2. تستقبل SensorData_t من xFirebaseQueue → updateNode("/sensor")
//  3. تستقبل trigger byte من xHistoryQueue  → pushJSON("/history")
void vTaskFirebaseManager(void* pvParameters);

#endif
