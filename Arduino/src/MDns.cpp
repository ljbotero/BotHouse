#include <ESP8266mDNS.h>
#include "Config.h"
#include "Logs.h"
#include "Mesh.h"
#include "MessageGenerator.h"
#include "Network.h"

namespace MDns {

#ifndef DISABLE_MDNS
static const Logs::caller me = Logs::caller::MDns;

static bool mdnsStarted = false;
static uint32_t delayMillisInitiate = 0;
static auto retries = 3;

void ICACHE_FLASH_ATTR initiate(const IPAddress& resolveToIP, uint32_t delayMillis) {
  if (mdnsStarted) {
    return;
  }
  if (!resolveToIP.isSet()) {
    Logs::serialPrintln(me, PSTR("IP is Unset, retrying in 20 seconds"));
    delayMillis = 20000;
  }
  if (delayMillis > 0) {
    delayMillisInitiate = millis() + delayMillis;
    return;
  }

  Logs::serialPrintlnStart(me, PSTR("Initiating"));

  mdnsStarted = MDNS.begin(MDNS_NAME, resolveToIP);
  if (mdnsStarted) {
    MDNS.notifyAPChange();
    MDNS.addService(F("http"), F("tcp"), 80);
    Logs::serialPrintln(me, PSTR("mDNS Beginning on IP: "), resolveToIP.toString().c_str());
  } else {
    Logs::serialPrintln(me, PSTR("mDNS FAILED Beginning on IP: "), resolveToIP.toString().c_str());
    if (retries > 0) {
      Logs::serialPrintln(me, PSTR("Retrying in 15 seconds"));
      delayMillisInitiate = millis() + 15000;
    }
    retries--;
    Logs::serialPrint(me, PSTR("Closing:"));
    MDNS.close();
    Logs::serialPrintln(me, PSTR("Ok"));
  }
  Logs::serialPrintlnEnd(me);
}
#endif

void ICACHE_FLASH_ATTR stop() {
#ifndef DISABLE_MDNS
  if (mdnsStarted) {
    Logs::serialPrintln(me, PSTR("mDNS Stopping"));
    MDNS.notifyAPChange();
    // MDNS.close();
    mdnsStarted = false;
  }
  delayMillisInitiate = 0;
#endif
}

void ICACHE_FLASH_ATTR start() {
#ifndef DISABLE_MDNS
  if (mdnsStarted) {
    return;
  }
  retries = 3;
  if (Mesh::isConnectedToWifi()) {
    initiate(WiFi.localIP(), 15000);
  } else { //if (Mesh::isAccessPointNode()) {
    //initiate(WiFi.softAPIP(), 15000);
    delayMillisInitiate = millis() + 15000;
  }
#endif
}

void handle() {
#ifndef DISABLE_MDNS
  if (mdnsStarted) {
    MDNS.update();
  } else if (delayMillisInitiate > 0 && millis() > delayMillisInitiate) {
    delayMillisInitiate = 0;
    if (Mesh::isConnectedToWifi()) {
      initiate(WiFi.localIP(), 0);
    } else { //if (Mesh::isAccessPointNode()) {
      //initiate(WiFi.softAPIP(), 0);
      delayMillisInitiate = millis() + 15000;
    }
  }
#endif
}

}  // namespace MDns