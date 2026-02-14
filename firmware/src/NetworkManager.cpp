#include "NetworkManager.h"
#include "Config.h"

WiFiUDP udp;
unsigned int localUdpPort = 4210;
char incomingPacket[255];

void initWiFi() {
    Serial.printf("Connecting to %s", ssid);
    WiFi.begin(ssid, password);
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 20) {
        delay(500);
        Serial.print(".");
        retries++;
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[WIFI]: Connected!");
        syncTime();
        udp.begin(localUdpPort);
    } else {
        Serial.println("\n[WIFI]: Offline Mode.");
    }
}

void syncTime() {
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
}

bool checkUDP(bool &pcStatusFocus) {
    int packetSize = udp.parsePacket();
    if (packetSize) {
        int len = udp.read(incomingPacket, 255);
        if (len > 0) incomingPacket[len] = 0;
        String status = String(incomingPacket);
        pcStatusFocus = (status == "FOCUS");
        return true; // Received a packet
    }
    return false;
}