// include/config.h
#ifndef CONFIG_H
#define CONFIG_H

// 1. WiFi Credentials
#define WIFI_SSID     "YOUR_WIFI_NAME"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// 2. Telegram Credentials
#define BOT_TOKEN "YOUR_TELEGRAM_BOT_TOKEN"
#define CHAT_ID   "YOUR_TELEGRAM_CHAT_ID"

// 3. Google Sheets Settings
// Note: Copy the full Web App URL from Google Apps Script deployment
#define GOOGLE_SCRIPT_ID "YOUR_SCRIPT_ID" 
#define G_SCRIPT_URL     "YOUR_GOOGLE_SCRIPT_WEB_APP_URL"
#define LOG_INTERVAL     30000 // Log data every 30 seconds

// 4. Firebase Settings
#define API_KEY      "YOUR_FIREBASE_API_KEY"
#define DATABASE_URL "YOUR_FIREBASE_DATABASE_URL"

// 5. Alarm Thresholds
#define TEMP_HIGH_LIMIT 40.0   // Trigger alert if Temp > 40°C
#define HUM_HIGH_LIMIT  80.0   // Trigger alert if Humidity > 80%

// 6. Alert Cooldowns (in milliseconds)
// Prevents spamming alerts (e.g., wait 60s between temp alerts)
#define ALARM_COOLDOWN 60000 
#define FLAME_COOLDOWN 30000 // Fire is critical, alert more frequently (30s)

// 7. Pin Definitions & System
#define DHTPIN      27     // DHT Sensor connected to D27
#define DHTTYPE     DHT22  // Sensor type
#define FLAME_PIN   14     // Flame Sensor connected to D14
#define BAUD_RATE   115200 // Serial Monitor Speed

#endif