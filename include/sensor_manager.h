#ifndef SENSOR_MANAGER_H  
#define SENSOR_MANAGER_H  

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include "config.h"

void initSensors();

// دوال للعرض (Web Server)
String readTemperature();
String readHumidity();
String readFlameStatus();

// دوال للمنطق (Logic & Telegram) - [جديد]
float getRawTemperature();
float getRawHumidity();
bool isFlameDetected();

#endif