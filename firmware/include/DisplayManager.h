#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Config.h"

// Define the system states here so DisplayManager knows what to draw
enum SystemState {
    STATE_MENU,
    STATE_LOGGING,
    STATE_WIFI_SCAN,
    STATE_ALARM
};
// selection, waiting status, and pre-computed values for session/bar
void updateDisplay(SystemState state, int selection, bool waiting, unsigned long sessionSec, int barPercent);
void initDisplay();
void showBootAnimation();
void displaySleepMsg();

#endif