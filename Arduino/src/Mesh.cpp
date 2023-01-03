#include <ESP8266WiFi.h>
#include "AccessPoints.h"
#include "Config.h"
#include "Events.h"
#include "Hash.h"
#include "Logs.h"
#include "MessageGenerator.h"
#include "Network.h"
#include "Storage.h"
#include "Utils.h"

namespace Mesh {
  static const Logs::caller me = Logs::caller::Mesh;

  static const unsigned long NETWORK_SCAN_FREQUENCY_MILLIS = 1000 * 60 * 5;
  static const auto SCAN_TIMES_TOO_SOON_WHEN_LAST_NODE_UPDATE = 5;
  static const auto WIFI_SCAN_TIMES = 1;
  static const auto WIFI_MAX_FAILED_ATTEMPTS = 3;

  static uint32_t nextScanTimeMillis = 0;
  static int32_t _apLevel = 0;
  static bool _isAccessPoint = false;
  static bool _isConnectedToWifi = false;
  static bool _isMasterNode = false;
  static uint8_t _scanNetworksCounter = 0;

  static uint8_t _failedConsecutiveAttemptsConnectingToWiFi = 0;

  static Node* _nodesTip = NULL;

  bool isConnectedToWifi() {
    if (!WiFi.isConnected()) {
      return false;
    }
    Storage::storageStruct flashData = Storage::readFlash();
    if (flashData.wifiName == nullptr || WiFi.SSID() == nullptr) {
      return false;
    }
    String wifiSSID = String(flashData.wifiName);
    if (wifiSSID.equalsIgnoreCase(String(WiFi.SSID()))) {
      if (!_isConnectedToWifi) {
        _isConnectedToWifi = true;
        Events::onConnectedToWiFi();
      }
    }
    else if (_isConnectedToWifi) {
      _isConnectedToWifi = false;
      Events::onConnectedToAPNode();
    }
    return _isConnectedToWifi;
  }

  bool isAccessPointNode() {
    return _isAccessPoint;
  }

  bool isMasterNode() {
    return _isMasterNode;
  }

  void resetMasterWifiNode() {
    if (Mesh::isMasterNode()) {
      Events::onExitMasterWifiNode();
    }
    _isMasterNode = false;
  }

  int32_t getAPLevel() {
    return _apLevel;
  }

  void setAccessPointNode(bool isAccessPointNode) {
    _isAccessPoint = isAccessPointNode;
  }

  Node* getNodesTip() {
    return _nodesTip;
  }

  void setNodesTip(Node* tip) {
    _nodesTip = tip;
  }

  bool ICACHE_FLASH_ATTR isAccessPointAPotentialNode(const char* SSID) {
    if (strlen(SSID) == 0) {
      return false;
    }
    auto posUnderscore = String(SSID).lastIndexOf("_");
    if (posUnderscore < 0) {
      // Logs::serialPrintln(me, PSTR("isAccessPointAPotentialNode:noUnderscore:"), SSID);
      return false;
    }

    String lastString = String(SSID).substring(posUnderscore + 1);
    if (lastString.length() == 0) {
      // Logs::serialPrintln(me, PSTR("isAccessPointAPotentialNode:nothingAfterUnderscore:"),
      // lastString);
      return false;
    }
    if (lastString.toInt() == 0) {
      // Logs::serialPrintln(me, PSTR("isAccessPointAPotentialNode:noIntegerAfterUnderscore:"),
      // lastString);
      return false;
    }
    return true;
  }

  ICACHE_FLASH_ATTR Node getNodeInfo() {
    Storage::storageStruct flashData = Storage::readFlash();
    Node nodeInfo;
    Utils::sstrncpy(nodeInfo.deviceId, chipId.c_str(), MAX_LENGTH_DEVICE_ID);
    Utils::sstrncpy(nodeInfo.deviceName, Devices::getDeviceName(&flashData), MAX_LENGTH_DEVICE_NAME);
    Utils::sstrncpy(
      nodeInfo.wifiSSID, WiFi.isConnected() ? WiFi.SSID().c_str() : "", MAX_LENGTH_SSID);
    Utils::sstrncpy(nodeInfo.macAddress, WiFi.macAddress().c_str(), MAX_LENGTH_MAC);
#ifdef FORCE_MASTER_NODE
    nodeInfo.wifiRSSI = -5;
#else
    nodeInfo.wifiRSSI = WiFi.isConnected() ? WiFi.RSSI() : 0;
#endif
    nodeInfo.isMaster = isMasterNode();
    Utils::sstrncpy(nodeInfo.IPAddress, WiFi.localIP().toString().c_str(),
      MAX_LENGTH_IP);  // : WiFi.softAPIP().toString();
    Utils::sstrncpy(
      nodeInfo.apSSID, isAccessPointNode() ? WiFi.softAPSSID().c_str() : "", MAX_LENGTH_SSID);
    nodeInfo.apLevel = getAPLevel();
    nodeInfo.freeHeap = ESP.getFreeHeap();
    nodeInfo.systemTime = millis();
    nodeInfo.devices = Devices::getRootDevice();
    nodeInfo.accessPoints = AccessPoints::getAccessPointsList();
    return nodeInfo;
  }

