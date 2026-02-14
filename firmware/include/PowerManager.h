#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <Arduino.h>
#include "driver/rtc_io.h"
#include <Adafruit_SSD1306.h>

void enterDeepSleep(Adafruit_SSD1306 &display);
#endif