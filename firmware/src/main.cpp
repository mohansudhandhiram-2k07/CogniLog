#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <Preferences.h> // <--- NEW: Permanent Storage
#include <time.h>        // <--- NEW: Core C time library
#include <WiFiUdp.h>
#include "driver/rtc_io.h"

// --- WIFI & TIME CONFIG ---
const char *ssid = "vivo Y56 5G";
const char *password = "12345678";

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 19800; // +5:30 for India
const int daylightOffset_sec = 0; // No DST in India
WiFiUDP udp;
unsigned int localUdpPort = 4210;
char incomingPacket[255];
bool pcStatusFocus = true;
// --- PINS ---
#define PIN_BTN_UP 18
#define PIN_BTN_DOWN 19
#define PIN_BTN_SELECT 27
#define PIN_LED 4 // Blue Resistor!
#define PIN_BUZZER 15
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
//SCL - 22
//SDA - 21

// --- CONFIG ---
#define DISTRACTION_LIMIT 3
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
// --- STATES ---
enum SystemState
{
    STATE_MENU,
    STATE_LOGGING,
    STATE_WIFI_SCAN,
    STATE_ALARM
};

// --- GLOBALS ---
Preferences preferences; // Create storage object
SystemState currentState = STATE_MENU;
int currentSelection = 0;
unsigned long lastDebounce = 0;
unsigned long lastCheckInTime = 0;
unsigned long nextCheckInInterval = 0;
bool waitingForCheckIn = false;
unsigned long checkInPromptTime = 0;
unsigned long lastClockUpdate = 0;
unsigned long lastActivityTime = 0; // Track the last time a button was touched

// Session Vars
unsigned long sessionStartTime = 0;
unsigned long totalFocusTime = 0; // Saved in Flash
int distractionCounter = 0;
bool isDistracted = false;

void initWiFi()
{
    Serial.print("Connecting to WiFi: ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);

    // Timeout loop for industry-grade robustness
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 20)
    {
        delay(500);
        Serial.print(".");
        retries++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("\n[WIFI]: Connected!");
        // Sync Time
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        Serial.println("[TIME]: Syncing with NTP Server...");
    }
    else
    {
        Serial.println("\n[WIFI]: Failed to connect. Operating offline.");
        WiFi.disconnect(); // Don't let it hang searching
    }
}
void printLocalTime()
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        Serial.println("Time not set/syncing...");
        return;
    }
    // Print formatted time: HH:MM:SS
    Serial.printf("Current Time: %02d:%02d:%02d\n",
                  timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
}
void dumpStoredData()
{
    // Visual Signal: Turn LED ON to show "Busy"
    digitalWrite(PIN_LED, HIGH);

    preferences.begin("cognilog", true);
    int totalSessions = preferences.getInt("session_count", 0);

    // Start Chirp
    digitalWrite(PIN_BUZZER, HIGH);
    delay(100);
    digitalWrite(PIN_BUZZER, LOW);

    for (int i = 0; i < totalSessions; i++)
    {
        char key[15];
        sprintf(key, "sess_%d", i);
        String data = preferences.getString(key, "N/A");
        Serial.printf("%d,%s\n", i, data.c_str());
        delay(50); // Small delay to prevent serial buffer overflow
    }

    preferences.end();

    // End Signal: Blink LED 3 times and turn OFF
    for (int i = 0; i < 3; i++)
    {
        digitalWrite(PIN_LED, LOW);
        delay(100);
        digitalWrite(PIN_LED, HIGH);
        delay(100);
    }
    digitalWrite(PIN_LED, LOW);
}
// --- STORAGE HELPER ---
void loadStats()
{
    preferences.begin("cognilog", false);                   // Namespace "cognilog", RW mode
    totalFocusTime = preferences.getULong("focus_time", 0); // Default 0
    preferences.end();
}
void saveStats(unsigned long sessionSeconds)
{
    preferences.begin("cognilog", false);

    // 1. Update Lifetime Total
    unsigned long currentTotal = preferences.getULong("focus_time", 0);
    preferences.putULong("focus_time", currentTotal + sessionSeconds);
    totalFocusTime = currentTotal + sessionSeconds;

    // 2. NEW: Save the actual session record for the Harvester
    int sessionID = preferences.getInt("session_count", 0);
    char key[15];
    sprintf(key, "sess_%d", sessionID); // Creates keys like sess_0, sess_1

    // Get current Unix Time from NTP
    time_t now;
    time(&now);

    // Save as CSV string: "Timestamp,Duration"
    char dataString[32];
    sprintf(dataString, "%ld,%lu", (long)now, sessionSeconds);

    preferences.putString(key, dataString);
    preferences.putInt("session_count", sessionID + 1); // Increment for next time

    preferences.end();
    Serial.printf("\n[STORAGE]: Saved Session %d to Flash.\n", sessionID);
}
void enterDeepSleep()
{
    Serial.println("[POWER]: Deep Sleep Entry...");
    display.clearDisplay();
    display.setCursor(20, 30);
    display.println("SLEEPING...");
    display.display();
    delay(500);

    display.ssd1306_command(SSD1306_DISPLAYOFF);

    // --- HARDWARE LOCK FOR GPIO 27 ---
    // Use the specific GPIO_NUM matching your physical wiring
    rtc_gpio_init(GPIO_NUM_27);
    rtc_gpio_set_direction(GPIO_NUM_27, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pullup_en(GPIO_NUM_27); // Keep it HIGH
    rtc_gpio_pulldown_dis(GPIO_NUM_27);

    // Arm wakeup on GPIO 27
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_27, 0);

    esp_deep_sleep_start();
}