  Node* findNodeInfo(const char* deviceId) {
    if (deviceId[0] == '\0') {
      return NULL;
    }
    Node* node = getNodesTip();
    while (node != nullptr) {
      if (strncmp(node->deviceId, deviceId, MAX_LENGTH_DEVICE_ID) == 0) {
        // Logs::serialPrintln(me, node->deviceId, PSTR("=="), deviceId);
        return node;
      }
      else {
        // Logs::serialPrintln(me, node->deviceId, PSTR("!="), deviceId);
      }
      node = node->next;
    }
    return NULL;
  }

  bool ICACHE_FLASH_ATTR updateExistingNodeInfo(Node nodeInfo) {
    Logs::serialPrintlnStart(me, PSTR("updateExistingNodeInfo"));
    Node* node = findNodeInfo(nodeInfo.deviceId);
    if (node == nullptr) {
      Logs::serialPrintlnEnd(me);
      return false;
    }
    Utils::sstrncpy(node->deviceName, nodeInfo.deviceName, MAX_LENGTH_DEVICE_NAME);
    node->lastUpdate = Utils::getNormailzedTime();
    node->systemTime = nodeInfo.systemTime;
    Logs::serialPrintlnEnd(me, PSTR("Updated"));
    return true;
  }

  /*******************************************************************/

  void ICACHE_FLASH_ATTR removeDeviceFromNodeList(const char* deviceId) {
    Node* prevNode = NULL;
    Node* node = getNodesTip();
    while (node != nullptr) {
      if (strncmp(node->deviceId, deviceId, MAX_LENGTH_DEVICE_ID) == 0) {
        break;
      }
      else {
        prevNode = node;
        node = node->next;
      }
    }
    if (node == nullptr || strncmp(node->deviceId, deviceId, MAX_LENGTH_DEVICE_ID) != 0) {
      Logs::serialPrintln(me, PSTR("[WARNING] findNode:NotFound:"), String(deviceId).c_str());
      return;
    }
    Logs::serialPrintln(me, PSTR("removeDeviceFromNodeList:"), String(deviceId).c_str());
    if (getNodesTip() == node) {
      _nodesTip = node->next;
    }
    else if (prevNode != nullptr) {
      prevNode->next = node->next;
    }
    else {
      Logs::serialPrintln(me, PSTR("[ERROR] removeDeviceFromNodeList:"), String(deviceId).c_str());
    }
    AccessPoints::deleteNodeAccessPoints(node->accessPoints);
    Devices::deleteDevices(node->devices);
    delete node;
  }

  void ICACHE_FLASH_ATTR purgeDevicesFromNodeList(uint16_t ifNotSeenForNumberOfScans) {
    unsigned long purgeIfLastUpdatedOlderThanMillis = 0;
    if (Utils::getNormailzedTime() > (NETWORK_SCAN_FREQUENCY_MILLIS * ifNotSeenForNumberOfScans)) {
      purgeIfLastUpdatedOlderThanMillis =
        Utils::getNormailzedTime() - (NETWORK_SCAN_FREQUENCY_MILLIS * ifNotSeenForNumberOfScans);
    }
    else {
      return;
    }

    Node* node = getNodesTip();
    Logs::serialPrintln(me, PSTR("Purging nodes not seen in last "),
      String(purgeIfLastUpdatedOlderThanMillis).c_str(), PSTR(" ms"));
    while (node != nullptr) {
      auto nextNode = node->next;
      if (node->lastUpdate < purgeIfLastUpdatedOlderThanMillis || ifNotSeenForNumberOfScans == 0) {
        long lastSeenMinutes = (Utils::getNormailzedTime() - node->lastUpdate);
        Logs::serialPrintln(me, PSTR("**** Purged: "), String(node->deviceId).c_str());
        Logs::serialPrint(
          me, PSTR(" -> Last Seen "), String(lastSeenMinutes).c_str(), PSTR(" ms ago"));
        removeDeviceFromNodeList(node->deviceId);
      }
      node = nextNode;
      yield();
    }
  }

  uint32_t _lastNodeUpdate = 0;
  bool _nodesListWasAppended = false;

