#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include "config.h"

void initSensors();

// دوال للمنطق (Logic & Telegram & Firebase)
float getRawTemperature();
float getRawHumidity();
bool isFlameDetected();

#endif