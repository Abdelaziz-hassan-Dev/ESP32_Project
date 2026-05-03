// include/sensor_manager.h
#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include "config.h"
#include "rtos_shared.h"   // ← جديد: SensorData_t + Queue handles

// تهيئة الحساسات — تُستدعى مرة واحدة في setup()
void initSensors();

// المهمة الجديدة — تُسجَّل في main.cpp عبر xTaskCreatePinnedToCore()
// Core 1 ، Priority 5
void vTaskSensorRead(void* pvParameters);

// ===================================================
// الدوال القديمة حُذفت:
//   float getRawTemperature()  ← أصبحت جزءاً من vTaskSensorRead
//   float getRawHumidity()     ← أصبحت جزءاً من vTaskSensorRead
//   bool  isFlameDetected()    ← أصبحت جزءاً من vTaskSensorRead
// ===================================================

#endif