#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include <time.h>

// Initializes the NVS (Non-Volatile Storage)
void initStorage();

// Loads the lifetime focus time from flash
unsigned long loadTotalFocusTime();

// Saves a completed session (Unix timestamp + duration)
void saveSession(unsigned long sessionSeconds);

// Formats and prints all stored data to Serial (for your Data Dump)
void dumpAllSessions();

// Wipes the memory (for the Reset Stats menu option)
void clearAllData();

#endif