// --- HELPER: MOCK AI ---
bool getAttentionScore() { return (random(0, 100) > 20); }

// --- HELPER: DISPLAY ---
void updateDisplay()
{
    // Safety check: If the display hasn't been initialized, stop here to prevent crash
    if (!&display)
        return;

    display.clearDisplay();
    display.setCursor(0, 0);

    if (currentState == STATE_MENU)
    {
        display.println("--- COGNILOG V1.0 ---");

        struct tm timeinfo;
        if (getLocalTime(&timeinfo))
        {
            display.printf("Time: %02d:%02d:%02d\n", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        }
        display.println("---------------------");

        const char *menuItems[] = {"START SESSION", "WIFI SETUP", "RESET STATS", "DATA DUMP", "DEEP SLEEP"}; // Updated to 5 items
        for (int i = 0; i < 5; i++)
        { // Change loop to 5
            display.print(i == currentSelection ? "> " : "  ");
            display.println(menuItems[i]);
        }
    }
    else if (currentState == STATE_LOGGING)
    {
        unsigned long duration = (millis() - sessionStartTime) / 1000;
        display.println("[ SESSION ACTIVE ]");
        display.drawFastHLine(0, 10, 128, SSD1306_WHITE);

        display.setCursor(0, 20);
        display.printf("Time: %lus\n", duration);

        if (waitingForCheckIn)
        {
            // --- NEW: COUNTDOWN UI ---
            display.setCursor(0, 35);
            display.println("STILL THERE? (SEL)");

            // Calculate remaining time (10s window)
            long remaining = 10000 - (millis() - checkInPromptTime);
            if (remaining < 0)
                remaining = 0;

            // Map 10000ms to 128 pixels (screen width)
            int barWidth = map(remaining, 0, 10000, 0, 128);

            // Draw the bar
            display.fillRect(0, 50, barWidth, 8, SSD1306_WHITE);
            display.drawRect(0, 50, 128, 8, SSD1306_WHITE);
        }
        else
        {
            display.printf("Focus: YES\n");
            // Show a simple static progress line
            display.drawRect(0, 50, 128, 4, SSD1306_WHITE);
        }
    }
    else if (currentState == STATE_ALARM)
    {
        display.setTextSize(2);
        display.setCursor(10, 20);
        display.println("WAKE UP!");
        display.setTextSize(1);
        display.setCursor(10, 45);
        display.println("Press any button");
    }

    display.display();
}

void setup()
{
    Serial.begin(115200);
    // 1. MANDATORY: Initialize OLED hardware FIRST
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {
        Serial.println(F("OLED failed"));
        for (;;)
            ;
    }
    // Initialize OLED
    // --- Boot Animation ---
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    for (int i = 0; i < 128; i += 8)
    {
        display.clearDisplay();
        display.drawRect(0, 30, i, 8, SSD1306_WHITE); // Progress bar
        display.setCursor(30, 15);
        display.println("COGNILOG OS");
        display.display();
    }
    delay(500);

    // Hardware Init
    pinMode(PIN_BTN_UP, INPUT_PULLUP);
    pinMode(PIN_BTN_DOWN, INPUT_PULLUP);
    pinMode(PIN_BTN_SELECT, INPUT_PULLUP);
    pinMode(PIN_LED, OUTPUT);
    pinMode(PIN_BUZZER, OUTPUT);

    initWiFi();
    // After initWiFi();
    udp.begin(localUdpPort);
    Serial.printf("[UDP]: Listening on IP %s, Port %d\n", WiFi.localIP().toString().c_str(), localUdpPort);
    // Load Data
    loadStats();
    // Set first check-in to be between 10 and 20 mins from now (for testing, make it 10-20 seconds)
    nextCheckInInterval = random(10000, 20000); // 10s - 20s for DEMO only. Use minutes later.
    lastCheckInTime = millis();

    // Boot Chirp
    digitalWrite(PIN_BUZZER, HIGH);
    delay(50);
    digitalWrite(PIN_BUZZER, LOW);

    Serial.println("System Booted.");
    updateDisplay();
}

void loop()
{
    
    // 1. INPUT PHASE: Capture raw hardware signals
    bool bUp = !digitalRead(PIN_BTN_UP);
    bool bDown = !digitalRead(PIN_BTN_DOWN);
    bool bSel = !digitalRead(PIN_BTN_SELECT);
    bool shouldRefresh = false; // Flag to trigger OLED update
    // 1.5 UDP PHASE: Check for updates from Python Watchdog
    int packetSize = udp.parsePacket();
    if (packetSize)
    {
        int len = udp.read(incomingPacket, 255);
        if (len > 0)
            incomingPacket[len] = 0; // Null-terminate string

        String status = String(incomingPacket);
        pcStatusFocus = (status == "FOCUS");

        // If PC detects distraction while logging, trigger the alarm logic
        if (!pcStatusFocus && currentState == STATE_LOGGING && !waitingForCheckIn)
        {
            waitingForCheckIn = true;
            checkInPromptTime = millis();
            // Feedback
            digitalWrite(PIN_BUZZER, HIGH);
            delay(100);
            digitalWrite(PIN_BUZZER, LOW);
        }
    }

    // 2. BACKGROUND PHASE: Auto-update clock every second
    if (millis() - lastClockUpdate >= 1000)
    {
        lastClockUpdate = millis();
        shouldRefresh = true;
    }

    // 3. INPUT FILTERING: Centralized Software Debounce
    if (millis() - lastDebounce < 250)
    {
        bUp = bDown = bSel = false; // Ignore inputs during lockout
    }
    else if (bUp || bDown || bSel)
    {
        lastDebounce = millis();     // Valid press detected
        lastActivityTime = millis(); // Reset inactivity timer
        shouldRefresh = true;
    }

    // 4. LOGIC PHASE: The State Machine
    switch (currentState)
    {
    case STATE_MENU:
        // --- Automated Inactivity Check (Moved inside MENU to avoid blocking) ---
        if (millis() - lastActivityTime > 60000)
        {
            enterDeepSleep();
        }

        if (bUp)
        {
            currentSelection = (currentSelection <= 0) ? 4 : currentSelection - 1;
        }
        if (bDown)
        {
            currentSelection = (currentSelection >= 4) ? 0 : currentSelection + 1;
        }

        if (bSel)
        {
            if (currentSelection == 0) // START SESSION
            {
                currentState = STATE_LOGGING;
                sessionStartTime = millis();
                lastCheckInTime = millis();
                distractionCounter = 0;
            }
            else if (currentSelection == 1) // WIFI SETUP
            {
                initWiFi();
            }
            else if (currentSelection == 2) // RESET STATS
            {
                preferences.begin("cognilog", false);
                preferences.clear();
                preferences.end();
                totalFocusTime = 0;
            }
            else if (currentSelection == 3) // DATA DUMP
            {
                dumpStoredData();
            }
            else if (currentSelection == 4)
            {
                enterDeepSleep();
            }
        }
        break;

    case STATE_LOGGING:
        // --- 1. Reality Check Trigger ---
        if (!waitingForCheckIn && (millis() - lastCheckInTime > nextCheckInInterval))
        {
            waitingForCheckIn = true;
            checkInPromptTime = millis();
            digitalWrite(PIN_BUZZER, HIGH);
            delay(50);
            digitalWrite(PIN_BUZZER, LOW);
            shouldRefresh = true;
        }

        // --- 2. Handling Check-in Response ---
        if (waitingForCheckIn)
        {
            digitalWrite(PIN_LED, (millis() / 200) % 2 == 0); // Blink during prompt

            if (bSel) // User verifies focus
            {
                waitingForCheckIn = false;
                digitalWrite(PIN_LED, LOW);
                lastCheckInTime = millis();
                nextCheckInInterval = random(10000, 20000); // 10-20s for demo
            }
            else if (millis() - checkInPromptTime > 10000) // Timeout
            {
                currentState = STATE_ALARM;
                waitingForCheckIn = false;
                shouldRefresh = true;
            }
        }
        else if (bSel && !waitingForCheckIn)
        {
            saveStats((millis() - sessionStartTime) / 1000);
            currentState = STATE_MENU;
        }
        break;

    case STATE_ALARM:
        digitalWrite(PIN_LED, (millis() / 100) % 2 == 0);
        digitalWrite(PIN_BUZZER, (millis() / 100) % 2 == 0);

        if (bUp || bDown || bSel) // Any button silences alarm
        {
            digitalWrite(PIN_LED, LOW);
            digitalWrite(PIN_BUZZER, LOW);
            lastCheckInTime = millis();
            currentState = STATE_LOGGING;
        }
        break;
    }

    // 5. OUTPUT PHASE: Refresh display only if flag is set
    if (shouldRefresh)
    {
        updateDisplay();
    }
}