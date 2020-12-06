#include "src/Config.h"
#include "src/Devices.h"
#include "src/Events.h"
#include "src/Files.h"
#include "src/Logs.h"
#include "src/Network.h"
#include "src/OTAupdates.h"
#include "src/Storage.h"

#ifdef RUN_UNIT_TESTS
#include <AUnit.h>
#include <AUnitVerbose.h>
#include "tests/Mesh.Test.h"
#endif

const Logs::caller me = Logs::caller::Main;

void setup() {
#if defined(DEBUG_MODE) || defined(RUN_UNIT_TESTS)
  //Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
  Serial.begin(9600); 
  Serial.setTimeout(2000);
  while (!Serial) {
    delay(1);
  }
#endif
  // delay(10000);
  Logs::serialPrintln(me, F("\n\n*********BOOTING-UP*********"));
  Logs::serialPrintln(me, F("ESP Full Version: "), ESP.getFullVersion());
  Logs::serialPrintln(me, F("ChipId:               "), String(ESP.getChipId()));
  Logs::serialPrintln(me, F("Sdk Version:          "), String(ESP.getSdkVersion()));
  Logs::serialPrintln(me, F("Core Version:         "), String(ESP.getCoreVersion()));
  Logs::serialPrintln(me, F("Boot Version:         "), String(ESP.getBootVersion()));
  Logs::serialPrintln(me, F("Flash Chip Vendor Id: "), String(ESP.getFlashChipVendorId()));
  Logs::serialPrintln(me, F("Flash Chip Id:        "), String(ESP.getFlashChipId()));
  Logs::serialPrintln(me, F("Reset Reason: "), ESP.getResetReason());
  Logs::serialPrintln(me, ESP.getResetInfo());
#ifdef ARDUINO_ESP8266_GENERIC
  Logs::serialPrintln(me, F("ESP8266_GENERIC"));
#elif defined(ARDUINO_ESP8266_NODEMCU)
  Logs::serialPrintln(me, F("ESP8266_NODEMCU"));
#else
  Logs::serialPrintln(me, F("ESP8266_UNKNOWN"));
#endif

  Files::setup();
  Storage::setup();
  Devices::setup();
  Network::setup();
  OTAupdates::setup();
}

// the loop function runs over and over again forever
void loop() {
#ifdef RUN_UNIT_TESTS
  aunit::TestRunner::run();
  OTAupdates::handle();
#else
  if (!OTAupdates::isOTAUpdating()) {
    Events::onCriticalLoop();
    Devices::handle();
  }
  OTAupdates::handle();
  delay(1);
#endif
}