  bool ICACHE_FLASH_ATTR updateOrAddNodeInfoList(Node& nodeInfo) {
    Logs::serialPrint(me, PSTR("updateOrAddNodeInfoList"));

    Mesh::Node* currentNode = Mesh::findNodeInfo(nodeInfo.deviceId);
    Mesh::Node* tip = Mesh::getNodesTip();
    bool dirty = false;
    bool appending = false;
    if (currentNode == nullptr) {
      Logs::serialPrint(me, PSTR(":Appending"));
      dirty = true;
      appending = true;
      currentNode = new Mesh::Node;
      currentNode->next = NULL;
      if (tip != nullptr) {
        currentNode->next = tip;
        Mesh::setNodesTip(currentNode);
      }
    }
    else {
      Logs::serialPrint(me, PSTR(":Updating"));
      // Check if anything with significance changed
      dirty = (currentNode->deviceName != nodeInfo.deviceName ||
        currentNode->isMaster != nodeInfo.isMaster || currentNode->apSSID != nodeInfo.apSSID ||
        currentNode->apLevel != nodeInfo.apLevel);
    }
    if (tip == nullptr) {
      Mesh::setNodesTip(currentNode);
    }

    // Updating existing
    Utils::sstrncpy(currentNode->deviceId, nodeInfo.deviceId, MAX_LENGTH_DEVICE_ID);
    Utils::sstrncpy(currentNode->deviceName, nodeInfo.deviceName, MAX_LENGTH_DEVICE_NAME);
    Utils::sstrncpy(currentNode->macAddress, nodeInfo.macAddress, MAX_LENGTH_MAC);
    Utils::sstrncpy(currentNode->IPAddress, nodeInfo.IPAddress, MAX_LENGTH_IP);
    currentNode->wifiRSSI = nodeInfo.wifiRSSI;
    Utils::sstrncpy(currentNode->wifiSSID, nodeInfo.wifiSSID, MAX_LENGTH_SSID);
    currentNode->isMaster = nodeInfo.isMaster;
    Utils::sstrncpy(currentNode->apSSID, nodeInfo.apSSID, MAX_LENGTH_SSID);
    currentNode->apLevel = nodeInfo.apLevel;
    currentNode->freeHeap = nodeInfo.freeHeap;
    currentNode->lastUpdate = Utils::getNormailzedTime();
    currentNode->systemTime = nodeInfo.systemTime;
    Devices::deleteDevices(currentNode->devices);
    currentNode->devices = nodeInfo.devices;
    AccessPoints::deleteNodeAccessPoints(currentNode->accessPoints);
    currentNode->accessPoints = nodeInfo.accessPoints;

    if (!dirty) {
      Logs::serialPrintln(me, PSTR(":Unchanged:"), String(nodeInfo.deviceId).c_str());
    }
    else if (strncmp(nodeInfo.deviceId, chipId.c_str(), MAX_LENGTH_DEVICE_ID) != 0) {
      if (appending) {
        Logs::serialPrintln(me, PSTR(":Appended:"), String(nodeInfo.deviceId).c_str());
        _nodesListWasAppended = true;
#ifndef DISABLE_MESH    
        Network::forceNetworkScan(10000);
#endif      
      }
      else {
        Logs::serialPrintln(me, PSTR(":Updated:"), String(nodeInfo.deviceId).c_str());
      }
      _lastNodeUpdate = millis();
    }
    else {
      Logs::serialPrintln(me, PSTR(":"), String(nodeInfo.deviceId).c_str());
    }
    return true;
  }

  /*******************************************************************/

  void ICACHE_FLASH_ATTR showMemoryContents() {
    Logs::serialPrintlnStart(me, PSTR("########MEMORY-INFO#########"));
    Logs::serialPrintln(me, PSTR("# Free heap: "), String(ESP.getFreeHeap()).c_str());
    Logs::serialPrintln(me, PSTR("# Fragmentation : "), String(ESP.getHeapFragmentation()).c_str());
    // Logs::serialPrintln(me, PSTR("# MaxFreeBlockSize: "), String(ESP.getMaxFreeBlockSize()));
    Logs::serialPrintln(me, PSTR("# SketchSize: "), String(ESP.getSketchSize()).c_str());
    Logs::serialPrintln(me, PSTR("# FreeSketchSpace: "), String(ESP.getFreeSketchSpace()).c_str());
    uint16_t accessPointListLength = 0;
    AccessPoints::AccessPointList* currNode = AccessPoints::getAccessPointsList();
    while (currNode != nullptr) {
      accessPointListLength++;
      currNode = currNode->next;
    }
    Logs::serialPrintln(me, PSTR("# accessPointListLength: "), String(accessPointListLength).c_str());

    // uint16_t logHistoryLength = 0;
    // Logs::LogHistory *logHistory = Logs::getLogHistoryQueue();
    // while (logHistory != nullptr) {
    //   logHistoryLength++;
    //   logHistory = logHistory->next;
    // }
    // Logs::serialPrintln(me, PSTR("# logHistoryLength: "), String(logHistoryLength).c_str());

    uint16_t meshNodeLength = 0;
    Node* meshNode = getNodesTip();
    while (meshNode != nullptr) {
      meshNodeLength++;
      AccessPoints::AccessPointList* aps = meshNode->accessPoints;
      while (aps != nullptr) {
        meshNodeLength++;
        aps = aps->next;
      }
      Devices::DeviceDescription* devices = meshNode->devices;
      while (devices != nullptr) {
        meshNodeLength++;
        devices = devices->next;
      }
      meshNode = meshNode->next;
    }
    Logs::serialPrintlnEnd(me, PSTR("# meshNodeLength: "), String(meshNodeLength).c_str());
  }

