#ifndef OTAupdates_h
#define OTAupdates_h
#include <ArduinoOTA.h>

namespace OTAupdates {
bool isOTAUpdating();
void setupWebServer(ESP8266WebServer &httpServer);
void setup();
void handle();
}  // namespace OTAupdates
#endif