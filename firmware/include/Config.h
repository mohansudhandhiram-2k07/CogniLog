#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// --- WIFI & TIME ---
extern const char *ssid;
extern const char *password;
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

// --- MIC PINS (I2S) ---
#define I2S_WS 25
#define I2S_SCK 26
#define I2S_SD 33
#define I2S_PORT I2S_NUM_0
#define SAMPLE_RATE 16000 // 16kHz is the standard for TinyML voice models
#define SAMPLES_PER_BUFFER 512 // Standard chunk size for audio processing

// --- OLED CONFIG ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDR 0x3C

#endif