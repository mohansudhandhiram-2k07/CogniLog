#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h> // <--- NEW: Permanent Storage

// --- PINS ---
#define PIN_BTN_UP     18
#define PIN_BTN_DOWN   19
#define PIN_BTN_SELECT 5
#define PIN_LED        4   // Blue Resistor!
#define PIN_BUZZER     15

// --- CONFIG ---
#define DISTRACTION_LIMIT 3

// --- STATES ---
enum SystemState { STATE_MENU, STATE_LOGGING, STATE_WIFI_SCAN, STATE_ALARM };

// --- GLOBALS ---
Preferences preferences; // Create storage object
SystemState currentState = STATE_MENU;
int currentSelection = 0;
unsigned long lastDebounce = 0;

// Session Vars
unsigned long sessionStartTime = 0;
unsigned long totalFocusTime = 0; // Saved in Flash
int distractionCounter = 0;
bool isDistracted = false;

// --- STORAGE HELPER ---
void loadStats() {
    preferences.begin("cognilog", false); // Namespace "cognilog", RW mode
    totalFocusTime = preferences.getULong("focus_time", 0); // Default 0
    preferences.end();
}

void saveStats(unsigned long sessionSeconds) {
    preferences.begin("cognilog", false);
    unsigned long newTotal = totalFocusTime + sessionSeconds;
    preferences.putULong("focus_time", newTotal);
    totalFocusTime = newTotal; // Update RAM copy
    preferences.end();
    Serial.println("[STORAGE]: Session Saved to Flash Memory.");
}

// --- HELPER: MOCK AI ---
bool getAttentionScore() { return (random(0, 100) > 20); }

// --- HELPER: DISPLAY ---
void updateDisplay() {
    if (currentState == STATE_ALARM) return; 

    Serial.println("\n\n\n--- COGNILOG V1.0 ---");
    
    if (currentState == STATE_MENU) {
        Serial.println("   [ MAIN MENU ]");
        // Show Lifetime Stats
        Serial.print("   Lifetime Focus: "); 
        Serial.print(totalFocusTime / 60); 
        Serial.println(" mins");
        Serial.println("--------------------");
        
        const char* menuItems[] = { "START SESSION", "WIFI SETUP", "RESET STATS" };
        for (int i = 0; i < 3; i++) {
            Serial.print(i == currentSelection ? " > " : "   ");
            Serial.println(menuItems[i]);
        }
    } 
    else if (currentState == STATE_LOGGING) {
        unsigned long duration = (millis() - sessionStartTime) / 1000;
        Serial.print("   [ SESSION ACTIVE ]\n");
        Serial.print("   Time: "); Serial.print(duration); Serial.println("s");
        Serial.print("   Status: "); 
        if (isDistracted) Serial.println("[!] DISTRACTED");
        else              Serial.println("(:) FOCUSED");
        
        Serial.print("   Distraction Level: ");
        for(int i=0; i<distractionCounter; i++) Serial.print("X");
        Serial.println("\n\n   (Press SELECT to Save & Stop)");
    }
}

void setup() {
    Serial.begin(115200);
    
    // Hardware Init
    pinMode(PIN_BTN_UP, INPUT_PULLUP);
    pinMode(PIN_BTN_DOWN, INPUT_PULLUP);
    pinMode(PIN_BTN_SELECT, INPUT_PULLUP);
    pinMode(PIN_LED, OUTPUT);
    pinMode(PIN_BUZZER, OUTPUT);
    
    // Load Data
    loadStats();

    // Boot Chirp
    digitalWrite(PIN_BUZZER, HIGH); delay(50); digitalWrite(PIN_BUZZER, LOW);
    
    Serial.println("System Booted.");
    updateDisplay();
}

void loop() {
    bool bUp = !digitalRead(PIN_BTN_UP);
    bool bDown = !digitalRead(PIN_BTN_DOWN);
    bool bSel = !digitalRead(PIN_BTN_SELECT);

    if (millis() - lastDebounce < 200) return;

    switch (currentState) {
        case STATE_MENU:
            if (bUp) { currentSelection--; if (currentSelection < 0) currentSelection = 2; updateDisplay(); lastDebounce = millis(); }
            if (bDown) { currentSelection++; if (currentSelection > 2) currentSelection = 0; updateDisplay(); lastDebounce = millis(); }
            if (bSel) {
                lastDebounce = millis();
                if (currentSelection == 0) { // START
                    currentState = STATE_LOGGING;
                    sessionStartTime = millis();
                    distractionCounter = 0;
                    updateDisplay();
                } 
                else if (currentSelection == 1) { // WIFI
                     // (WiFi Code Hidden for brevity, focus on logic)
                     Serial.println("Scanning..."); delay(500); currentState = STATE_MENU; updateDisplay();
                }
                else if (currentSelection == 2) { // RESET STATS
                    preferences.begin("cognilog", false);
                    preferences.clear();
                    preferences.end();
                    totalFocusTime = 0;
                    Serial.println("Stats Wiped.");
                    updateDisplay();
                }
            }
            break;

        case STATE_LOGGING:
            if (bSel) { 
                // SAVE SESSION ON EXIT
                unsigned long duration = (millis() - sessionStartTime) / 1000;
                saveStats(duration);
                
                currentState = STATE_MENU;
                updateDisplay();
                lastDebounce = millis();
            }
            
            static unsigned long lastAiCheck = 0;
            if (millis() - lastAiCheck > 1000) {
                lastAiCheck = millis();
                bool focused = getAttentionScore();
                isDistracted = !focused;

                if (!focused) {
                    distractionCounter++;
                    digitalWrite(PIN_LED, HIGH); digitalWrite(PIN_BUZZER, HIGH);
                    delay(50);
                    digitalWrite(PIN_LED, LOW); digitalWrite(PIN_BUZZER, LOW);
                } else {
                    if (distractionCounter > 0) distractionCounter--; 
                }

                if (distractionCounter >= DISTRACTION_LIMIT) currentState = STATE_ALARM;
                updateDisplay(); 
            }
            break;

        case STATE_ALARM:
            Serial.println("!!! WAKE UP !!!");
            for(int i=0; i<3; i++) {
                digitalWrite(PIN_LED, HIGH); digitalWrite(PIN_BUZZER, HIGH); delay(100);
                digitalWrite(PIN_LED, LOW); digitalWrite(PIN_BUZZER, LOW); delay(100);
            }
            distractionCounter = 0; 
            currentState = STATE_LOGGING;
            lastDebounce = millis(); 
            break;
    }
}