#include "rtos_shared.h"

// ============================================================
// التعريف الفعلي للمتغيرات
// (الملف .h يعلن عنها، الملف .cpp يعرّفها ويخصص الذاكرة)
// ============================================================

QueueHandle_t xFirebaseQueue  = NULL;
QueueHandle_t xAlertQueue     = NULL;
QueueHandle_t xHistoryQueue   = NULL;

SemaphoreHandle_t xSensorMutex   = NULL;
SemaphoreHandle_t xWiFiMutex     = NULL;
SemaphoreHandle_t xFirebaseMutex = NULL;

EventGroupHandle_t xSystemEventGroup = NULL;

TimerHandle_t xLogTimer = NULL;

//volatile SensorData_t g_latestSensorData = {0};
SensorData_t g_latestSensorData = {};
// ============================================================
// دالة التهيئة — تُنشئ كل شيء قبل بدء المهام
// ============================================================
void xRTOS_Init() {

    // -- إنشاء الـ Queues --
    // xQueueCreate(عدد العناصر, حجم العنصر الواحد بالبايت)
    xFirebaseQueue = xQueueCreate(5, sizeof(SensorData_t));
    xAlertQueue    = xQueueCreate(5, sizeof(SensorData_t));
    xHistoryQueue  = xQueueCreate(3, sizeof(uint8_t));

    // -- إنشاء الـ Mutexes --
    xSensorMutex   = xSemaphoreCreateMutex();
    xWiFiMutex     = xSemaphoreCreateMutex();
    xFirebaseMutex = xSemaphoreCreateMutex();

    // -- إنشاء الـ Event Group --
    xSystemEventGroup = xEventGroupCreate();

    // للتأكد إن كل شيء اتنشأ بنجاح
    configASSERT(xFirebaseQueue);
    configASSERT(xAlertQueue);
    configASSERT(xHistoryQueue);
    configASSERT(xSensorMutex);
    configASSERT(xWiFiMutex);
    configASSERT(xFirebaseMutex);
    configASSERT(xSystemEventGroup);

    // -- الـ Timer يُنشأ في setup() بعد ما نعرف LOG_INTERVAL --
    // (سيُنشأ في main.cpp لأنه يحتاج callback function)
}