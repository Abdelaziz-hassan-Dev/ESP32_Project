#ifndef RTOS_SHARED_H
#define RTOS_SHARED_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/event_groups.h>
#include <freertos/timers.h>

// ============================================================
// 1. الحزمة — البيانات اللي تتنقل بين المهام
// ============================================================
// بدل ما كل مهمة تقرأ متغيرات globals منفصلة،
// المهمة اللي تقرأ الحساسات تحزم كل شي هنا وترسله
typedef struct {
    float      temperature;   // درجة الحرارة
    float      humidity;      // الرطوبة
    bool       flameDetected; // هل كُشف حريق؟
    TickType_t capturedAt;    // وقت القراءة بتكات FreeRTOS (بدل millis)
} SensorData_t;

// ============================================================
// 2. Queue Handles — قنوات التواصل بين المهام
// ============================================================
// extern يعني: "هذا المتغير معرّف في rtos_shared.cpp، مش هنا"
// كل مهمة تعلن extern لتقول "أنا عارف إنه موجود"

extern QueueHandle_t xFirebaseQueue;  // SensorData_t → مهمة Firebase
extern QueueHandle_t xAlertQueue;     // SensorData_t → مهمة التنبيهات
extern QueueHandle_t xHistoryQueue;   // uint8_t trigger → مهمة التسجيل التاريخي

// ============================================================
// 3. Mutex & Semaphore Handles — أقفال الحماية
// ============================================================
extern SemaphoreHandle_t xSensorMutex;   // يحمي g_latestSensorData
extern SemaphoreHandle_t xWiFiMutex;     // يحمي كل HTTP / SSL (Telegram + Sheets)
extern SemaphoreHandle_t xFirebaseMutex; // يحمي مكتبة Firebase (غير thread-safe)

// ============================================================
// 4. Event Group — أعلام الحالة
// ============================================================
extern EventGroupHandle_t xSystemEventGroup;

// كل bit يمثل حالة معينة في النظام
#define BIT_WIFI_CONNECTED  ( 1 << 0 )  // WiFi متصل؟
#define BIT_FIREBASE_READY  ( 1 << 1 )  // Firebase جاهز؟
#define BIT_NTP_SYNCED      ( 1 << 2 )  // الوقت متزامن؟
#define BIT_FIRE_DETECTED   ( 1 << 3 )  // كُشف حريق؟

// ============================================================
// 5. Software Timer — المؤقت الدوري
// ============================================================
extern TimerHandle_t xLogTimer; // ينطلق كل LOG_INTERVAL ثانية

// ============================================================
// 6. المتغير العام المشترك — آخر قراءة
// ============================================================
// volatile يقول للمترجم: "لا تخزّن هذا في register، اقرأه من الذاكرة دائماً"
// لأن أي مهمة ممكن تغيره في أي لحظة
extern volatile SensorData_t g_latestSensorData;

// ============================================================
// 7. دالة التهيئة — تُستدعى مرة واحدة في setup()
// ============================================================
void xRTOS_Init();

#endif