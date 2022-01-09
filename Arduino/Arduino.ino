
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
#include "tests/Devices.Test.h"
//#include "tests/Utils.Test.h"
#endif

static const Logs::caller me = Logs::caller::Main;

void ICACHE_FLASH_ATTR setup() {
  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
  //Serial.begin(9600, SERIAL_8N1, SERIAL_TX_ONLY);
  // Serial.begin(9600);
  while (!Serial) {
    delay(1);
  }
#ifdef DEBUG_MODE
  delay(5000);
#endif
  Logs::serialPrintln(me, PSTR("\n\n*********BOOTING-UP*********"));
  Logs::logEspInfo();
  rst_info *resetInfo = ESP.getResetInfoPtr();
  if (resetInfo != nullptr) {
    Logs::serialPrintln(me, PSTR("("), String(resetInfo->reason).c_str(), PSTR(")"));
    if (resetInfo->reason != rst_reason::REASON_DEFAULT_RST &&
        resetInfo->reason != rst_reason::REASON_DEEP_SLEEP_AWAKE &&
        resetInfo->reason != rst_reason::REASON_SOFT_RESTART &&
        resetInfo->reason != rst_reason::REASON_EXT_SYS_RST) {
      Events::setSafeMode();
      Logs::serialPrintln(me, PSTR("### SAFE MODE ENABLED FOR NEXT 5 MINUTES ###"));
    }
  } else {
    Logs::serialPrintln(me, PSTR(""));
  }
#ifdef ARDUINO_ESP8266_GENERIC
  Logs::serialPrintln(me, PSTR("ESP8266_GENERIC"));
#elif defined(ARDUINO_ESP8266_NODEMCU)
  Logs::serialPrintln(me, PSTR("ESP8266_NODEMCU"));
#else
  Logs::serialPrintln(me, PSTR("ESP8266_UNKNOWN"));
#endif

#ifndef RUN_UNIT_TESTS
  Files::setup();
  Storage::setup();
  Devices::setup();
  Network::setup();
  OTAupdates::setup();
#endif
}

// the loop function runs over and over again forever
void loop() {
#ifdef RUN_UNIT_TESTS
  aunit::TestRunner::run();
  //OTAupdates::handle();
#else
  if (!OTAupdates::isOTAUpdating()) {
    Events::onCriticalLoop();
    Devices::handle();
  }
  OTAupdates::handle();
  delay(1);
#endif
}
