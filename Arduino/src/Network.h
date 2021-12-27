#ifndef Network_h
#define Network_h

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "Config.h"
#include "Storage.h"

namespace Network {

struct httpResponse {
  int httpCode;
  String returnPayload;
};

httpResponse httpGet(const String &path);
httpResponse httpPost(const String &path, const String &payload, const String &auth,
    const String &contentType, const String &apiKey = FPSTR(""));
// httpResponse httpPost(const String &path, const String &payload, const String &auth,
//     const String &contentType, const String &apiKey = "");
uint8_t getConnectedStations();
void forceNetworkScan(const uint32_t waitMillis = 0);
void scheduleNextScan();
void startAccessPoint();
void stopAccessPoint();
bool connectToAP(const char *SSID, const String &password, const uint32_t channel,
    const uint8_t *bssid, const uint32_t timeoutMillis = MAX_MILLIS_TO_WAIT_TO_CONNECT_TO_AP);
void broadcastToMyAPNodes(const char *message);
bool broadcastToWifi(const char *message);
void broadcastEverywhere(const char *message, bool processLocallyFirst = false,
    bool stopIfProcessedLocally = false);
void sendUdpMessage(const IPAddress &ip, uint16_t port, const char *body);
// bool isIpInSubnet(const IPAddress &ip, const IPAddress &subnetIP);
void checkForUdpMessages(WiFiUDP &client, void (&onReceivedUdpMessage)(const char *message,
                                              const IPAddress &sender, const uint16_t senderPort));
bool isAnyClientConnectedToThisAP();
bool isOneOfMyClients(const IPAddress &clientIP);
void setup();
void handle();
}  // namespace Network
#endif