  void ICACHE_FLASH_ATTR showNodeInfo() {
    Storage::storageStruct flashData = Storage::readFlash();
    Logs::serialPrintlnStart(me, PSTR("########NODE-INFO#########"));
    Logs::serialPrintln(me, PSTR("# Device Name: "), String(flashData.deviceName).c_str());
    Logs::serialPrintln(me, PSTR("# Device ID: "), chipId.c_str());
    if (isAccessPointNode()) {
      String ssid((char*)0);
      getMySSID(ssid, flashData);
      Logs::serialPrintln(me, PSTR("# AP: "), ssid.c_str());
      Logs::serialPrint(me, PSTR(" / IP: "), WiFi.softAPIP().toString().c_str());
      Logs::serialPrint(me, PSTR(" / Level: "), String(getAPLevel()).c_str());
      uint8_t stations = Network::getConnectedStations();
      Logs::serialPrintln(me, PSTR("# Connected Clients: "), String(stations).c_str());
      // if (stations > 0) {
      //   struct station_info *stat_info;
      //   struct ip4_addr *IPaddress;
      //   IPAddress address;
      //   stat_info = wifi_softap_get_station_info();
      //   while (stat_info != nullptr) {
      //     IPaddress = &stat_info->ip;
      //     address = IPaddress->addr;
      //     Logs::serialPrintln(me, PSTR("# - "), address.toString().c_str());
      //     stat_info = STAILQ_NEXT(stat_info, next);
      //   }
      // }
    }

    // if (WiFi.status() == WL_CONNECTED) {
    // Logs::serialPrintln(me, PSTR("# WiFi Pwd: "), String(flashData.wifiPassword).c_str());

    if (isConnectedToWifi()) {
      Logs::serialPrintln(me, PSTR("# Connected to WiFi: "), String(flashData.wifiName).c_str());
    }
    if (WiFi.isConnected()) {
      Logs::serialPrintln(me, PSTR("# Connected to AP: "), String(WiFi.SSID()).c_str());
    }

    AccessPoints::showWifiScanInfo(true, true);
    AccessPoints::showWifiScanInfo(false, false);
    showMemoryContents();
    Logs::serialPrintlnEnd(me, PSTR("###########################"));
  }

  /*******************************************************************/

  void getSaltedMeshName(String& out, Storage::storageStruct& flashData) {
#ifdef ENABLE_NAT_ROUTER
    if (strlen(flashData.wifiName) <= 2 && strlen(flashData.wifiPassword) <= 2) {
#endif
      String hash = sha1(flashData.wifiName);
      if (hash.length() > 4) {
        hash = hash.substring(0, 3);
      }
      out = MESH_SSID_NAME;
      out += hash;
#ifdef ENABLE_NAT_ROUTER
    }
    else {
      out = flashData.wifiName;
    }
#endif
    out += FPSTR("_");
  }

  void getMySSID(String& out, Storage::storageStruct& flashData) {
    getSaltedMeshName(out, flashData);
    out += String(getAPLevel());
  }
  /*******************************************************************/
  bool ICACHE_FLASH_ATTR shouldStopAccessPoint(Storage::storageStruct& flashData,
    AccessPoints::AccessPointInfo* strongestAccessPoint,
    AccessPoints::AccessPointList* accessPointlists) {
    Logs::serialPrintlnStart(me, PSTR("shouldStopAccessPoint"));
    if (!isAccessPointNode()) {
      Logs::serialPrintlnEnd(me, PSTR("No Stopping AP: Node is not an AP"));
      return false;
    }
    if (strongestAccessPoint == nullptr) {
      Logs::serialPrintlnEnd(me, PSTR("No Stopping AP: No APs or WiFi found"));
      return false;
    }
    if (isConnectedToWifi() && getAPLevel() <= 0) {
      Logs::serialPrintlnEnd(me, PSTR("Stopping AP: AP Level is negative and I'm connected to WiFi"));
      return true;
    }
    if (Network::isAnyClientConnectedToThisAP()) {
      Logs::serialPrintlnEnd(me, PSTR("No Stopping AP: Found node connected to my AP"));
      return false;
    }
    AccessPoints::AccessPointList* currNode = accessPointlists;
    while (currNode != nullptr) {
      String mySsid((char*)0);
      getMySSID(mySsid, flashData);
      if (String(currNode->ap->SSID) == mySsid) {
        Logs::serialPrintlnEnd(me, PSTR("Stopping AP:Found other AP with same name as me"));
        return true;
      }
      currNode = currNode->next;
    }
    Logs::serialPrintlnEnd(me);
    return false;
  }
  /*******************************************************************/
  unsigned long _lastFailedConnectionAttempt = 0;
  unsigned long _firstFailedConnectionAttempt = millis();

