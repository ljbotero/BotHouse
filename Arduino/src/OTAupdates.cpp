#include "Config.h"
#include <ESP8266HTTPUpdateServer.h>
#ifndef DISABLE_OTA
//#define DEBUG_ESP_OTA 1
#include <ArduinoOTA.h>
#include "Devices.h"
#include "Logs.h"
#endif

// Manual Update via command line:
// C:\Users\ljbot\AppData\Local\Arduino15\packages\esp8266\hardware\esp8266\2.6.2\tools\espota.py -i
// 192.168.2.220 -p 8266 -f
// "C:\Users\ljbot\AppData\Local\Temp\arduino_build_146826\Arduino.ino.bin"

namespace OTAupdates {

#ifndef DISABLE_OTA
static const Logs::caller me = Logs::caller::OTAupdates;

static ESP8266HTTPUpdateServer httpUpdater;

static bool _isOTAUpdating = false;
static uint32_t _lastOTAUpdate = 0;
#endif

bool isOTAUpdating() {
#ifndef DISABLE_OTA
  if (_lastOTAUpdate > 0) {
    if (millis() - _lastOTAUpdate > 15000) {
      // Unresponsive
      Devices::restart();
    }
    return true;
  }
#endif
  return false;
}

void ICACHE_FLASH_ATTR setupWebServer(ESP8266WebServer &httpServer) {
#ifndef DISABLE_OTA  
  httpUpdater.setup(&httpServer);
  Logs::serialPrintln(me, PSTR("OTAupdates WebServer setup"));
#endif
}

void ICACHE_FLASH_ATTR setup() {
#ifndef DISABLE_OTA
  // ArduinoOTA.setPort(8266);
  ArduinoOTA.onStart([]() {
    _isOTAUpdating = true;
    String type((char *)0);
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = F("sketch");
    } else {  // U_SPIFFS
      type = F("filesystem");
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Logs::serialPrintln(me, PSTR("Start updating "), String(type).c_str());
  });

  ArduinoOTA.onEnd([]() {
    _isOTAUpdating = false;
    Logs::serialPrintln(me, PSTR("\nEnd"));
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    _lastOTAUpdate = millis();
    Logs::serialPrintln(me, PSTR("Progress: "), String(progress / (total / 100)).c_str());
  });

  ArduinoOTA.onError([](ota_error_t error) {
    _isOTAUpdating = false;
    Logs::serialPrintln(me, PSTR("[ERROR]  "), String(error).c_str());
    if (error == OTA_AUTH_ERROR) {
      Logs::serialPrintln(me, PSTR("Auth Failed"));
    } else if (error == OTA_BEGIN_ERROR) {
      Logs::serialPrintln(me, PSTR("Begin Failed"));
    } else if (error == OTA_CONNECT_ERROR) {
      Logs::serialPrintln(me, PSTR("Connect Failed"));
    } else if (error == OTA_RECEIVE_ERROR) {
      Logs::serialPrintln(me, PSTR("Receive Failed"));
    } else if (error == OTA_END_ERROR) {
      Logs::serialPrintln(me, PSTR("End Failed"));
    }
  });
  ArduinoOTA.begin();
#endif
}

void handle() {
#ifndef DISABLE_OTA
  ArduinoOTA.handle();
#endif
}

}  // namespace OTAupdates