#include "Events.h"
#include <ESP8266WebServer.h>
#include "HubsIntegration.h"
#include "Logs.h"
#include "MDns.h"
#include "Mesh.h"
#include "MessageGenerator.h"
#include "MessageProcessor.h"
#include "Network.h"
#include "OTAupdates.h"
#include "UniversalPlugAndPlay.h"
#include "Utils.h"
#include "WebServer.h"

namespace Events {

const Logs::caller me = Logs::caller::Events;

void onCriticalLoop() {
  // yield();
  WebServer::handle();
  Network::handle();
  HubsIntegration::handle();
  if (Mesh::isMasterNode()) {
    //#ifndef ARDUINO_ESP8266_GENERIC
    UniversalPlugAndPlay::handle();
    //#endif
    MDns::handle();
  }
}

void onStartingAccessPoint() {
  Logs::serialPrintlnStart(me, F("onStartingAccessPoint"));
  Mesh::setAccessPointNode(true);
  WebServer::setup();
  Logs::serialPrintlnEnd(me);
}

void onStoppingAccessPoint() {
  Logs::serialPrintlnStart(me, F("onStoppingAccessPoint"));
  Mesh::setAccessPointNode(false);
  Mesh::resetMasterWifiNode();
  Logs::serialPrintlnEnd(me);
}

void onConnectedToAPNode() {
  Logs::serialPrintlnStart(me, F("onConnectedToAPNode"));
  String message = MessageGenerator::generateRawAction(F("requestSharedInfo"));
  Network::broadcastMessage(message);
  WebServer::setup();
  Logs::serialPrintlnEnd(me);
}

void onBecomingMasterWifiNode() {
  Logs::serialPrintlnStart(me, F("onBecomingMasterWifiNode"));
  WebServer::setup();
  //#ifndef ARDUINO_ESP8266_GENERIC
  UniversalPlugAndPlay::setup();
  //#endif
  MDns::start();
  Logs::serialPrintlnEnd(me);
}

void onExitMasterWifiNode() {
  Logs::serialPrintlnStart(me, F("onExitMasterWifiNode"));
  MDns::stop();
  //#ifndef ARDUINO_ESP8266_GENERIC
  UniversalPlugAndPlay::stop();
  //#endif
  String message = MessageGenerator::generateRawAction(F("onExitMasterWifiNode"));
  Network::broadcastMessage(message);
  Logs::serialPrintlnEnd(me);
}

void onScanNetworksComplete() {
  Logs::serialPrintlnStart(me, F("onScanNetworksComplete"));
  Mesh::showNodeInfo();
  Mesh::purgeDevicesFromNodeList();
  // String json = MessageGenerator::generateRawAction(F("pollDevices"));
  // Network::broadcastMessage(json, true, false);
  Mesh::Node nodeInfo = Mesh::getNodeInfo();
  String deviceInfo = MessageGenerator::generateDeviceInfo(nodeInfo, F("deviceInfo"));
  Network::broadcastMessage(deviceInfo, true, false);

  String message = MessageGenerator::generateRawAction(F("heartbeat"), Utils::getChipIdString());
  Network::broadcastMessage(message, true, true);
  Logs::serialPrintlnEnd(me);
}

void onStartingWebServer(ESP8266WebServer &server) {
  Logs::serialPrintln(me, F("onStartingWebServer"));
  HubsIntegration::setup();
  OTAupdates::setupWebServer(server);
}

void onConnectedToWiFi() {
  Logs::serialPrintlnStart(me, F("onConnectedToWiFi"));
  String message = MessageGenerator::generateRawAction(F("requestSharedInfo"));
  Network::broadcastMessage(message);
  WebServer::setup();
  Logs::serialPrintlnEnd(me);
}

void onReceivedUdpMessage(
    const char *message, const IPAddress &senderIP, const uint16_t senderPort) {
  Logs::pauseLogging(true);
  Logs::serialPrintlnStart(me, F("onReceivedUdpMessage: "), String(strlen(message)), F(" bytes"));
  // Logs::serialPrintln(me, F("Msg: "), message);
  Logs::pauseLogging(false);
  MessageProcessor::processMessage(message, senderIP, senderPort);
  Logs::serialPrintlnEnd(me);
}

bool onDeviceEvent(const Devices::DeviceState &state) {
  return HubsIntegration::postDeviceEvent(state);
}

bool onWebServerNotFound(ESP8266WebServer &server) {
  String path = server.uri();
  //#ifndef ARDUINO_ESP8266_GENERIC
  if (path.endsWith(FPSTR(UniversalPlugAndPlay::SSDP_DESCRIPTION_PATH))) {
    UniversalPlugAndPlay::handleDescription(FPSTR(UniversalPlugAndPlay::DEVICE_TYPE_LOCALBOT));
    return true;
  }

  //#endif
  return false;
}

}  // namespace Events