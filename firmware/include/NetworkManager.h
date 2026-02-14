#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <WiFi.h>
#include <WiFiUdp.h>
#include <time.h>

void initWiFi();
void syncTime();
bool checkUDP(bool &pcStatusFocus); 

#endif