#include <Arduino.h>
#include "Config.h"
#include "DisplayManager.h"
#include "StorageManager.h"
#include "NetworkManager.h"
#include "PowerManager.h"

// --- GLOBALS ---
SystemState currentState = STATE_MENU;
int currentSelection = 0;
unsigned long lastDebounce = 0;
unsigned long lastActivityTime = 0;
unsigned long sessionStartTime = 0;
unsigned long lastCheckInTime = 0;
unsigned long nextCheckInInterval = 0;
bool waitingForCheckIn = false;
unsigned long checkInPromptTime = 0;
bool pcFocus = true;
unsigned long lastClockUpdate = 0;

// ELITE FIX: Missing Global for Async Buzzer
unsigned long buzzerEndTime = 0; 

extern Adafruit_SSD1306 display;

// Function Prototype
void triggerChirp(unsigned long duration);

void setup() {
    Serial.begin(115200);
    randomSeed(analogRead(34) + micros());

    pinMode(PIN_BTN_UP, INPUT_PULLUP);
    pinMode(PIN_BTN_DOWN, INPUT_PULLUP);
    pinMode(PIN_BTN_SELECT, INPUT_PULLUP);
    pinMode(PIN_LED, OUTPUT);
    pinMode(PIN_BUZZER, OUTPUT);

    initDisplay();
    showBootAnimation();
    initStorage();
    initWiFi();

    triggerChirp(50); // Non-blocking boot chirp
    
    nextCheckInInterval = random(10000, 20000); 
    lastActivityTime = millis();
}

void loop() {
    
    // ASYNC BUZZER HANDLER
    if (buzzerEndTime != 0 && millis() > buzzerEndTime) {
        digitalWrite(PIN_BUZZER, LOW);
        buzzerEndTime = 0;
    }

    bool bUp = !digitalRead(PIN_BTN_UP);
    bool bDown = !digitalRead(PIN_BTN_DOWN);
    bool bSel = !digitalRead(PIN_BTN_SELECT);
    bool shouldRefresh = false;

    if (checkUDP(pcFocus)) {
        if (!pcFocus && currentState == STATE_LOGGING && !waitingForCheckIn) {
            waitingForCheckIn = true;
            checkInPromptTime = millis();
            triggerChirp(150);
            shouldRefresh = true;
        }
    }

    if (millis() - lastDebounce > 250 && (bUp || bDown || bSel)) {
        lastDebounce = millis();
        lastActivityTime = millis();
        shouldRefresh = true;
    } else {
        bUp = bDown = bSel = false;
    }

    switch (currentState) {
        case STATE_MENU:
            if (millis() - lastActivityTime > 60000) enterDeepSleep(display);
            if (bUp) currentSelection = (currentSelection <= 0) ? 4 : currentSelection - 1;
            if (bDown) currentSelection = (currentSelection >= 4) ? 0 : currentSelection + 1;
            if (bSel) {
                if (currentSelection == 0) { currentState = STATE_LOGGING; sessionStartTime = millis(); lastCheckInTime = millis(); }
                else if (currentSelection == 1) { initWiFi(); }
                else if (currentSelection == 2) { clearAllData(); }
                else if (currentSelection == 3) { dumpAllSessions(); triggerChirp(100); }
                else if (currentSelection == 4) { enterDeepSleep(display); }
            }
            break;

        case STATE_LOGGING:
            if (waitingForCheckIn) {
                digitalWrite(PIN_LED, (millis() / 200) % 2 == 0);
                if (bSel) {
                    waitingForCheckIn = false;
                    digitalWrite(PIN_LED, LOW);
                    lastCheckInTime = millis();
                    nextCheckInInterval = random(10000, 20000); 
                } else if (millis() - checkInPromptTime > 10000) {
                    currentState = STATE_ALARM;
                    waitingForCheckIn = false;
                }
            } else {
                if (bSel) { saveSession((millis() - sessionStartTime) / 1000); currentState = STATE_MENU; }
                if (millis() - lastCheckInTime > nextCheckInInterval) {
                    waitingForCheckIn = true;
                    checkInPromptTime = millis();
                    triggerChirp(50); // ELITE FIX: No delay(50) here anymore!
                }
            }
            break;

        case STATE_ALARM:
            digitalWrite(PIN_LED, (millis() / 100) % 2 == 0);
            digitalWrite(PIN_BUZZER, (millis() / 100) % 2 == 0);
            if (bUp || bDown || bSel) {
    digitalWrite(PIN_LED, LOW);
    digitalWrite(PIN_BUZZER, LOW);
    currentState = STATE_LOGGING;
    
    // ELITE FIX: Reset both timing AND randomized interval
    lastCheckInTime = millis();
    nextCheckInInterval = random(10000, 20000); 
}
            break;
    }

    // --- 1. UNIFIED HARDWARE CONTROL ---
    if (currentState == STATE_ALARM) {
        digitalWrite(PIN_BUZZER, (millis() / 100) % 2 == 0); // Alarm pattern
    } else if (buzzerEndTime != 0) {
        if (millis() > buzzerEndTime) {
            digitalWrite(PIN_BUZZER, LOW);
            buzzerEndTime = 0;
        } else {
            digitalWrite(PIN_BUZZER, HIGH); // Async Chirp
        }
    } else {
        digitalWrite(PIN_BUZZER, LOW);
    }

    // --- 2. DATA PREPARATION (The Math) ---
    unsigned long sessionSec = (currentState == STATE_LOGGING) ? (millis() - sessionStartTime) / 1000 : 0;
    int barPercent = 0;
    if (waitingForCheckIn) {
        long elapsed = millis() - checkInPromptTime;
        // Drain from 100 down to 0 over 10 seconds
        barPercent = 100 - constrain(map(elapsed, 0, 10000, 0, 100), 0, 100); 
    }

    // --- 3. REFRESH PHASE ---
    if (shouldRefresh || (millis() - lastClockUpdate >= REFRESH_INTERVAL)) {
        lastClockUpdate = millis();
        updateDisplay(currentState, currentSelection, waitingForCheckIn, sessionSec, barPercent);
        shouldRefresh = false; 
    }
}

void triggerChirp(unsigned long duration) {
    digitalWrite(PIN_BUZZER, HIGH);
    buzzerEndTime = millis() + duration;
}