#include <ESP8266mDNS.h>
#include "Config.h"
#include "Logs.h"
#include "Mesh.h"
#include "MessageGenerator.h"
#include "Network.h"

namespace MDns {

const Logs::caller me = Logs::caller::MDns;

bool mdnsStarted = false;
uint32_t delayMillisInitiate = 0;
auto retries = 3;

void initiate(const IPAddress& resolveToIP, uint32_t delayMillis) {
  if (mdnsStarted) {
    return;
  }
  if (!resolveToIP.isSet()) {
    Logs::serialPrintln(me, F("IP is Unset, retrying in 20 seconds"));
    delayMillis = 20000;
  }
  if (delayMillis > 0) {
    delayMillisInitiate = millis() + delayMillis;
    return;
  }

  Logs::serialPrintlnStart(me, F("Initiating"));

  mdnsStarted = MDNS.begin(MDNS_NAME, resolveToIP);
  if (mdnsStarted) {
    MDNS.notifyAPChange();
    MDNS.addService(F("http"), F("tcp"), 80);
    Logs::serialPrintln(me, F("mDNS Beginning on IP: "), resolveToIP.toString());
  } else {
    Logs::serialPrintln(me, F("mDNS FAILED Beginning on IP: "), resolveToIP.toString());
    if (retries > 0) {
      Logs::serialPrintln(me, F("Retrying in 15 seconds"));
      delayMillisInitiate = millis() + 15000;
    }
    retries--;
    Logs::serialPrint(me, F("Closing:"));
    MDNS.close();
    Logs::serialPrintln(me, F("Ok"));
  }
  Logs::serialPrintlnEnd(me);
}

void stop() {
  if (mdnsStarted) {
    Logs::serialPrintln(me, F("mDNS Stopping"));
    MDNS.notifyAPChange();
    // MDNS.close();
    mdnsStarted = false;
  }
  delayMillisInitiate = 0;
}

void start() {
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
}

void handle() {
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
}

}  // namespace MDns