#include "Network.h"
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <TypeConversionFunctions.h>
#include <WiFiClientSecureBearSSL.h>
#include <WiFiUdp.h>
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

namespace Network {
const Logs::caller me = Logs::caller::Network;

const int16_t MINIMAL_SIGNAL_STRENGHT = -80;
const uint32_t NETWORK_SCAN_FREQUENCY_MILLIS = 1000 * 60 * 5;
uint32_t nextScanTimeMillis = 0;
// WiFiEventHandler gotIpEventHandler, disconnectedEventHandler;

/*******************************************************************/

WiFiUDP UdpClient;

/*******************************************************************/

httpResponse httpsPost(const String &path, const String &payload, const String &auth,
    const String &contentType, const String &apiKey, const char *rootCA) {
  Logs::serialPrintlnStart(me, F("httpsPost:"), path);
  Logs::serialPrintln(me, payload);
  httpResponse response;
  response.returnPayload = "";

  BearSSL::WiFiClientSecure httpsClient;
  HTTPClient https;
  // httpsClient.setNoDelay(true);
  // httpsClient.setSync(false);
  //https.setReuse(false);

  //WiFiMode_t storedWiFiMode = WiFi.getMode();
  //WiFi.mode(WIFI_STA);

  if (rootCA == nullptr) {
    Logs::serialPrintln(me, F("http.begin"));
    if (!https.begin(path)) {
      Logs::serialPrintlnEnd(me, F("httpPost:FAILED: "), path);
      //WiFi.mode(storedWiFiMode);
      return response;
    }
  } else {
    Logs::serialPrintln(me, F("https.begin:"));
    httpsClient.setInsecure();
    // httpsClient.setCiphersLessSecure();
    // BearSSLX509List cert(rootCA);
    // httpsClient.setTrustAnchors(&cert);
    //httpsClient.setFingerprint(rootCA);
    if (!https.begin(httpsClient, path)) {
      Logs::serialPrintlnEnd(me, F("httpsPost:FAILED: "), path);
      //WiFi.mode(storedWiFiMode);
      return response;
    }     
  }
  Logs::serialPrintln(me, F("Content-Type: "), contentType);
  https.addHeader(F("Content-Type"), contentType, true, true);
  if (!auth.isEmpty()) {
    Logs::serialPrintln(me, F("Authorization: "), auth.c_str());
    https.addHeader(F("Authorization"), auth.c_str(), false, true);
  }
  if (!apiKey.isEmpty()) {
    Logs::serialPrintln(me, F("x-api-key: "), apiKey.c_str());
    https.addHeader(F("x-api-key"), apiKey.c_str(), false, true);
  }
  https.addHeader(F("Accept"), F("*/*"), false, true);
  https.addHeader(F("Pragma"), F("no-cache"), false, true);
  https.addHeader(F("Cache-Control"), F("no-cache"), false, true);
  https.addHeader(F("Connection"), F("keep-alive"), false, true);

  Logs::serialPrintln(me, F("https.POST"));

  response.httpCode = https.POST(payload);

  Logs::serialPrintln(me, F("response.httpCode:"), String(response.httpCode));
  response.returnPayload = https.getString();
  Logs::serialPrintln(me, F("response.returnPayload:"), String(response.returnPayload));
  https.end();
  Logs::serialPrintln(me, F("https.ended"));
  if (response.httpCode != HTTP_CODE_OK && response.httpCode != HTTP_CODE_MOVED_PERMANENTLY) {
    Logs::serialPrintln(
        me, F("httpPost:FAILED:"), String(response.httpCode) + ": " + response.returnPayload);
  }
  Logs::serialPrintlnEnd(me);
  // WiFi.mode(storedWiFiMode);
  return response;
}

// httpResponse httpPost(const String &path, const String &payload, const String &auth,
//     const String &contentType, const String &apiKey) {
//   Logs::serialPrintlnStart(me, F("httpPost:"), path);
//   Logs::serialPrintln(me, payload);
//   httpResponse response;
//   response.returnPayload = "";

//   http.setReuse(false);

//   httpClient.setNoDelay(true);
//   httpClient.setSync(false);

//   Logs::serialPrintln(me, F("http.begin"));
//   if (!http.begin(httpClient, path)) {
//     http.end();
//     Logs::serialPrintlnEnd(me, F("httpPost:FAILED: "), path);
//     return response;
//   }
//   Logs::serialPrintln(me, F("Content-Type: "), contentType);
//   http.addHeader(F("Content-Type"), contentType, true, true);
//   if (!auth.isEmpty()) {
//     Logs::serialPrintln(me, F("Authorization: "), auth.c_str());
//     http.addHeader(F("Authorization"), auth.c_str(), false, true);
//   }
//   if (!apiKey.isEmpty()) {
//     Logs::serialPrintln(me, F("x-api-key: "), apiKey.c_str());
//     http.addHeader(F("x-api-key"), apiKey.c_str(), false, true);
//   }
//   http.addHeader(F("Accept"), F("*/*"), false, true);
//   http.addHeader(F("Pragma"), F("no-cache"), false, true);
//   http.addHeader(F("Cache-Control"), F("no-cache"), false, true);
//   http.addHeader(F("Connection"), F("close"), false, true);

//   Logs::serialPrintln(me, F("http.POST"));

//   response.httpCode = http.POST(payload);

//   Logs::serialPrintln(me, F("response.httpCode:"), String(response.httpCode));
//   response.returnPayload = http.getString();
//   Logs::serialPrintln(me, F("response.returnPayload:"), String(response.returnPayload));
//   http.end();
//   Logs::serialPrintln(me, F("http.ended"));
//   if (response.httpCode != HTTP_CODE_OK && response.httpCode != HTTP_CODE_MOVED_PERMANENTLY) {
//     Logs::serialPrintln(
//         me, F("httpPost:FAILED:"), String(response.httpCode) + ": " + response.returnPayload);
//   }
//   Logs::serialPrintlnEnd(me);
//   return response;
// }

/*******************************************************************/

uint8_t getConnectedStations() {
  if (!Mesh::isAccessPointNode()) {
    return 0;
  }
  return WiFi.softAPgetStationNum();
}

bool isOneOfMyClients(const IPAddress &clientIP) {
  if (getConnectedStations() == 0 || !clientIP.isSet()) {
    return false;
  }
  struct station_info *stat_info;
  struct ip4_addr *IPaddress;
  IPAddress address;
  stat_info = wifi_softap_get_station_info();
  while (stat_info != nullptr) {
    IPaddress = &stat_info->ip;
    address = IPaddress->addr;
    if (clientIP == address) {
      return true;
    }
    stat_info = STAILQ_NEXT(stat_info, next);
  }
  return false;
}

void sendUdpMessage(IPAddress ip, uint16_t port, const char *body) {
  if (body != nullptr && strlen(body) > 0) {
    // Logs::serialPrintln(me, F("sendUdpMessage:"), String(strlen(body)));
    UdpClient.beginPacket(ip, port);
    UdpClient.write(body);
    UdpClient.endPacket();
  }
}

void checkForUdpMessages(WiFiUDP &client, void (&onReceivedUdpMessage)(const char *message,
                                              const IPAddress &sender, const uint16_t senderPort)) {
  int packetSize = client.parsePacket();
  if (!packetSize) {
    return;
  }
  String finalMessage;
  while (packetSize) {
    char udpPacketBuffer[packetSize + 1];
    int n = client.read(udpPacketBuffer, packetSize);
    udpPacketBuffer[n] = 0;
    finalMessage = finalMessage + String(udpPacketBuffer);
    packetSize = client.parsePacket();
  }
  IPAddress ip;  // doing this instead of passing by ref due to an issue
  ip.fromString(client.remoteIP().toString());
  onReceivedUdpMessage(finalMessage.c_str(), ip, client.remotePort());
}

// bool isIpInSubnet(const IPAddress &ip, const IPAddress &subnetIP) {
//   return subnetIP.isSet() && ip[0] == subnetIP[0] && ip[1] == subnetIP[1] && ip[2] ==
//   subnetIP[2];
// }

bool isAnyClientConnectedToThisAP() {
  uint8_t stations = Network::getConnectedStations();
  if (stations > 0) {
    Logs::serialPrintln(me, F("N of connected stations: "), String(stations));
  }
  return stations > 0;
}

/**************************************************************************/
/**
 * MessageBroadcast to clients connected to my AP
 * @param message Message to send
 */
void forwardMessage(const String &message) {
  if (!Mesh::isAccessPointNode() && !Network::isAnyClientConnectedToThisAP()) {
    return;
  }
  if (message.length() <= 4) {
    Logs::serialPrintln(me, F("WARNING forwardMessage: "), message);
    return;
  }
  IPAddress apIp = WiFi.softAPIP();
  if (!apIp.isSet()) {
    Logs::serialPrintln(me, F("WARNING AP has no IP"));
    return;
  }
  apIp[3] = 255;
  Logs::serialPrintln(me, F("forwardMessage - AP IP: "), apIp.toString());
  // Logs::serialPrintln(me, message);
  // WiFiMode_t storedWiFiMode = WiFi.getMode();
  // WiFi.mode(WIFI_AP);
  Network::sendUdpMessage(apIp, BROADCAST_PORT, message.c_str());
  // WiFi.mode(storedWiFiMode);
}

/**
 * MessageBroadcast to clients of AP I'm connected to
 * @param message Message to send
 */
void broadcastMessage(
    const String &message, bool processLocallyFirst, bool stopIfProcessedLocally) {
  if (message.length() <= 4) {
    Logs::serialPrintln(me, F("WARNING broadcastMessage: "), message);
    return;
  }
  if (processLocallyFirst) {
    if (MessageProcessor::processMessage(message.c_str()) && stopIfProcessedLocally) {
      return;
    }
  }
  if (!WiFi.isConnected()) {
    Logs::serialPrintln(me, F("WARNING broadcastMessage:NotConnectedToAnAP"));
  } else {
    // WiFiMode_t storedWiFiMode = WiFi.getMode();
    // WiFi.mode(WIFI_STA);
    IPAddress localIp = WiFi.localIP();
    if (localIp.isSet()) {
      localIp[3] = 255;
      Logs::pauseLogging(true);
      Logs::serialPrintln(me, F("broadcastMessage to: "), localIp.toString());
      Logs::pauseLogging(false);
      Network::sendUdpMessage(localIp, BROADCAST_PORT, message.c_str());
    }
    // WiFi.mode(storedWiFiMode);
  }
  if (!processLocallyFirst) {
    forwardMessage(message);
  }
}

/*******************************************************************/
bool connectToAP(const String &ssid, const String &password, const uint32_t channel,
    const uint8_t *bssid, const uint32_t timeoutMillis) {
  if (ssid.isEmpty()) {
    Logs::serialPrintln(me, F("ERROR: connectToAP: Cannot connect to an exmpty SSID"));
    return false;
  }
  Logs::serialPrintlnStart(me, F("connectToAP: "), ssid);
  if (WiFi.isConnected()) {
    if (WiFi.SSID() == ssid) {
      Logs::serialPrintlnEnd(me, F("connectToAP:AlreadyConnected"));
      return true;
    }
    WiFi.disconnect();
  }
  int milliSecondsCounter = millis() + timeoutMillis;
  int secondsCounter = millis() + 1000;
  int8_t lastStatus = -1;
  int8_t status = WiFi.begin(ssid.c_str(), password.c_str());  //, channel, bssid);
  Logs::pauseLogging(true);
  while (status != WL_CONNECTED && status != WL_CONNECT_FAILED && millis() < milliSecondsCounter) {
    delay(200);
    if (millis() > secondsCounter) {
      secondsCounter = millis() + 1000;
      Logs::serialPrint(me, ".");
    }
    status = WiFi.status();
    if (lastStatus != status) {
      lastStatus = status;
      Logs::serialPrintln(me, "");
      Logs::serialPrintln(me, F("StatusChanged: "), String(status));
    }
    // status = WiFi.waitForConnectResult();
    Devices::handle();
  }
  Logs::pauseLogging(false);
  Logs::serialPrintln(me, "");

  if (!WiFi.isConnected()) {
    Logs::serialPrintln(me, "Status: " + String(WiFi.status()));
    Logs::serialPrintlnEnd(me, F("FAILURE: WiFi failed connecting"));
    return false;
  } else {
    // TODO
    // IPAddress gateway(192, 168, 2, 1);
    // IPAddress subnet(255, 255, 255, 0);
    // IPAddress dns(8, 8, 8, 8);  // Google DNS
    // WiFi.config(WiFi.localIP(), gateway, subnet, dns);

    Logs::serialPrintlnEnd(me, F("connected to "), ssid,
        String(FPSTR(" - Local IP: ")) + WiFi.localIP().toString() + String(FPSTR(" - AP IP: ")) +
            WiFi.softAPIP().toString());
    return true;
  }
}

String macToString(const unsigned char *mac) {
  char buf[20];
  snprintf(buf, sizeof(buf), "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3],
      mac[4], mac[5]);
  return String(buf);
}

void onStationConnected(const WiFiEventSoftAPModeStationConnected &evt) {
  Logs::serialPrint(me, F("Station connected: "));
  Logs::serialPrintln(me, macToString(evt.mac));
}

void onStationDisconnected(const WiFiEventSoftAPModeStationDisconnected &evt) {
  Logs::serialPrint(me, F("Station disconnected: "));
  Logs::serialPrintln(me, macToString(evt.mac));
}

void onDhcpTimeout() {
  Logs::serialPrintln(me, F("(Warning) Station Mode DHCP Timeout"));
}

void startAccessPoint() {
#ifndef DISABLE_AP
  if (Mesh::isAccessPointNode()) {
    return;
  }
  WiFi.mode(WIFI_AP_STA);
  Storage::storageStruct flashData = Storage::readFlash();
  const String ssid = Mesh::getMySSID(flashData).c_str();
  const String passphrase = flashData.meshPassword;
  const int channelNumber = MESH_CHANNEL;

  WiFi.softAPConfig(accessPointIP, accessPointGateway, accessPointSubnet);
  Logs::serialPrintln(me, F("Setting up AP "), ssid);
  WiFi.onSoftAPModeStationConnected(&onStationConnected);
  WiFi.onSoftAPModeStationDisconnected(&onStationDisconnected);
  WiFi.onStationModeDHCPTimeout(&onDhcpTimeout);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  bool isAccessPoint = WiFi.softAP(ssid, passphrase, channelNumber);  //, 0, MESH_MAX_CLIENTS);
  if (isAccessPoint) {
    Events::onStartingAccessPoint();
  }
#endif
}

void stopAccessPoint() {
  bool isAccessPoint = true;
  Events::onStoppingAccessPoint();
  for (uint8_t tries = 3; tries > 0 && isAccessPoint; tries--) {
    // Disconnect stations from the network established by the soft-AP.
    isAccessPoint = !WiFi.softAPdisconnect(true);
  }
  if (!isAccessPoint) {
    WiFi.mode(WIFI_STA);
    Logs::serialPrintln(me, F("** AP Stopped"));
  }
}

void forceNetworkScan(const uint32_t waitMillis) {
  Logs::serialPrintln(me, F("forceNetworkScan"));
  if (waitMillis == 0) {
    nextScanTimeMillis = 0;
  } else {
    nextScanTimeMillis = millis() + waitMillis;
  }
}

void scheduleNextScan() {
  uint8_t stations = Network::getConnectedStations();
  nextScanTimeMillis = millis();
  nextScanTimeMillis += random(RANDOM_VARIANCE_MILLIS_FROM, RANDOM_VARIANCE_MILLIS_UPTO);
  if (stations == 0 && !WiFi.isConnected()) {
    nextScanTimeMillis += (NETWORK_SCAN_FREQUENCY_MILLIS / 5);
    Logs::serialPrint(me, F("Scanning again in "));
    Logs::serialPrintln(me, String(nextScanTimeMillis / 1000) + FPSTR(" seconds"));
  } else {
    nextScanTimeMillis += NETWORK_SCAN_FREQUENCY_MILLIS;
  }
}

void setup() {
  WiFi.persistent(false);  // No need to to keep wifi info in flash
  WiFi.setAutoReconnect(
      true);  // attempt to reconnect to an access point in case it is disconnected.
  WiFi.setAutoConnect(true);  // connect to last used access point on power on.
  WiFi.scanDelete();
  nextScanTimeMillis = millis() + random(0, RANDOM_VARIANCE_MILLIS_UPTO);
  UdpClient.begin(BROADCAST_PORT);
  WiFi.mode(WIFI_STA);  // Start initially as station only to prevent starting an AP automatically
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