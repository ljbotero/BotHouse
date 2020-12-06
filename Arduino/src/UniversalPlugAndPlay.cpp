//#include <ESP8266SSDP.h>
#include "UniversalPlugAndPlay.h"
#include "Config.h"
#include "Devices.h"
#include "Logs.h"
#include "Mesh.h"
#ifndef DISABLE_UPNP
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "Network.h"
#include "Utils.h"
#include "WebServer.h"
#include "lwip/igmp.h"
#endif

namespace UniversalPlugAndPlay {

const Logs::caller me = Logs::caller::UniversalPlugAndPlay;

char _uuid[SSDP_UUID_SIZE];

char *getUUID(String &deviceId, uint8_t deviceIndex) {
  sprintf_P(
      _uuid, PSTR("uuid:38323636-4558-4dda-9188-cda0e%05s%02d"), deviceId.c_str(), deviceIndex);
  return _uuid;
}

//#ifndef ARDUINO_ESP8266_GENERIC

#ifndef DISABLE_UPNP
WiFiUDP ssdpClient;  // Simple Discovery Protocol (SSDP)

char _root[SSDP_SCHEMA_URL_SIZE];
char _schemaURL[SSDP_SCHEMA_URL_SIZE];
char _deviceType[SSDP_DEVICE_TYPE_SIZE];
char _friendlyName[SSDP_FRIENDLY_NAME_SIZE];
char _serialNumber[SSDP_SERIAL_NUMBER_SIZE];
char _presentationURL[SSDP_PRESENTATION_URL_SIZE];
char _manufacturer[SSDP_MANUFACTURER_SIZE];
char _manufacturerURL[SSDP_MANUFACTURER_URL_SIZE];
char _modelName[SSDP_MODEL_NAME_SIZE];
char _modelURL[SSDP_MODEL_URL_SIZE];
char _modelNumber[SSDP_MODEL_VERSION_SIZE];
char _serviceType[SDP_SERVICE_TYPE_SIZE];
char _serviceId[SDP_SERVICE_ID_SIZE];
char _SCPDURL[SDP_SCPDURL_SIZE];
char _controlURL[SDP_CONTROL_URL_SIZE];
char _eventSubURL[SDP_EVENT_SUB_URL_SIZE];
#endif

/* ********************************************************************/
void setDeviceFieldValues(
    Mesh::Node *node, Devices::DeviceDescription *device, const String &ssdpDeviceType) {
#ifndef DISABLE_UPNP
  const String SERIAL_NUMBER = node->deviceId + String(device->index);
  String urlpath = node->deviceId + FPSTR("/") + String(device->index);
  getUUID(node->deviceId, device->index);
  sprintf(_schemaURL, (urlpath + FPSTR(SSDP_DESCRIPTION_PATH)).c_str());
  sprintf_P(_root, SSDP_ROOT);
  sprintf(_deviceType, String(ssdpDeviceType + FPSTR(":") + device->typeId).c_str());
  _modelNumber[0] = '\0';
  sprintf(_friendlyName,
      String(node->deviceName + "-" + device->typeId + "-" + String(device->index)).c_str());
  sprintf(_presentationURL, SSDP_URL);
  _serialNumber[0] = '\0';
  sprintf(_modelName, SSDP_MODEL_NAME);
  sprintf(_modelURL, SSDP_URL);
  sprintf(_manufacturer, SSDP_MANUFACTURER);
  _manufacturerURL[0] = '\0';
  // Logs::serialPrintln(me, "serviceUrl:len:" + String(strlen(serviceUrl)));
  sprintf(_serviceType, device->typeId.c_str());
  sprintf(_serviceId, device->typeId.c_str());
  sprintf(_SCPDURL, String(urlpath + FPSTR("/") + device->typeId).c_str());
  sprintf(_controlURL, _SCPDURL);
  sprintf(_eventSubURL, _SCPDURL);
#endif
}

void sendMessage(char buffer[], const IPAddress &senderIP, const uint16_t senderPort) {
#ifndef DISABLE_UPNP
  String message = String(buffer);
  message.replace("\r\n", "\\n");
  Logs::serialPrintln(
      me, F("send:"), senderIP.toString() + FPSTR(":") + String(senderPort) + FPSTR(":") + message);
  // Network::sendUdpMessage(remoteAddr, remotePort, buffer);
  ssdpClient.beginPacket(senderIP, senderPort);
  ssdpClient.write(buffer);
  ssdpClient.endPacket();
#endif
}

void respondForEachDevice(
    const IPAddress &senderIP, const uint16_t senderPort, const String &ssdpDeviceType) {
#ifndef DISABLE_UPNP
  Mesh::Node *currNode = Mesh::getNodesTip();
  IPAddress ip = WiFi.localIP();
  uint16_t port = 80;
  while (currNode != nullptr) {
    Devices::DeviceDescription *currDevice = currNode->devices;
    while (currDevice != nullptr) {
      setDeviceFieldValues(currNode, currDevice, ssdpDeviceType);
      char buffer[1460];
      snprintf_P(buffer, sizeof(buffer), _ssdp_response_template, SSDP_INTERVAL, _modelName,
          _modelNumber, _uuid, ssdpDeviceType.c_str(), ip.toString().c_str(), port, _schemaURL);
      sendMessage(buffer, senderIP, senderPort);

      Logs::serialPrintln(me, F("schemaURL:"), _schemaURL);
      currDevice = currDevice->next;
    }
    currNode = currNode->next;
  }
  Logs::serialPrintlnEnd(me);
#endif
}

void eventNotify(const String &propertyName, const String &propertyValue, const IPAddress &senderIP,
    const uint16_t senderPort) {
  // TODO
}

/**********************************************************************/

void handleDescription(const String &ssdpDeviceType) {
#ifndef DISABLE_UPNP
  ESP8266WebServer &server = WebServer::getServer();
  String deviceId = "";
  uint8_t deviceIndex = 0;

  int pos1stSlash = server.uri().indexOf('/', 1);
  int pos2ndSlash = server.uri().indexOf('/', pos1stSlash + 1);
  if (pos1stSlash > 0 && pos1stSlash < pos2ndSlash) {
    deviceId = server.uri().substring(1, pos1stSlash);
    auto deviceNumberStr = server.uri().substring(pos1stSlash + 1, pos2ndSlash);
    deviceIndex = deviceNumberStr.toInt();
    Logs::serialPrintln(me, F("handleDescription:"), String(server.uri()) + FPSTR(":"),
        deviceId + FPSTR(":") + deviceNumberStr);
  } else {
    Logs::serialPrintln(me, F("handleDescription:NULL"));
    server.send(404);
    return;
  }
  if (deviceId.isEmpty()) {
    server.send(404, F("text/plain"), F("device Id not found"));
    return;
  }
  Mesh::Node *currNode = Mesh::getNodesTip();
  while (currNode != nullptr) {
    if (currNode->deviceId == deviceId) {
      break;
    } else {
      currNode = currNode->next;
    }
  }
  if (currNode == nullptr) {
    server.send(404, F("text/plain"), F("device Id not valid"));
    return;
  }

  uint16_t port = 80;
  IPAddress ip = WiFi.localIP();
  Devices::DeviceDescription *device = Devices::getDeviceFromIndex(currNode->devices, deviceIndex);
  setDeviceFieldValues(currNode, device, ssdpDeviceType);

  char buffer[strlen_P(_ssdp_schema_template) + 1];
  strcpy_P(buffer, _ssdp_schema_template);
  server.client().printf(buffer, _root, ip.toString().c_str(), port, _deviceType, deviceId.c_str(),
      String(deviceIndex).c_str(), _friendlyName, _presentationURL, _serialNumber, _modelName,
      _modelNumber, _modelURL, _manufacturer, _manufacturerURL, _uuid, _serviceType, _serviceId,
      _SCPDURL, _controlURL, _eventSubURL);
#endif
}

void handleEvent(const String &ssdpDeviceType) {
#ifndef DISABLE_UPNP
  ESP8266WebServer &server = WebServer::getServer();
  Logs::serialPrintln(me, F("handleEvent:"), String(server.uri()));
  Logs::serialPrintln(me, String(server.method()) + FPSTR(": "), server.arg(F("plain")));

  // char buffer[strlen_P(_upnp_response_state_response_template_wemo) + 1];
  // strcpy_P(buffer, _upnp_response_state_response_template_wemo);
  // server.client().printf(buffer, 1);
#endif
}

void handleSubscribe(const char *message, const IPAddress &senderIP, const uint16_t senderPort) {
  // TODO
}

void handleDiscovery(const char *message, const IPAddress &senderIP, const uint16_t senderPort) {
#ifndef DISABLE_UPNP
  /* M-SEARCH * HTTP/1.1
     HOST: 239.255.255.250:1900
     MAN: "ssdp:discover"
     MX: 4
     ST: urn:schemas-upnp-org:device:bothouse

    M-SEARCH:M-SEARCH * HTTP/1.1
    Host: 239.255.255.250:1900
    Man: "ssdp:discover"
    MX: 3
    ST: ssdp:all
   */
  if (strstr_P(message, ST_SSDP_DEVICE_TYPE) != nullptr ||
      strstr_P(message, ST_SSDP_ALL) != nullptr) {
    Logs::serialPrintln(me, F("handleDiscovery:LOCALBOT:"), message);
    respondForEachDevice(senderIP, senderPort, FPSTR(DEVICE_TYPE_LOCALBOT));
  } else {
    // Logs::serialPrintln(me, F("handleDiscovery:NO_MATCH:"), message);
  }
#endif
}

void handleMessage(const char *message, const IPAddress &senderIP, const uint16_t senderPort) {
#ifndef DISABLE_UPNP
  if (!Mesh::isConnectedToWifi()) {
    // Logs::serialPrintln(me, F("handleMessage:NOWIFI"));
    return;
  }
  if (strlen(message) == 0) {
    // Logs::serialPrintln(me, F("handleMessage:EMPTY"));
    return;
  }

  const char MSEARCH[] = "M-SEARCH * HTTP/1.1";
  const char SUBSCRIBE[] = "SUBSCRIBE";
  if (strstr(message, MSEARCH) != nullptr) {
    // Logs::serialPrintln(me, F("handleMessage:handleDiscovery:"), message);
    handleDiscovery(message, senderIP, senderPort);
  } else if (strstr(message, SUBSCRIBE) != nullptr) {
    // Logs::serialPrintln(me, F("handleMessage:handleSubscribe:"), message);
    handleSubscribe(message, senderIP, senderPort);
  } else {
    Logs::serialPrintln(me, F("handleMessage:NOHANDLER:"), message);
  }
#endif
}

void advertise() {
#ifndef DISABLE_UPNP
  if (!Mesh::isConnectedToWifi())
    return;
    // respondForEachDevice(IPAddress(SSDP_MULTICAST_ADDR), SSDP_PORT);
    // TODO
#endif
}

/**********************************************************************/
bool isRunning = false;

void setup() {
#ifndef DISABLE_UPNP
  isRunning = true;
  IPAddress local = WiFi.localIP();
  IPAddress mcast(SSDP_MULTICAST_ADDR);
  igmp_leavegroup(local, mcast);
  igmp_joingroup(local, mcast);
  ssdpClient.begin(SSDP_PORT);
  advertise();
  // startSimpleDiscoveryProtocol();
  Logs::serialPrintln(me, F("UPNP is setup"));
#endif
}

void stop() {
  isRunning = false;
}

void handle() {
#ifndef DISABLE_UPNP
  if (isRunning) {
    Network::checkForUdpMessages(ssdpClient, handleMessage);
  }
#endif
}
//#endif
}  // namespace UniversalPlugAndPlay
