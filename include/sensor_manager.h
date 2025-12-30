#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include "config.h"

void initSensors();

float getRawTemperature();
float getRawHumidity();
bool isFlameDetected();

#endif