  bool ICACHE_FLASH_ATTR tryConnectingToBetterAccessPoint(Storage::storageStruct& flashData) {
    Logs::serialPrintlnStart(me, PSTR("tryConnectingToBetterAccessPoint"));

    int32_t minAPLevel = isAccessPointNode() ? getAPLevel() : 0;
    AccessPoints::AccessPointConnectionInfo* wifiInfo = NULL;

    int32_t minSignalStrength = MINIMAL_SIGNAL_STRENGHT;
    if (WiFi.isConnected()) {
      minSignalStrength = WiFi.RSSI() + 10;
    }

    // Try an AP if signal is strong
    bool foundNoAps = true;
    AccessPoints::AccessPointInfo* strongestAccessPoint = NULL;
    if (!Events::isSafeMode()) {
      strongestAccessPoint = AccessPoints::getStrongestAccessPoint(NULL, 0, minAPLevel, 1, _firstFailedConnectionAttempt);
      // strongestAccessPoint = AccessPoints::getStrongestAccessPoint(accessPointHomeWifi,
      //     minSignalStrength, minAPLevel, WIFI_MAX_FAILED_ATTEMPTS, findNotFailedConnectingSince);
      if (strongestAccessPoint != nullptr) {
        foundNoAps = false;
      }
      else {
        _firstFailedConnectionAttempt = millis();
      }
    }

    if (strongestAccessPoint == nullptr) {
      Logs::serialPrintlnEnd(me, PSTR("Found no APs to connect to"));
      return false;
    }
    else if (!Utils::areBSSIDEqual(strongestAccessPoint->BSSID, WiFi.BSSID())) {
      Logs::serialPrintln(
        me, PSTR("Found close AP to connect to: "), String(strongestAccessPoint->SSID).c_str());
    }
    else if (WiFi.isConnected()) {
      Logs::serialPrintlnEnd(me, PSTR("Already connected to best AP"));
      return false;
    }

    // if (strongestAccessPoint == accessPointHomeWifi) {
    //   Network::connectToAP(accessPointHomeWifi->SSID, 
    //       flashData.wifiPassword, 
    //       accessPointHomeWifi->wifiChannel, 
    //       accessPointHomeWifi->BSSID); //0, NULL);
    // } else if (strlen(flashData.wifiPassword) == 0) {
    //   Logs::serialPrintln(me, PSTR("Can't connect to other APs unless a Mesh password is set"));
    // } else {
    Logs::serialPrintln(me, PSTR("Trying to connect to: "), Utils::getBSSIDStr(strongestAccessPoint->BSSID).c_str());
    int8_t status = Network::connectToAP(strongestAccessPoint->SSID,
      flashData.wifiPassword,
      strongestAccessPoint->wifiChannel,
      strongestAccessPoint->BSSID); // NULL);
    // }
    if (status == WL_WRONG_PASSWORD) {
      Logs::serialPrintln(me, PSTR("Starting AP due to WRONG WIFI PASSWORD"));
      Network::startAccessPoint(true);
    }

    wifiInfo = AccessPoints::getAccessPointInfo(strongestAccessPoint->BSSID);
    if (!WiFi.isConnected()) {
      if (foundNoAps) {
        wifiInfo->connectionAttempts = 1;
      }
      else {
        wifiInfo->connectionAttempts++;
      }
      wifiInfo->lastFailedConnection = millis();
      Logs::serialPrintln(
        me, PSTR("Times failed connecting: "), String(wifiInfo->connectionAttempts + 1).c_str());
    }
    else {
      Logs::serialPrintln(me, PSTR("Resetting times failed connecting as Wifi connected successfully"));
      _lastFailedConnectionAttempt = 0;
      wifiInfo->connectionAttempts = 0;
      wifiInfo->lastFailedConnection = 0;
    }
    Logs::serialPrintlnEnd(me);
    return foundNoAps;
  }
  /*******************************************************************/
  void ICACHE_FLASH_ATTR checkIfMasterNode() {
    //#ifndef ARDUINO_ESP8266_GENERIC
    bool newIsMasterNode = false;
    if (getAPLevel() == 1 || getAPLevel() == -1) {
      newIsMasterNode = true;
    }
    else if (isAccessPointNode() && getAPLevel() > 0 && getNodesTip() != nullptr) {
      // Find node with lowest level
      newIsMasterNode = true;
      Node* node = getNodesTip();
      String myDeviceId = chipId;
      while (node != nullptr) {
        if (node->apLevel > 0 && node->apLevel < getAPLevel()) {
          newIsMasterNode = false;
          break;
        }
        if (node->isMaster && isMasterNode() && String(node->deviceId) != myDeviceId) {
          newIsMasterNode = false;
          break;
        }
        node = node->next;
      }
    }
    if (isMasterNode() != newIsMasterNode) {
      _isMasterNode = newIsMasterNode;
      if (newIsMasterNode) {
        Events::onBecomingMasterWifiNode();
      }
      else {
        Events::onExitMasterWifiNode();
      }
    }
    //#endif
  }
  /*******************************************************************/
  // int32_t shouldLowerAPLevel() {
  //   uint8_t stations = Network::getConnectedStations();
  //   if (stations > 0) {
  //     return 0;
  //   }
  //   int32_t availableAPLevel = getNextLowestAPLevelAvailable(getAPLevel());
  //   if (availableAPLevel == 0 || availableAPLevel == getAPLevel()) {
  //     return 0;
  //   }
  //   // Change AP Level
  //   return availableAPLevel;
  // }
  /*******************************************************************/
  int32_t ICACHE_FLASH_ATTR calculateAccessPointLevel(AccessPoints::AccessPointList* accessPointList,
    AccessPoints::AccessPointInfo* strongestAccessPoint, uint32_t freeHeap) {
    Logs::serialPrintlnStart(me, PSTR("calculateAccessPointLevel"));
    if (freeHeap < MIN_HEAP_TO_BE_AP) {
      Logs::serialPrintlnEnd(me, PSTR("Not enough free Heap memory to be an access point"));
      return 0;
    }
    if (strongestAccessPoint == nullptr) {
      Logs::serialPrintlnEnd(me, PSTR("There are no APs around"));
      return -1;
    }

    if (strongestAccessPoint != nullptr && strongestAccessPoint->RSSI >= MINIMAL_SIGNAL_STRENGHT) {
      Logs::serialPrint(me, PSTR("Found close AP with strong signal -> "),
        String(strongestAccessPoint->SSID).c_str(), PSTR(" (RSSI "));
      Logs::serialPrintlnEnd(me, String(strongestAccessPoint->RSSI).c_str(), PSTR(")"));
      return 0;
    }

    int32_t nextLevel = 0;
    if (strongestAccessPoint != nullptr && strongestAccessPoint->isHomeWifi) {
      // Is WiFi the strongest signal
      Logs::serialPrintln(me, PSTR("WiFi Is closest AP"));
      nextLevel = 1;
    }
    else if (strongestAccessPoint != nullptr) {
      // There's another strong signal
      nextLevel = strongestAccessPoint->apLevel > 0 ? strongestAccessPoint->apLevel + 1
        : strongestAccessPoint->apLevel - 1;
      Logs::serialPrint(me, PSTR("Found a close AP -> "), String(strongestAccessPoint->SSID).c_str());
      Logs::serialPrintln(me, PSTR("("), String(strongestAccessPoint->apLevel).c_str(), PSTR(")"));
    }
    else {
      Logs::serialPrintlnEnd(me, PSTR("No wifi signals found"));
      return 0;
    }

    AccessPoints::AccessPointInfo* nextAccessPoint =
      getAccessPointAtLevel(nextLevel, accessPointList);
    if (nextAccessPoint == nullptr) {
      Logs::serialPrintlnEnd(me, PSTR("Found no AP in Level -> "), String(nextLevel).c_str());
      return nextLevel;
    }
    AccessPoints::AccessPointInfo* lastAccessPoint =
      AccessPoints::getAccessPointWithHighestLevel(accessPointList);
    if (lastAccessPoint != nullptr) {
      Logs::serialPrint(
        me, PSTR("Found weak AP with next AP Level -> "), String(lastAccessPoint->SSID).c_str());
      Logs::serialPrintlnEnd(me, PSTR("("), String(nextLevel).c_str(), PSTR(")"));
      if (lastAccessPoint->apLevel > 0) {
        return lastAccessPoint->apLevel + 1;
      }
      else {
        return lastAccessPoint->apLevel - 1;
      }
    }
    else {
      Logs::serialPrintlnEnd(me, PSTR("[ERROR]  This should never happen"));
      return 0;
    }
  }
  /*******************************************************************/

