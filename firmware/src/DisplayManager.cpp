#include "DisplayManager.h"

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void initDisplay() {
    if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        Serial.println(F("SSD1306 failed"));
        for(;;);
    }
    display.setTextColor(SSD1306_WHITE);
}

void showBootAnimation() {
    for (int i = 0; i < 128; i += 8) {
        display.clearDisplay();
        display.drawRect(0, 30, i, 8, SSD1306_WHITE);
        display.setCursor(30, 15);
        display.println("COGNILOG OS");
        display.display();
    }
}

void updateDisplay(SystemState state, int selection, bool waiting, unsigned long sessionSec, int barPercent) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);

    if (state == STATE_MENU) {
        display.println("--- COGNILOG V1.0 ---");
        const char *menuItems[] = {"START SESSION", "WIFI SETUP", "RESET STATS", "DATA DUMP", "DEEP SLEEP"};
        for (int i = 0; i < 5; i++) {
            display.print(i == selection ? "> " : "  ");
            display.println(menuItems[i]);
        }
    } else if (state == STATE_LOGGING) {
        display.printf("Time: %lus\n", sessionSec); // No math here anymore!

        if (waiting) {
        int barWidth = map(barPercent, 0, 100, 0, 128); 
        display.fillRect(0, 50, barWidth, 8, SSD1306_WHITE);
    }
    }
    display.display();
}