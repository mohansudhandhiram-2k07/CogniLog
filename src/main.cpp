#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <Preferences.h> // <--- NEW: Permanent Storage
#include <time.h>        // <--- NEW: Core C time library

// --- WIFI & TIME CONFIG ---
const char *ssid = "vivo Y56 5G";
const char *password = "12345678";

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 19800; // +5:30 for India
const int daylightOffset_sec = 0; // No DST in India
// --- PINS ---
#define PIN_BTN_UP 18
#define PIN_BTN_DOWN 19
#define PIN_BTN_SELECT 5
#define PIN_LED 4 // Blue Resistor!
#define PIN_BUZZER 15
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

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
void saveStats(unsigned long sessionSeconds) {
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



// --- HELPER: MOCK AI ---
bool getAttentionScore() { return (random(0, 100) > 20); }

// --- HELPER: DISPLAY ---
void updateDisplay() {
    display.clearDisplay();
    display.setCursor(0,0);

    if (currentState == STATE_MENU) {
        display.println("--- COGNILOG V1.0 ---");
        
        // Real-time from your NTP Sync
        struct tm timeinfo;
        if (getLocalTime(&timeinfo)) {
            display.printf("Time: %02d:%02d:%02d\n", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        }
        display.println("---------------------");

        const char *menuItems[] = {"START SESSION", "WIFI SETUP", "RESET STATS", "DATA DUMP"};
        for (int i = 0; i < 4; i++) {
            display.print(i == currentSelection ? "> " : "  ");
            display.println(menuItems[i]);
        }
    } 
    else if (currentState == STATE_LOGGING) {
        unsigned long duration = (millis() - sessionStartTime) / 1000;
        display.println("[ SESSION ACTIVE ]");
        display.drawFastHLine(0, 10, 128, SSD1306_WHITE);
        
        display.setCursor(0, 20);
        display.printf("Time: %lus\n", duration);
        display.printf("Focus: %s\n", isDistracted ? "NO" : "YES");
        
        // Visual Distraction Bar (TinyML Data Placeholder)
        display.setCursor(0, 45);
        display.print("Alerts: ");
        display.drawRect(50, 45, 70, 10, SSD1306_WHITE);
        display.fillRect(52, 47, (distractionCounter * 20), 6, SSD1306_WHITE);
    }
    
    display.display(); // Important: Push to screen
}

void setup()
{
    Serial.begin(115200);
    // Initialize OLED
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {
        Serial.println(F("OLED failed"));
        for (;;)
            ;
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.display();
    initWiFi();

    // Hardware Init
    pinMode(PIN_BTN_UP, INPUT_PULLUP);
    pinMode(PIN_BTN_DOWN, INPUT_PULLUP);
    pinMode(PIN_BTN_SELECT, INPUT_PULLUP);
    pinMode(PIN_LED, OUTPUT);
    pinMode(PIN_BUZZER, OUTPUT);

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
    bool bUp = !digitalRead(PIN_BTN_UP);
    bool bDown = !digitalRead(PIN_BTN_DOWN);
    bool bSel = !digitalRead(PIN_BTN_SELECT);

    if (millis() - lastDebounce < 200)
        return;

    switch (currentState)
    {
    case STATE_MENU:
        if (bUp)
        {
            currentSelection--;
            if (currentSelection < 0)
                currentSelection = 3;
            updateDisplay();
            lastDebounce = millis();
        }
        if (bDown)
        {
            currentSelection++;
            if (currentSelection > 3)
                currentSelection = 0;
            updateDisplay();
            lastDebounce = millis();
        }
        if (bSel)
        {
            lastDebounce = millis();
            if (currentSelection == 0)
            { // START
                currentState = STATE_LOGGING;
                sessionStartTime = millis();
                distractionCounter = 0;
                updateDisplay();
            }
            else if (currentSelection == 1)
            { // WIFI SETUP
                Serial.println("\n[SYSTEM]: Initiating Network Sync...");
                initWiFi(); // This calls the function that connects and syncs NTP
                currentState = STATE_MENU;
                updateDisplay();
                lastDebounce = millis();
            }
            else if (currentSelection == 2)
            { // RESET STATS
                preferences.begin("cognilog", false);
                preferences.clear();
                preferences.end();
                totalFocusTime = 0;
                Serial.println("Stats Wiped.");
                updateDisplay();
            }
            else if (currentSelection == 3)
            { // <--- NEW CASE
                Serial.println("\n[SYSTEM]: Exporting Session Logs...");
                dumpStoredData(); // Calls the function to print CSV to Serial
                updateDisplay();  // Refresh menu so it doesn't look "stuck"
            }
        }
        break;

    case STATE_LOGGING:
        // --- 1. REALITY CHECK (The "Dead Man's Switch") ---
        // If it's time to check, and we aren't already waiting...
        if (!waitingForCheckIn && (millis() - lastCheckInTime > nextCheckInInterval))
        {
            waitingForCheckIn = true;
            checkInPromptTime = millis();

            Serial.println("[CHECK]: Are you still there? Press SELECT!");
            // Trigger Alarm chirp
            digitalWrite(PIN_BUZZER, HIGH);
            delay(100);
            digitalWrite(PIN_BUZZER, LOW);
        }

        // --- 2. HANDLING THE CHECK-IN ---
        if (waitingForCheckIn)
        {
            // Blink LED aggressively
            if ((millis() / 200) % 2 == 0)
                digitalWrite(PIN_LED, HIGH);
            else
                digitalWrite(PIN_LED, LOW);

            // IF BUTTON PRESSED: Pass!
            if (bSel)
            {
                waitingForCheckIn = false;
                digitalWrite(PIN_LED, LOW);
                Serial.println("[PASS]: Focus Verified.");

                // Reset Check-in Timer
                lastCheckInTime = millis();
                nextCheckInInterval = random(10000, 20000); // 10-20 seconds (TEST MODE)
                lastDebounce = millis();                    // Prevent double-triggering "Stop Session"
            }

            // IF TIMEOUT: Fail!
            if (millis() - checkInPromptTime > 10000)
            { // 10 seconds to react
                Serial.println("[FAIL]: User unresponsive. ALARM!");
                currentState = STATE_ALARM;
                waitingForCheckIn = false;
            }
        }

        // --- 3. NORMAL LOGGING (When not checking in) ---
        else
        {
            // Check if user wants to STOP manually
            if (bSel)
            {
                unsigned long duration = (millis() - sessionStartTime) / 1000;
                saveStats(duration);
                currentState = STATE_MENU;
                updateDisplay();
                lastDebounce = millis();
            }

            // Update the Display Timer every 1 second
            static unsigned long lastUpdate = 0;
            if (millis() - lastUpdate > 1000)
            {
                lastUpdate = millis();
                updateDisplay(); // This makes the "Time: 12s" tick up!
            }
        }
        break;

    case STATE_ALARM:
        Serial.println("!!! WAKE UP !!!");
        for (int i = 0; i < 3; i++)
        {
            digitalWrite(PIN_LED, HIGH);
            digitalWrite(PIN_BUZZER, HIGH);
            delay(100);
            digitalWrite(PIN_LED, LOW);
            digitalWrite(PIN_BUZZER, LOW);
            delay(100);
        }
        distractionCounter = 0;
        currentState = STATE_LOGGING;
        lastDebounce = millis();
        break;
    }
}