  bool ICACHE_FLASH_ATTR isThereABetterAccessPoint(Storage::storageStruct& flashData) {
    if (!isAccessPointNode()) {
      Logs::serialPrintln(me, PSTR("isThereABetterAccessPoint:false"));
      return false;
    }
    Logs::serialPrintlnStart(me, PSTR("isThereABetterAccessPoint"));
    // Find an AP with a level greater than mine
    Node* node = getNodesTip();
    Node* nextNode = NULL;
    Node* prevNode = NULL;
    while (node != nullptr) {
      Logs::serialPrint(me, PSTR("AP: "), String(node->deviceId).c_str());
      Logs::serialPrintln(me, PSTR(", Level: "), String(node->apLevel).c_str());
      if (String(node->deviceId) != chipId) {
        if (getAPLevel() > 0 && node->apLevel > getAPLevel() &&
          (nextNode == nullptr || nextNode->apLevel > node->apLevel)) {
          nextNode = node;
        }
        else if (getAPLevel() < 0 && node->apLevel < getAPLevel() &&
          (nextNode == nullptr || nextNode->apLevel < node->apLevel)) {
          nextNode = node;
        }
        else if (node->apLevel > 0 && node->apLevel < getAPLevel() &&
          (prevNode == nullptr || prevNode->apLevel < node->apLevel)) {
          prevNode = node;
        }
        else if (node->apLevel < 0 && node->apLevel > getAPLevel() &&
          (prevNode == nullptr || prevNode->apLevel > node->apLevel)) {
          prevNode = node;
        }
      }
      node = node->next;
      yield();
    }
    if (nextNode == nullptr) {
      Logs::serialPrintln(
        me, PSTR("Found no AP with level greater than: "), String(getAPLevel()).c_str());
    }
    else {
      // Logs::serialPrintln(me, PSTR("Found a better AP: "), String(nextNode->deviceId).c_str());
      // AccessPoints::AccessPointInfo* accessPointHomeWifi = AccessPoints::getAccessPointHomeWifi();
      // if (accessPointHomeWifi == nullptr || nextNode->wifiRSSI > accessPointHomeWifi->RSSI + 8) {
      Logs::serialPrintlnEnd(
        me, PSTR("Found an AP closer to the Wifi -> "), String(nextNode->deviceId).c_str());
      String deviceId = nextNode->deviceId;
      String message((char*)0);
      MessageGenerator::generateRawAction(
        message, F("setAPLevel"), deviceId, WiFi.softAPSSID(), String(getAPLevel()));
      Network::broadcastEverywhere(message.c_str());
      return true;
      // }
      // else {
      //   Logs::serialPrintln(me, PSTR("Its Wifi is not stronger"));
      // }
    }
    // The following is to ensure AP numbers are continguos
    if (prevNode != nullptr) {
      int32_t newApLevel = 0;
      if (prevNode->apLevel > 0 && prevNode->apLevel + 1 < getAPLevel()) {
        newApLevel = prevNode->apLevel + 1;
      }
      else if (prevNode->apLevel < 0 && prevNode->apLevel - 1 > getAPLevel()) {
        newApLevel = prevNode->apLevel - 1;
      }
      if (newApLevel != 0) {
        Logs::serialPrintln(me, PSTR("Lowering AP Number"));
        String message((char*)0);
        MessageGenerator::generateRawAction(
          message, F("setAPLevel"), chipId, WiFi.softAPSSID(), String(newApLevel));
        Network::broadcastToMyAPNodes(message.c_str());  // Disconnect any connected clients
        setAPLevel(newApLevel);
      }
    }
    Logs::serialPrintlnEnd(me);
    return false;
  }

