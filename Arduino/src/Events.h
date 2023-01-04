#ifndef Events_h
#define Events_h
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include "Devices.h"

namespace Events {

bool isInSetupMode();
void setSetupMode();
void setSafeMode();
bool isSafeMode();
void onStartingAccessPoint();
void onConnectedToAPNode();
void onBecomingMasterWifiNode();
void onExitMasterWifiNode();
void onConnectedToWiFi();
void onScanNetworksComplete();
void onStoppingAccessPoint();
void onReceivedUdpMessage(const char *message, const IPAddress &sender, const uint16_t senderPort);
void onCriticalLoop();
bool onDeviceEvent(const Devices::DeviceState &state);
bool onWebServerNotFound(ESP8266WebServer &server);
void onStartingWebServer(ESP8266WebServer &server);
}  // namespace Events
#endif
