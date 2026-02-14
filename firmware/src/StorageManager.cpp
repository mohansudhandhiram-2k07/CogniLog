#include "StorageManager.h"

Preferences prefs;

void initStorage() {
    // Open namespace "cognilog" in Read/Write mode (false)
    prefs.begin("cognilog", false);
    prefs.end();
}

unsigned long loadTotalFocusTime() {
    prefs.begin("cognilog", true); // Read-only
    unsigned long total = prefs.getULong("focus_time", 0);
    prefs.end();
    return total;
}

void saveSession(unsigned long sessionSeconds) {
    prefs.begin("cognilog", false);
    
    // 1. Update Lifetime Total
    unsigned long currentTotal = prefs.getULong("focus_time", 0);
    prefs.putULong("focus_time", currentTotal + sessionSeconds);

    // 2. Log Individual Session
    int sessionID = prefs.getInt("session_count", 0);
    char key[15];
    sprintf(key, "sess_%d", sessionID);

    time_t now;
    time(&now);

    char dataString[32];
    sprintf(dataString, "%ld,%lu", (long)now, sessionSeconds);

    prefs.putString(key, dataString);
    prefs.putInt("session_count", sessionID + 1);
    
    prefs.end();
    Serial.printf("[STORAGE]: Saved Session %d (%lu sec)\n", sessionID, sessionSeconds);
}

void dumpAllSessions() {
    prefs.begin("cognilog", true);
    int total = prefs.getInt("session_count", 0);
    
    Serial.println("\n--- COGNILOG DATA DUMP ---");
    Serial.println("ID,Timestamp,Duration(s)");
    
    for (int i = 0; i < total; i++) {
        char key[15];
        sprintf(key, "sess_%d", i);
        String data = prefs.getString(key, "N/A");
        Serial.printf("%d,%s\n", i, data.c_str());
    }
    Serial.println("--- END OF DATA ---");
    prefs.end();
}

void clearAllData() {
    prefs.begin("cognilog", false);
    prefs.clear();
    prefs.end();
    Serial.println("[STORAGE]: All flash data wiped.");
}