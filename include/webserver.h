#ifndef MY_CUSTOM_WEBSERVER_H  // <--- غير الاسم هنا
#define MY_CUSTOM_WEBSERVER_H  // <--- وهنا أيضاً

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "web_page.h"
#include "config.h"

// Initialize web server
void initWebServer(AsyncWebServer* server);

// Processor for HTML templates
String processor(const String& var);
#endif