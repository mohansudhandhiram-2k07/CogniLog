#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// --- WIFI & TIME ---
const char *ssid = "vivo Y56 5G";
const char *password = "12345678";
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET_SEC 19800
#define DAYLIGHT_OFFSET_SEC 0
#define REFRESH_INTERVAL 1000

// --- PINS ---
#define PIN_BTN_UP 18
#define PIN_BTN_DOWN 19
#define PIN_BTN_SELECT 27
#define PIN_LED 4
#define PIN_BUZZER 15

// --- OLED CONFIG ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDR 0x3C

#endif