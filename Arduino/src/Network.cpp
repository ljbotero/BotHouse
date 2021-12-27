#include "Network.h"
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <TypeConversionFunctions.h>
#include <WiFiUdp.h>
#ifdef ENABLE_NAT_ROUTER
#include <dhcpserver.h>
#include <lwip/dns.h>
#include <lwip/napt.h>
#endif
#include "Config.h"
#include "Events.h"
#include "Logs.h"
#include "Mesh.h"
#include "MessageProcessor.h"
#include "Storage.h"
#include "Utils.h"

extern "C" {
#include "user_interface.h"
}

//#define NAPT 100
//#define NAPT_PORT 10

namespace Network {
static const Logs::caller me = Logs::caller::Network;

static const auto MINIMAL_SIGNAL_STRENGHT = -78;
static const auto NETWORK_SCAN_FREQUENCY_MILLIS = 1000 * 60 * 5;
static uint32_t nextScanTimeMillis = 0;
// WiFiEventHandler gotIpEventHandler, disconnectedEventHandler;

/*******************************************************************/

static WiFiUDP UdpClient;

/*******************************************************************/
ICACHE_FLASH_ATTR httpResponse httpGet(const String &path) {
  httpResponse response;
  response.returnPayload = FPSTR("");
  if (WiFi.status() != WL_CONNECTED) {
    return response;
  }
  Logs::serialPrintlnStart(me, PSTR("httpGet:"), path.c_str());

  Logs::serialPrintln(me, PSTR("http.begin:"));
  WiFiClient wifiClient;
  HTTPClient http;
  http.setReuse(false);

  if (!http.begin(wifiClient, path)) {
    Logs::serialPrintlnEnd(me, PSTR("httpGet:FAILED: "), path.c_str());
    // WiFi.mode(storedWiFiMode);
    http.end();
    return response;
  }
  http.addHeader(F("Accept"), F("*/*"), false, true);
  http.addHeader(F("Pragma"), F("no-cache"), false, true);
  http.addHeader(F("Cache-Control"), F("no-cache"), false, true);
  http.addHeader(F("Connection"), F("close"), false, true);

  http.setTimeout(1000);
  Logs::serialPrintln(me, PSTR("http.GET"));
  response.httpCode = http.GET();
  Logs::serialPrintln(me, PSTR("response.httpCode:"), String(response.httpCode).c_str());
  response.returnPayload = http.getString();
  Logs::serialPrintln(me, PSTR("response.returnPayload:"), String(response.returnPayload).c_str());
  http.end();
  Logs::serialPrintln(me, PSTR("http.ended"));
  if (response.httpCode != HTTP_CODE_OK && response.httpCode != HTTP_CODE_MOVED_PERMANENTLY) {
    Logs::serialPrintln(
        me, PSTR("httpPost:FAILED:"), String(response.httpCode).c_str(), PSTR(": "));
    Logs::serialPrint(me, response.returnPayload.c_str());
  }
  Logs::serialPrintlnEnd(me);
  // WiFi.mode(storedWiFiMode);
  return response;
}

httpResponse httpPost(const String &path, const String &payload, const String &auth,
    const String &contentType, const String &apiKey) {
  httpResponse response;
  response.returnPayload = FPSTR("");
  if (WiFi.status() != WL_CONNECTED || Events::isSafeMode()) {
    return response;
  }
  Logs::serialPrintlnStart(me, PSTR("httpGet:"), path.c_str());

  Logs::serialPrintlnStart(me, PSTR("httpPost:"), path.c_str());
  Logs::serialPrintln(me, payload.c_str());
  response.returnPayload = FPSTR("");

  // WiFiMode_t storedWiFiMode = WiFideploy.getMode();
  // WiFi.mode(WIFI_STA);

  Logs::serialPrintln(me, PSTR("http.begin:"));
  WiFiClient wifiClient;
  HTTPClient http;
  http.setReuse(false);

  if (!http.begin(wifiClient, path)) {
    Logs::serialPrintlnEnd(me, PSTR("httpPost:FAILED: "), path.c_str());
    // WiFi.mode(storedWiFiMode);
    http.end();
    return response;
  }

  Logs::serialPrintln(me, PSTR("Content-Type: "), contentType.c_str());
  http.addHeader(F("Content-Type"), contentType, true, true);
  if (!auth.isEmpty()) {
    Logs::serialPrintln(me, PSTR("Authorization: "), auth.c_str());
    http.addHeader(F("Authorization"), auth.c_str(), false, true);
  }
  if (!apiKey.isEmpty()) {
    Logs::serialPrintln(me, PSTR("x-api-key: "), apiKey.c_str());
    http.addHeader(F("x-api-key"), apiKey.c_str(), false, true);
  }
  http.addHeader(F("Accept"), F("*/*"), false, true);
  http.addHeader(F("Pragma"), F("no-cache"), false, true);
  http.addHeader(F("Cache-Control"), F("no-cache"), false, true);
  http.addHeader(F("Connection"), F("close"), false, true);

  http.setTimeout(500);
  Logs::serialPrintln(me, PSTR("http.POST"));

  response.httpCode = http.POST(payload);

  Logs::serialPrintln(me, PSTR("response.httpCode:"), String(response.httpCode).c_str());
  response.returnPayload = http.getString();
  Logs::serialPrintln(me, PSTR("response.returnPayload:"), String(response.returnPayload).c_str());
  http.end();
  Logs::serialPrintln(me, PSTR("http.ended"));
  if (response.httpCode != HTTP_CODE_OK && response.httpCode != HTTP_CODE_MOVED_PERMANENTLY) {
    Logs::serialPrint(me, PSTR("httpPost:FAILED:"), String(response.httpCode).c_str());
    Logs::serialPrintln(me, PSTR(": "), response.returnPayload.c_str());
  }
  Logs::serialPrintlnEnd(me);
  // WiFi.mode(storedWiFiMode);
  return response;
}

/*******************************************************************/

uint8_t getConnectedStations() {
  if (!Mesh::isAccessPointNode()) {
    return 0;
  }
  return WiFi.softAPgetStationNum();
}

bool ICACHE_FLASH_ATTR isOneOfMyClients(const IPAddress &clientIP) {
  if (getConnectedStations() == 0 || !clientIP.isSet()) {
    return false;
  }

  bool retValue = (WiFi.softAPIP()[0] == clientIP[0] && WiFi.softAPIP()[1] == clientIP[1] &&
                   WiFi.softAPIP()[2] == clientIP[2]);
  if (retValue) {
    Logs::serialPrintln(me, PSTR("isOneOfMyClients:"), clientIP.toString().c_str());
  }
  return retValue;
  // struct station_info *stat_info;
  // struct ip4_addr *IPaddress;
  // IPAddress address;
  // stat_info = wifi_softap_get_station_info();
  // while (stat_info != nullptr) {
  //   IPaddress = &stat_info->ip;
  //   address = IPaddress->addr;
  //   if (clientIP == address) {
  //     return true;
  //   }
  //   stat_info = STAILQ_NEXT(stat_info, next);
  // }
  // return false;
}

void sendUdpMessage(const IPAddress &ip, uint16_t port, const char *body) {
  if (body != nullptr && strlen(body) > 0) {
    // Logs::serialPrintln(me, PSTR("sendUdpMessage:"), String(strlen(body)));
    if (UdpClient.beginPacket(ip, port) == 0) {
      Logs::serialPrintln(me, PSTR("[ERROR] UdpClient.beginPacket"));
      return;
    }
    // UdpClient.write(body);
    UdpClient.print(body);
    UdpClient.endPacket();
    // UdpClient.flush();
  }
}

void checkForUdpMessages(WiFiUDP &client, void (&onReceivedUdpMessage)(const char *message,
                                              const IPAddress &sender, const uint16_t senderPort)) {
  if (Events::isSafeMode()) {
    return;
  }
  int packetSize = client.parsePacket();
  if (!packetSize) {
    return;
  }
  // auto totalPackages = 0;
  // auto totalPackageSize = packetSize;
  String finalMessage;  //((char *)0);
  finalMessage.reserve(256);  // 1024 causes exception
  while (packetSize) {
    char udpPacketBuffer[packetSize + 1];
    // totalPackages++;
    int n = client.read(udpPacketBuffer, packetSize);
    udpPacketBuffer[n] = 0;
    finalMessage = finalMessage + udpPacketBuffer;
    packetSize = client.parsePacket();
    // totalPackageSize += packetSize;
  }
  IPAddress ip;  // doing this instead of passing by ref due to an issue
  ip.fromString(client.remoteIP().toString());
  // Logs::serialPrint(me, PSTR("checkForUdpMessages:Received:"), String(totalPackages).c_str());
  // Logs::serialPrintln(me, PSTR(" packages of size "), String(totalPackageSize).c_str());
  onReceivedUdpMessage(finalMessage.c_str(), ip, client.remotePort());
}

// bool isIpInSubnet(IPAddress *ip, IPAddress *subnetIP) {
//   return subnetIP.isSet() && ip[0] == subnetIP[0] && ip[1] == subnetIP[1] && ip[2] ==
//   subnetIP[2];
// }

bool isAnyClientConnectedToThisAP() {
  uint8_t stations = Network::getConnectedStations();
  if (stations > 0) {
    Logs::serialPrintln(me, PSTR("N of connected stations: "), String(stations).c_str());
  }
  return stations > 0;
}

/**************************************************************************/
/**
 * MessageBroadcast to clients connected to my AP
 * @param message Message to send
 */
void broadcastToMyAPNodes(const char *message) {
  if (!Mesh::isAccessPointNode() || !Network::isAnyClientConnectedToThisAP()) {
    return;
  }
  if (strlen(message) <= 4) {
    Logs::serialPrintln(me, PSTR("[WARNING] broadcastToMyAPNodes: "), message);
    return;
  }
  IPAddress apIp = WiFi.softAPIP();
  if (!apIp.isSet()) {
    Logs::serialPrintln(me, PSTR("[WARNING] AP has no IP"));
    return;
  }
  apIp[3] = 255;
  Logs::serialPrintln(me, PSTR("broadcastToMyAPNodes - AP IP: "), apIp.toString().c_str());
  // Logs::serialPrintln(me, message);
  // WiFiMode_t storedWiFiMode = WiFi.getMode();
  // WiFi.mode(WIFI_AP);
  Network::sendUdpMessage(apIp, BROADCAST_PORT, message);
  // WiFi.mode(storedWiFiMode);
}

/**
 * MessageBroadcast to clients of AP I'm connected to
 * @param message Message to send
 */
bool broadcastToWifi(const char *message) {
  if (Events::isSafeMode()) {
    return false;
  }
  if (strlen(message) <= 4) {
    Logs::serialPrintln(me, PSTR("[WARNING] broadcastToWifi: "), message);
    return false;
  }
  if (!WiFi.isConnected()) {
    Logs::serialPrintln(me, PSTR("[WARNING] broadcastToWifi:NotConnectedToAnAP"));
    return false;
  }
  IPAddress localIp = WiFi.localIP();
  if (!localIp.isSet()) {
    return false;
  }
  localIp[3] = 255;
  Logs::pauseLogging(true);
  Logs::serialPrintln(me, PSTR("broadcastToWifi: "), localIp.toString().c_str());
  Logs::pauseLogging(false);
  Network::sendUdpMessage(localIp, BROADCAST_PORT, message);
  return true;
}

void broadcastEverywhere(
    const char *message, bool processLocallyFirst, bool stopIfProcessedLocally) {
  if (processLocallyFirst) {
    bool processed =
        MessageProcessor::processMessage(message, WiFi.localIP(), 80, !stopIfProcessedLocally);
    if (processed && stopIfProcessedLocally) {
      return;
    }
    if (!stopIfProcessedLocally) {
      broadcastToWifi(message);
    }
  } else {
    broadcastToWifi(message);
  }
  broadcastToMyAPNodes(message);
}

/*******************************************************************/
bool ICACHE_FLASH_ATTR connectToAP(const char *SSID, const String &password, const uint32_t channel,
    const uint8_t *bssid, const uint32_t timeoutMillis) {
  if (strlen(SSID) == 0) {
    Logs::serialPrintln(me, PSTR("[ERROR]  connectToAP: Cannot connect to an exmpty SSID"));
    return false;
  }
  Logs::serialPrintlnStart(me, PSTR("connectToAP: "), SSID);
  if (WiFi.isConnected()) {
    if (WiFi.SSID() == SSID) {
      Logs::serialPrintlnEnd(me, PSTR("connectToAP:AlreadyConnected"));
      return true;
    }
    WiFi.disconnect();
  }
  int milliSecondsCounter = millis() + timeoutMillis;
  int secondsCounter = millis() + 1000;
  int8_t lastStatus = -1;
  int8_t status = WiFi.begin(SSID, password.c_str());  //, channel, bssid);
  Logs::pauseLogging(true);
  while (status != WL_CONNECTED && status != WL_CONNECT_FAILED && millis() < milliSecondsCounter) {
    delay(200);
    if (millis() > secondsCounter) {
      secondsCounter = millis() + 1000;
      Logs::serialPrint(me, PSTR("."));
    }
    status = WiFi.status();
    if (lastStatus != status) {
      lastStatus = status;
      Logs::serialPrintln(me, PSTR(""));
      Logs::serialPrintln(me, PSTR("StatusChanged: "), String(status).c_str());
    }
    // status = WiFi.waitForConnectResult();
    Devices::handle();
  }
  Logs::pauseLogging(false);
  Logs::serialPrintln(me, PSTR(""));

  if (!WiFi.isConnected()) {
    Logs::serialPrintln(me, PSTR("Status: "), String(WiFi.status()).c_str());
    Logs::serialPrintlnEnd(me, PSTR("FAILURE: WiFi failed connecting"));
    return false;
  } else {
#ifdef ENABLE_NAT_ROUTER
    // give DNS servers to AP side - WiFi Repeater (NAT Router)
    dhcps_set_dns(0, WiFi.dnsIP(0));
    dhcps_set_dns(1, WiFi.dnsIP(1));
#endif

    // TODO
    // IPAddress gateway(192, 168, 2, 1);
    // IPAddress subnet(255, 255, 255, 0);
    // IPAddress dns(8, 8, 8, 8);  // Google DNS
    // WiFi.config(WiFi.localIP(), gateway, subnet, dns);

    Logs::serialPrintln(me, PSTR("connected to "), SSID, PSTR(" - Local IP: "));
    Logs::serialPrintlnEnd(me, WiFi.localIP().toString().c_str(), PSTR(" - AP IP: "),
        WiFi.softAPIP().toString().c_str());
    return true;
  }
}

String macToString(const unsigned char *mac) {
  char buf[20];
  snprintf(buf, sizeof(buf), "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3],
      mac[4], mac[5]);
  return String(buf);
}

void ICACHE_FLASH_ATTR onStationModeDisconnected(const WiFiEventStationModeDisconnected &event) {
  Logs::serialPrintln(me, PSTR("Station disconnected from WiFi"));
  forceNetworkScan(3000);
}

void ICACHE_FLASH_ATTR onStationConnected(const WiFiEventSoftAPModeStationConnected &evt) {
  Logs::serialPrintln(me, PSTR("Client connected: "));
  Logs::serialPrint(me, macToString(evt.mac).c_str());
}

void ICACHE_FLASH_ATTR onStationDisconnected(const WiFiEventSoftAPModeStationDisconnected &evt) {
  Logs::serialPrintln(me, PSTR("Client disconnected: "));
  Logs::serialPrint(me, macToString(evt.mac).c_str());
}

void ICACHE_FLASH_ATTR onDhcpTimeout() {
  Logs::serialPrintln(me, PSTR("[WARNING] Station Mode DHCP Timeout"));
}

void ICACHE_FLASH_ATTR startAccessPoint() {
#ifndef DISABLE_AP
  if (Mesh::isAccessPointNode()) {
    return;
  }
  Logs::serialPrintlnStart(me, PSTR("startAccessPoint:Start"));
  WiFi.mode(WIFI_AP_STA);
  Storage::storageStruct flashData = Storage::readFlash();
  String SSID((char *)0);
  Mesh::getMySSID(SSID, flashData);
  const String passphrase = flashData.wifiPassword;
  const auto channelNumber = MESH_CHANNEL;

  // enable AP, with android-compatible google domain

  WiFi.setOutputPower(20.5);  // Max power
  wifi_set_phy_mode(PHY_MODE_11G);
  WiFi.softAPConfig(accessPointIP, accessPointGateway, accessPointSubnet);
  Logs::serialPrintln(me, PSTR("Setting up AP "), SSID.c_str());
  WiFi.setSleepMode(WIFI_NONE_SLEEP);

  bool isAccessPoint = WiFi.softAP(SSID, passphrase, channelNumber);  //, 0, MESH_MAX_CLIENTS);
  if (isAccessPoint) {
// WiFi Repeater (NAT Router)
#ifdef ENABLE_NAT_ROUTER
    struct softap_config conf;
    wifi_softap_get_config(&conf);
    conf.max_connection = 3;
    conf.beacon_interval = 300;
    wifi_softap_set_config_current(&conf);

    err_t ret = ip_napt_init(IP_NAPT_MAX * 3 / 4, IP_PORTMAP_MAX * 3 / 4);
    if (ret == ERR_OK) {
      ret = ip_napt_enable_no(SOFTAP_IF, 1);
    }
    if (ret != ERR_OK) {
      Logs::serialPrintln(me, PSTR("[ERROR] NAPT initialization failed\n"));
    }
#endif
    Events::onStartingAccessPoint();
  }
#endif
  Logs::serialPrintlnEnd(me, PSTR("startAccessPoint:End"));
}

void ICACHE_FLASH_ATTR stopAccessPoint() {
  bool isAccessPoint = true;
  Events::onStoppingAccessPoint();
  for (uint8_t tries = 3; tries > 0 && isAccessPoint; tries--) {
    // Disconnect stations from the network established by the soft-AP.
    isAccessPoint = !WiFi.softAPdisconnect(true);
  }
  if (!isAccessPoint) {
    WiFi.mode(WIFI_STA);
    Logs::serialPrintln(me, PSTR("** AP Stopped"));
  }
}

void ICACHE_FLASH_ATTR forceNetworkScan(const uint32_t waitMillis) {
  Logs::serialPrintln(me, PSTR("forceNetworkScan"));
  if (waitMillis == 0) {
    nextScanTimeMillis = 0;
  } else {
    uint32_t proposeNextScanTimeMillis = millis() + waitMillis;
    if (nextScanTimeMillis < millis() || proposeNextScanTimeMillis < nextScanTimeMillis) {
      nextScanTimeMillis = proposeNextScanTimeMillis;
    }
  }
}

void ICACHE_FLASH_ATTR scheduleNextScan() {
  uint8_t stations = Network::getConnectedStations();
  nextScanTimeMillis = millis();
  nextScanTimeMillis += random(RANDOM_VARIANCE_MILLIS_FROM, RANDOM_VARIANCE_MILLIS_UPTO);
  if (stations == 0 && !WiFi.isConnected()) {
    nextScanTimeMillis += (NETWORK_SCAN_FREQUENCY_MILLIS / 5);
    Logs::serialPrint(me, PSTR("Scanning again in "));
    Logs::serialPrintln(me, String(nextScanTimeMillis / 1000).c_str(), PSTR(" seconds"));
  } else {
    nextScanTimeMillis += NETWORK_SCAN_FREQUENCY_MILLIS;
  }
}

void ICACHE_FLASH_ATTR setup() {
  nextScanTimeMillis = millis() + random(0, RANDOM_VARIANCE_MILLIS_UPTO);
  UdpClient.begin(BROADCAST_PORT);

  WiFi.mode(WIFI_STA);  // Start initially as station only to prevent starting an AP automatically
  WiFi.persistent(false);  // No need to to keep wifi info in flash
  WiFi.setAutoReconnect(
      true);  // attempt to reconnect to an access point in case it is disconnected.
  WiFi.setAutoConnect(true);  // connect to last used access point on power on.
  WiFi.scanDelete();
  WiFi.onStationModeDisconnected(&onStationModeDisconnected);
  WiFi.onSoftAPModeStationConnected(&onStationConnected);
  WiFi.onSoftAPModeStationDisconnected(&onStationDisconnected);
  WiFi.onStationModeDHCPTimeout(&onDhcpTimeout);

#if HAVE_NETDUMP
  phy_capture = dump;
#endif
}

void handle() {
  if (nextScanTimeMillis < millis()) {
    nextScanTimeMillis = millis() + (NETWORK_SCAN_FREQUENCY_MILLIS);
    WiFi.scanNetworks(true, false);
  }
  int numberOfNetworks = WiFi.scanComplete();
  if (numberOfNetworks >= 0) {
    Mesh::scanNetworksComplete(numberOfNetworks);
    WiFi.scanDelete();
  } else {
    checkForUdpMessages(UdpClient, Events::onReceivedUdpMessage);
  }
}

}  // namespace Network