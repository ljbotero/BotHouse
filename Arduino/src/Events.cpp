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

  static const Logs::caller me = Logs::caller::Events;

  static uint32_t _keepSafeModeUntil = 0;
  static bool wasSafeMode = false;
  static unsigned long setupModeLastInvoked = 0;
  static unsigned long setupModeStartedTime = 0;
  static uint8_t setupModePushCount = 0;
  static bool setupModeLastState = false;

  static unsigned long systemEventNextReportedMillis = 0;
  static bool systemEventNextReportedState = false;

  bool isInSetupMode() {
    bool newSetupModeState = setupModeStartedTime > 0 && (setupModeStartedTime + SETUP_MODE_MILLIS_DURATION) > millis();
    if (setupModeLastState != newSetupModeState && !newSetupModeState) {
      // Exit Setup Mode
      Logs::serialPrintln(me, PSTR("### SETUP MODE ENDED ###"));
      Network::stopAccessPoint();
      if (Mesh::isConnectedToWifi()) {
        Devices::appendSystemEvent(Devices::SystemEvent::SYSTEM_CONNECTING_ON);
      }
    }
    setupModeLastState = newSetupModeState;
    return newSetupModeState;
  }

  void setSetupMode() {
    if (millis() < SETUP_MODE_MILLIS_BETWEEN_PUSHES) {
      Logs::serialPrintln(me, PSTR("### SETUP MODE TOO SOON ###"));
      return;
    }
    if (isInSetupMode()) {
      auto remainingMillis = (setupModeStartedTime + SETUP_MODE_MILLIS_DURATION) - millis();
      Logs::serialPrint(me, PSTR("### ALREADY IN SETUP MODE ("));
      Logs::serialPrintln(me, String(remainingMillis / 1000).c_str(), PSTR(" seconds remaining) ###"));
      return;
    }
    if (setupModeLastInvoked < (millis() - SETUP_MODE_MILLIS_BETWEEN_PUSHES)) {
      setupModeLastInvoked = millis();
      setupModePushCount = 0;
      return;
    }
    else {
      setupModeLastInvoked = millis();
      setupModePushCount++;
    }

    if (setupModePushCount + 1 >= SETUP_MODE_NUMBER_OF_CONSECUTIVE_PUSHES) {
      Logs::serialPrintln(me, PSTR("### SETUP MODE STARTED ###"));
      setupModeStartedTime = millis();
      Network::startAccessPoint(true);
    }
    else {
      Logs::serialPrint(me, PSTR("### SETUP MODE PUSH COUNT: "), String(setupModePushCount).c_str());
      Logs::serialPrintln(me, PSTR(" of "), String(SETUP_MODE_NUMBER_OF_CONSECUTIVE_PUSHES).c_str(), PSTR(" ###"));
    }
  }

  void setSafeMode() {
    _keepSafeModeUntil = millis() + MILLIS_TO_RUN_SAFE_MODE_FOR;
  }

  void systemEvents() {
    if (systemEventNextReportedMillis < millis() && isInSetupMode()) {
      Devices::appendSystemEvent(systemEventNextReportedState ?
        Devices::SystemEvent::SETUP_MODE_ON : Devices::SystemEvent::SETUP_MODE_OFF);        
      systemEventNextReportedMillis = millis() + SETUP_MODE_REPORTING_FREQUENCY_MILLIS;
      systemEventNextReportedState = !systemEventNextReportedState;
    }
    else if (systemEventNextReportedMillis < millis()) {
      Devices::appendSystemEvent(systemEventNextReportedState ?
        Devices::SystemEvent::HEARTBEAT_ON : Devices::SystemEvent::HEARTBEAT_OFF);        
      if (Mesh::isConnectedToWifi()) {
        systemEventNextReportedMillis = systemEventNextReportedState 
          ? millis() + HEARTBEAT_REPORTING_FREQUENCY_MILLIS
          : millis() + HEARTBEAT_REPORTING_DURATION_MILLIS; 
      } else {
        systemEventNextReportedMillis = systemEventNextReportedState 
          ? millis() + HEARTBEAT_REPORTING_DURATION_MILLIS
          : millis() + HEARTBEAT_REPORTING_FREQUENCY_MILLIS; 
      }      
      systemEventNextReportedState = !systemEventNextReportedState;
    }
  }

  bool isSafeMode() {
#ifndef DISABLE_SAFE_MODE
    bool isSafeM = _keepSafeModeUntil > millis();
    if (wasSafeMode && !isSafeM) {
      Logs::serialPrintln(me, PSTR("### SAFE MODE ENDED ###"));
    }
    wasSafeMode = isSafeM;
    return isSafeM;
#else
    return false;
#endif
  }

  void onCriticalLoop() {
  #ifdef RESTART_EVERY
    // Restart one a day
    // Disable in router: Enable IGMP Snooping
    if (millis() > 1000 * 60 * 60 * RESTART_EVERY) {
      Logs::serialPrintlnEnd(me, PSTR("Restarting once a day"));
      Devices::restart();
    }
  #endif
    yield();
    MessageProcessor::handle();
#ifndef DISABLE_WEBSERVER
    WebServer::handle();
#endif
    Network::handle();
    Logs::handle();
    systemEvents();
    if (isSafeMode()) {
      return;
    }
    HubsIntegration::handle();
    if (Mesh::isMasterNode()) {
      //#ifndef ARDUINO_ESP8266_GENERIC
      UniversalPlugAndPlay::handle();
      //#endif
      MDns::handle();
    }
  }

  void ICACHE_FLASH_ATTR onStartingAccessPoint() {
    Logs::serialPrintlnStart(me, PSTR("onStartingAccessPoint"));
    Mesh::setAccessPointNode(true);
#ifndef DISABLE_WEBSERVER
    WebServer::setup();
#endif
    Logs::serialPrintlnEnd(me);
  }

  void ICACHE_FLASH_ATTR onStoppingAccessPoint() {
    Logs::serialPrintlnStart(me, PSTR("onStoppingAccessPoint"));
    Mesh::setAccessPointNode(false);
    Mesh::resetMasterWifiNode();
    Logs::serialPrintlnEnd(me);
  }

  void ICACHE_FLASH_ATTR onConnectedToAPNode() {
    Logs::serialPrintlnStart(me, PSTR("onConnectedToAPNode"));
    Logs::setup();
    String message((char*)0);
    MessageGenerator::generateRawAction(message, F("requestSharedInfo"));
    Network::broadcastEverywhere(message.c_str());
#ifndef DISABLE_WEBSERVER
    WebServer::setup();
#endif
    Logs::serialPrintlnEnd(me);
  }

  void ICACHE_FLASH_ATTR onBecomingMasterWifiNode() {
    Logs::serialPrintlnStart(me, PSTR("onBecomingMasterWifiNode"));
#ifndef DISABLE_WEBSERVER
    WebServer::setup();
#endif
    //#ifndef ARDUINO_ESP8266_GENERIC
    UniversalPlugAndPlay::setup();
    //#endif
    MDns::start();
    Logs::serialPrintlnEnd(me);
  }

  void ICACHE_FLASH_ATTR onExitMasterWifiNode() {
    Logs::serialPrintlnStart(me, PSTR("onExitMasterWifiNode"));
    MDns::stop();
    //#ifndef ARDUINO_ESP8266_GENERIC
    UniversalPlugAndPlay::stop();
    //#endif
    String message((char*)0);
    MessageGenerator::generateRawAction(message, F("onExitMasterWifiNode"));
    Network::broadcastEverywhere(message.c_str());
    Logs::serialPrintlnEnd(me);
  }

  void ICACHE_FLASH_ATTR onScanNetworksComplete() {
    if (isSafeMode()) {
      return;
    }
    Logs::serialPrintlnStart(me, PSTR("onScanNetworksComplete"));
    Mesh::purgeDevicesFromNodeList();
    Mesh::showNodeInfo();
    if (ESP.getMaxFreeBlockSize() <= LOWEST_MEMORY_POSSIBLE_BEFORE_REBOOT) {
      Logs::serialPrintlnEnd(me, PSTR("Max free block size is dangerously low!!!!!!!!!!"));
      Devices::restart();
      return;
    }

    // String json = MessageGenerator::generateRawAction(F("pollDevices"));
    // Network::broadcastEverywhere(json, true, false);
    Mesh::Node nodeInfo = Mesh::getNodeInfo();
    String deviceInfo((char*)0);
    MessageGenerator::generateDeviceInfo(deviceInfo, nodeInfo, F("deviceInfo"));
    Network::broadcastEverywhere(deviceInfo.c_str(), true, false);

    String message((char*)0);
    MessageGenerator::generateRawAction(message, F("heartbeat"), chipId.c_str());
    Network::broadcastEverywhere(message.c_str(), true, true);
    Logs::serialPrintlnEnd(me);
  }

  void ICACHE_FLASH_ATTR onStartingWebServer(ESP8266WebServer& server) {
    Logs::serialPrintlnStart(me, PSTR("onStartingWebServer"));
    OTAupdates::setupWebServer(server);
    HubsIntegration::setup();
    Logs::serialPrintlnEnd(me);
  }

  void ICACHE_FLASH_ATTR onConnectedToWiFi() {
    Logs::serialPrintlnStart(me, PSTR("onConnectedToWiFi"));
    Logs::setup();
    String message((char*)0);
    MessageGenerator::generateRawAction(message, PSTR("requestSharedInfo"));
    Network::broadcastEverywhere(message.c_str());
#ifndef DISABLE_WEBSERVER
    WebServer::setup();
#endif
    Logs::serialPrintlnEnd(me);
  }

  void onReceivedUdpMessage(
    const char* message, const IPAddress& senderIP, const uint16_t senderPort) {
    if (isSafeMode()) {
      return;
    }
    Logs::pauseLogging(true);
    Logs::serialPrintlnStart(
      me, PSTR("onReceivedUdpMessage: "), String(strlen(message)).c_str(), PSTR(" bytes"));
    // Logs::serialPrintln(me, PSTR("Msg: "), message);
    Logs::pauseLogging(false);
    MessageProcessor::processMessage(message, senderIP, senderPort);
    Logs::serialPrintlnEnd(me);
  }

  bool onDeviceEvent(const Devices::DeviceState& state) {
    // Check if this device is subscribed to this event
    Devices::processEventsFromOtherDevices(state);
    return HubsIntegration::postDeviceEvent(state);
  }

  bool ICACHE_FLASH_ATTR onWebServerNotFound(ESP8266WebServer& server) {
    String path = ESP8266WebServer::urlDecode(server.uri());
    //#ifndef ARDUINO_ESP8266_GENERIC
    if (path.endsWith(FPSTR(UniversalPlugAndPlay::SSDP_DESCRIPTION_PATH))) {
      UniversalPlugAndPlay::handleDescription(FPSTR(UniversalPlugAndPlay::DEVICE_TYPE_LOCALBOT));
      return true;
    }

    //#endif
    return false;
  }

}  // namespace Events