  void ICACHE_FLASH_ATTR setAPLevel(int32_t apLevel) {
    Logs::serialPrintln(me, PSTR("setAPLevel:"), String(apLevel).c_str());
    if (apLevel == 0 || (_apLevel != apLevel && isAccessPointNode())) {
      Network::stopAccessPoint();
    }
    _apLevel = apLevel;
#ifdef DISABLE_AP
    if (_apLevel < 0) {
      Network::startAccessPoint();
      Network::forceNetworkScan(random(1000, 10000));
    }
#else
    if (_apLevel != 0) {
      Network::startAccessPoint();
      Network::forceNetworkScan(random(1000, 10000));
    }
#endif
  }

  void ICACHE_FLASH_ATTR postScanNetworkAnalysis(Storage::storageStruct& flashData) {
    Logs::serialPrintlnStart(me, PSTR("postScanNetworkAnalysis"));
    AccessPoints::AccessPointInfo* strongestAccessPoint = AccessPoints::getStrongestAccessPoint(NULL);
    bool forceScan = false;

    // Logs::serialPrintln(me, PSTR("Wifi.Status: "), Utils::getWifiStatusText(WiFi.status()));
    // if (!WiFi.isConnected() && WiFi.status() == WL_WRONG_PASSWORD) {
    //   Logs::serialPrintln(me, PSTR("Starting AP due to WRONG WIFI PASSWORD"));
    //   Network::startAccessPoint(true);
    // }
    // else 
    if (isThereABetterAccessPoint(flashData) ||
      shouldStopAccessPoint(flashData, strongestAccessPoint, AccessPoints::getAccessPointsList())) {
      Network::stopAccessPoint();
      forceScan = true;
    }
    else if (!isAccessPointNode()) {
      _apLevel = calculateAccessPointLevel(AccessPoints::getAccessPointsList(),
        strongestAccessPoint, ESP.getFreeHeap());
#ifdef DISABLE_AP
      if (_apLevel < 0) {
        Network::startAccessPoint();
        Network::forceNetworkScan(random(1000, 10000));
      }
#else
      if (_apLevel != 0) {
        Network::startAccessPoint();
        Network::forceNetworkScan(random(1000, 10000));
      }
#endif
      //#endif
      }

    checkIfMasterNode();

    if (tryConnectingToBetterAccessPoint(flashData)) {
      forceScan = true;
    }

    if (forceScan) {
      Network::forceNetworkScan(random(1000, 10000));
    }
    else {
      Network::scheduleNextScan();
    }
    Logs::serialPrintlnEnd(me);
    }

