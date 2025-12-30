#ifndef CONFIG_H
#define CONFIG_H

// WiFi Credentials
#define WIFI_SSID  "zizo_hz5"
#define WIFI_PASSWORD  "22722722"

// Telegram Credentials
#define BOT_TOKEN "8446300738:AAGOyDsyEAA4I7MdaPJQaMqyDJBTu-mPtnI"
#define CHAT_ID "273003214"

// Google Sheets Settings
// ملاحظة: انسخ الرابط الذي حصلت عليه من الـ Deploy كاملاً
const String GOOGLE_SCRIPT_ID = "AKfycbyK7Zs1vQiRN2KO69gd8DkRMhxsh5DcoZp9UPLT9XUXsm-ZgZt4jkQ1AJaVmTYiowlv";
#define G_SCRIPT_URL "https://script.google.com/macros/s/AKfycbyK7Zs1vQiRN2KO69gd8DkRMhxsh5DcoZp9UPLT9XUXsm-ZgZt4jkQ1AJaVmTYiowlv/exec"
#define LOG_INTERVAL 30000 // إرسال البيانات كل دقيقة (لعدم ملء الشيت بسرعة)

// Firebase Settings
#define API_KEY "AIzaSyDDdgCi-ZwiVJN9xIBd-BsopL8tWbnfZWo"
#define DATABASE_URL "https://esp32-ce491-default-rtdb.firebaseio.com"

// Alarm Thresholds (عتبات الإنذار)
#define TEMP_HIGH_LIMIT 40.0   // درجة الحرارة التي يبدأ عندها الخطر
#define HUM_HIGH_LIMIT 80.0    // نسبة الرطوبة التي يبدأ عندها الخطر

// Alert Cooldowns (فترة الراحة بين التنبيهات بالمللي ثانية)
// لمنع إرسال مئات الرسائل، ننتظر دقيقة مثلاً بين كل تنبيه حرارة
#define ALARM_COOLDOWN 60000 
#define FLAME_COOLDOWN 30000 // الحريق خطير، نرسل كل 30 ثانية إذا استمر

// Pin Definitions
#define DHTPIN 27
#define DHTTYPE DHT22
#define FLAME_PIN 14
#define BAUD_RATE 115200

#endif