  /*******************************************************************/
  void ICACHE_FLASH_ATTR scanNetworksComplete(int numberOfNetworks) {
    if (_scanNetworksCounter == 0) {
      AccessPoints::deleteNodeAccessPoints(AccessPoints::getAccessPointsList());
      AccessPoints::setAccessPointsList(NULL);
    }
    Storage::storageStruct flashData = Storage::readFlash();
    AccessPoints::scanNetworksAnalysis(flashData, numberOfNetworks);

    if (_scanNetworksCounter < WIFI_SCAN_TIMES) {
      Logs::serialPrintln(me, PSTR("scanNetworksComplete: Scanning again"));
      WiFi.scanNetworks(true, false);
      _scanNetworksCounter++;
      return;
    }
    _scanNetworksCounter = 0;

    /**************************/
    postScanNetworkAnalysis(flashData);
    Events::onScanNetworksComplete();
  }

  }  // namespace Mesh

  // if that one is closer to the Wifi, then send message to that node ansking to change AP level

  // TODO: SHould someone else closer to the next AP be this AP node
  /*
      if (_nodesListWasAppended) {
        Logs::serialPrintln(me, PSTR("scanNetworksComplete:_nodesListWasAppended"));
        // Use knowledge from mesh to decide to wich AP to connect to
        // Find the closest AP with strongest WiFi signal
        _nodesListWasAppended = false;
        AccessPoints::AccessPointInfo *betterAccessPoint =
     getBetterAccessPoint(accessPointHomeWifi); if (betterAccessPoint != nullptr) {
          Network::connectToAP(betterAccessPoint->SSID, flashData.wifiPassword,
              betterAccessPoint->wifiChannel, betterAccessPoint->BSSID);
        }
      } else {
        Logs::serialPrintln(me, PSTR("scanNetworksComplete:!_nodesListWasAppended"));
      }
  */

  /*******************************************************************/
  // int32_t getNextLowestAPLevelAvailable(int32_t apLevel) {
  //   if (apLevel == 0) {
  //     return 0;
  //   }
  //   Node *node = getNodesTip();
  //   int32_t nextLowerstAPLevelAvailable = 0;
  //   while (node != nullptr) {
  //     if (apLevel > 0 && node->apLevel > nextLowerstAPLevelAvailable && node->apLevel < apLevel) {
  //       nextLowerstAPLevelAvailable = node->apLevel;
  //     } else if (apLevel < 0 && node->apLevel < nextLowerstAPLevelAvailable &&
  //                node->apLevel > apLevel) {
  //       nextLowerstAPLevelAvailable = node->apLevel;
  //     }
  //     node = node->next;
  //   }
  //   if (apLevel > 0 && apLevel > nextLowerstAPLevelAvailable + 1) {
  //     nextLowerstAPLevelAvailable++;
  //   }
  //   if (apLevel < 0 && apLevel < nextLowerstAPLevelAvailable - 1) {
  //     nextLowerstAPLevelAvailable--;
  //   }
  //   if (AccessPoints::getAccessPointAtLevel(
  //           nextLowerstAPLevelAvailable, AccessPoints::getAccessPointsList()) != nullptr) {
  //     nextLowerstAPLevelAvailable = 0;
  //   }
  //   return nextLowerstAPLevelAvailable;
  // }
  // AccessPoints::AccessPointInfo *getBetterAccessPoint(
  //     AccessPoints::AccessPointInfo *accessPointHomeWifi) {
  //   Logs::serialPrintlnStart(me, PSTR("getBetterAccessPoint"));
  //   int32_t minRRSSI =
  //       accessPointHomeWifi == nullptr ? MINIMAL_SIGNAL_STRENGHT : accessPointHomeWifi->RSSI;
  //   int32_t minAPLevel = isAccessPointNode() ? getAPLevel() : 0;
  //   AccessPoints::AccessPointInfo *nextStrongestAccessPoint =
  //       AccessPoints::getStrongestAccessPoint(NULL, minRRSSI, minAPLevel);
  //   if (nextStrongestAccessPoint == nullptr) {
  //     Logs::serialPrintln(me, PSTR("NoneFund"));
  //   }
  //   while (nextStrongestAccessPoint != nullptr) {
  //     Node *node = getNodesTip();
  //     while (node != nullptr && node->apSSID != nextStrongestAccessPoint->SSID) {
  //       node = node->next;
  //     }
  //     if (node != nullptr) {
  //       if (node->wifiRSSI > minRRSSI) {
  //         Logs::serialPrint(
  //             me, F("Found closer AP with stronger Wifi connection: "), node->apSSID, F(" "));
  //         Logs::serialPrint(me, String(node->wifiRSSI).c_str(), PSTR("db to "), node->wifiSSID);
  //         Logs::serialPrintlnEnd(me, PSTR(" vs "), String(minRRSSI), PSTR("db"));
  //         return nextStrongestAccessPoint;
  //       } else {
  //         Logs::serialPrint(
  //             me, F("Close AP has Weaker wifi connection than "), String(minRRSSI), F("db: "));
  //         Logs::serialPrint(me, nextStrongestAccessPoint->SSID);
  //         Logs::serialPrintln(me, PSTR(" ("), String(node->wifiRSSI).c_str(), PSTR("db)"));
  //       }
  //     }
  //     minRRSSI = nextStrongestAccessPoint->RSSI;
  //     nextStrongestAccessPoint =
  //         AccessPoints::getStrongestAccessPoint(nextStrongestAccessPoint, minRRSSI, minAPLevel);
  //   }
  //   Logs::serialPrintlnEnd(me);
  //   return NULL;
  // }

  // void consider() {
  //   if (isAccessPointNode()) {
  //     int32_t availableAPLevel = shouldLowerAPLevel();
  //     if (availableAPLevel != 0) {
  //       Logs::serialPrintln(me, PSTR("scanNetworksComplete: Changing AP level from "),
  //           String(getAPLevel()) + FPSTR(" to "), String(availableAPLevel));
  //       _apLevel = availableAPLevel;
  //       Network::stopAccessPoint();
  //       Network::startAccessPoint();
  //       _lastNodeUpdate = millis();
  //     }
  //   }
  // }
