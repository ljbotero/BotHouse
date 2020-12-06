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
const Logs::caller me = Logs::caller::Mesh;

const uint32_t NETWORK_SCAN_FREQUENCY_MILLIS = 1000 * 60 * 5;
const uint32_t SCAN_TIMES_TOO_SOON_WHEN_LAST_NODE_UPDATE = 5;
const uint8_t WIFI_SCAN_TIMES = 3;
const uint8_t WIFI_MAX_FAILED_ATTEMPTS = 3;

uint32_t nextScanTimeMillis = 0;
int32_t _apLevel = 0;
bool _isAccessPoint = false;
bool _isConnectedToWifi = false;
bool _isMasterNode = false;
uint8_t _scanNetworksCounter = 0;

uint8_t _failedConsecutiveAttemptsConnectingToWiFi = 0;

Node *_nodesTip = NULL;

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
  } else if (_isConnectedToWifi) {
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

Node *getNodesTip() {
  return _nodesTip;
}

void setNodesTip(Node *tip) {
  _nodesTip = tip;
}

bool isAccessPointAPotentialNode(String &ssid) {
  if (ssid.isEmpty()) {
    return false;
  }
  uint16_t posUnderscore = ssid.lastIndexOf("_");
  if (posUnderscore < 0) {
    // Logs::serialPrintln(me, F("isAccessPointAPotentialNode:noUnderscore:"), ssid);
    return false;
  }

  String lastString = ssid.substring(posUnderscore + 1);
  if (lastString.length() == 0) {
    // Logs::serialPrintln(me, F("isAccessPointAPotentialNode:nothingAfterUnderscore:"),
    // lastString);
    return false;
  }
  if (lastString.toInt() == 0) {
    // Logs::serialPrintln(me, F("isAccessPointAPotentialNode:noIntegerAfterUnderscore:"),
    // lastString);
    return false;
  }
  return true;
}

Node getNodeInfo() {
  Storage::storageStruct flashData = Storage::readFlash();
  Node nodeInfo;
  nodeInfo.deviceId = Utils::getChipIdString();
  nodeInfo.deviceName = Devices::getDeviceName(&flashData);
  nodeInfo.wifiSSID = WiFi.isConnected() ? WiFi.SSID() : "";
  nodeInfo.wifiRSSI = WiFi.isConnected() ? WiFi.RSSI() : 0;
  nodeInfo.isMaster = isMasterNode();
  nodeInfo.IPAddress = WiFi.localIP().toString();  // : WiFi.softAPIP().toString();
  nodeInfo.apSSID = isAccessPointNode() ? WiFi.softAPSSID() : "";
  nodeInfo.apLevel = getAPLevel();
  nodeInfo.freeHeap = ESP.getFreeHeap();
  nodeInfo.devices = Devices::getRootDevice();
  nodeInfo.accessPoints = AccessPoints::getAccessPointsList();
  return nodeInfo;
}

Node *findNodeInfo(const String &deviceId) {
  if (deviceId.isEmpty()) {
    return NULL;
  }
  Node *node = getNodesTip();
  while (node != nullptr) {
    if (node->deviceId == deviceId) {
      // Logs::serialPrintln(me, node->deviceId, F("=="), deviceId);
      return node;
    } else {
      // Logs::serialPrintln(me, node->deviceId, F("!="), deviceId);
    }
    node = node->next;
  }
  return NULL;
}

bool updateExistingNodeInfo(Node nodeInfo) {
  Logs::serialPrintlnStart(me, F("updateExistingNodeInfo"));
  Node *node = findNodeInfo(nodeInfo.deviceId);
  if (node == nullptr) {
    Logs::serialPrintlnEnd(me);
    return false;
  }
  node->deviceName = nodeInfo.deviceName;
  node->lastUpdate = Utils::getNormailzedTime();
  Logs::serialPrintlnEnd(me, F("Updated"));
  return true;
}

/*******************************************************************/
void removeDeviceFromNodeList(const String &deviceId) {
  Node *prevNode = NULL;
  Node *node = getNodesTip();
  while (node != nullptr) {
    if (node->deviceId == deviceId) {
      break;
    } else {
      prevNode = node;
      node = node->next;
    }
  }
  if (node == nullptr || node->deviceId != deviceId) {
    return;
  }
  if (getNodesTip() == node) {
    _nodesTip = node->next;
  } else {
    prevNode->next = node->next;
  }
  AccessPoints::deleteNodeAccessPoints(node->accessPoints);
  Devices::deleteDevices(node->devices);
  delete node;
  Logs::serialPrintln(me, F("removeDeviceFromNodeList:"), deviceId);
}

void purgeDevicesFromNodeList(uint16_t ifNotSeenForNumberOfScans) {
  unsigned long purgeIfLastUpdatedOlderThanMillis =
      Utils::getNormailzedTime() - (NETWORK_SCAN_FREQUENCY_MILLIS * ifNotSeenForNumberOfScans);
  // To prevent pruning nodes that have not synched times
  unsigned long dontPurgeIfLastUpdatedOlderThanMillis =
      Utils::getNormailzedTime() - (NETWORK_SCAN_FREQUENCY_MILLIS * ifNotSeenForNumberOfScans * 2);

  Node *node = getNodesTip();
  Node *prevNode = NULL;
  Node *nextNode = NULL;
  while (node != nullptr) {
    if ((node->lastUpdate > dontPurgeIfLastUpdatedOlderThanMillis &&
            node->lastUpdate < purgeIfLastUpdatedOlderThanMillis) ||
        ifNotSeenForNumberOfScans == 0) {
      if (prevNode == NULL) {
        _nodesTip = node->next;
      } else {
        prevNode->next = node->next;
      }
      long lastSeenMinutes = (Utils::getNormailzedTime() - node->lastUpdate) / 60000;
      Logs::serialPrintln(me, String(FPSTR("**** Purged: ")) + String(node->deviceId),
          String(FPSTR(" -> Last Seen ")) + String(lastSeenMinutes), F(" minutes ago"));
      if (node->accessPoints != AccessPoints::getAccessPointsList()) {
        AccessPoints::deleteNodeAccessPoints(node->accessPoints);
      }
      Devices::deleteDevices(node->devices);
      if (node == getNodesTip()) {
        _nodesTip = NULL;
      }
      nextNode = node->next;
      delete node;
      node = nextNode;
    } else {
      prevNode = node;
      node = node->next;
    }
  }
}

uint32_t _lastNodeUpdate = 0;
bool _nodesListWasAppended = false;

bool updateOrAddNodeInfoList(Node &nodeInfo) {
  Logs::serialPrint(me, F("updateOrAddNodeInfoList"));

  Mesh::Node *currentNode = Mesh::findNodeInfo(nodeInfo.deviceId);
  Mesh::Node *tip = Mesh::getNodesTip();
  bool dirty = false;
  bool appending = false;
  if (currentNode == nullptr) {
    Logs::serialPrint(me, F(":Appending"));
    dirty = true;
    appending = true;
    currentNode = new Mesh::Node;
    currentNode->next = NULL;
    if (tip != nullptr) {
      currentNode->next = tip;
      Mesh::setNodesTip(currentNode);
    }
  } else {
    Logs::serialPrint(me, F(":Updating"));
    // Check if anything with significance changed
    dirty = (currentNode->deviceName != nodeInfo.deviceName ||
             currentNode->isMaster != nodeInfo.isMaster || currentNode->apSSID != nodeInfo.apSSID ||
             currentNode->apLevel != nodeInfo.apLevel);
  }
  if (tip == nullptr) {
    Mesh::setNodesTip(currentNode);
  }

  // Updating existing
  currentNode->deviceId = nodeInfo.deviceId;
  currentNode->deviceName = nodeInfo.deviceName;
  currentNode->IPAddress = nodeInfo.IPAddress;
  currentNode->wifiRSSI = nodeInfo.wifiRSSI;
  currentNode->wifiSSID = nodeInfo.wifiSSID;
  currentNode->isMaster = nodeInfo.isMaster;
  currentNode->apSSID = nodeInfo.apSSID;
  currentNode->apLevel = nodeInfo.apLevel;
  currentNode->freeHeap = nodeInfo.freeHeap;
  currentNode->lastUpdate = Utils::getNormailzedTime();
  Devices::deleteDevices(currentNode->devices);
  currentNode->devices = nodeInfo.devices;
  AccessPoints::deleteNodeAccessPoints(currentNode->accessPoints);
  currentNode->accessPoints = nodeInfo.accessPoints;

  if (!dirty) {
    Logs::serialPrintln(me, F(":Unchanged:"), nodeInfo.deviceId);
  } else if (nodeInfo.deviceId != Utils::getChipIdString()) {
    if (appending) {
      Logs::serialPrintln(me, F(":Appended:"), nodeInfo.deviceId);
      _nodesListWasAppended = true;
      Network::forceNetworkScan(10000);
    } else {
      Logs::serialPrintln(me, F(":Updated:"), nodeInfo.deviceId);
    }
    _lastNodeUpdate = millis();
  }
  return true;
}

/*******************************************************************/

void showNodeInfo() {
  Storage::storageStruct flashData = Storage::readFlash();
  Logs::serialPrintln(me, F("########NODE-INFO#########"));
  Logs::serialPrintln(me, F("# Free heap: "), String(ESP.getFreeHeap()));
  Logs::serialPrintln(me, F("# Device Name: "), flashData.deviceName);
  Logs::serialPrintln(me, F("# Device ID: "), Utils::getChipIdString());
  Logs::serialPrintln(me, F("# Mesh: "), flashData.meshName);
  // Logs::serialPrintln(me, F("#       "), flashData.meshPassword);
  if (isAccessPointNode()) {
    Logs::serialPrint(me, F("# AP: "), getMySSID(flashData));
    Logs::serialPrint(me, F(" / IP: "), WiFi.softAPIP().toString());
    Logs::serialPrintln(me, F(" / Level: "), String(getAPLevel()));
    uint8_t stations = Network::getConnectedStations();
    Logs::serialPrintln(me, F("# Connected Clients: "), String(stations));
    if (stations > 0) {
      struct station_info *stat_info;
      struct ip4_addr *IPaddress;
      IPAddress address;
      stat_info = wifi_softap_get_station_info();
      while (stat_info != nullptr) {
        IPaddress = &stat_info->ip;
        address = IPaddress->addr;
        Logs::serialPrintln(me, F("# - "), address.toString());
        stat_info = STAILQ_NEXT(stat_info, next);
      }
    }
  }

  // if (WiFi.status() == WL_CONNECTED) {
  if (isConnectedToWifi()) {
    Logs::serialPrintln(me, F("# Connected to WiFi: "), flashData.wifiName);
  }
  if (WiFi.isConnected()) {
    Logs::serialPrintln(me, F("# Connected to AP: "), WiFi.SSID());
  }

  AccessPoints::showWifiScanInfo(true, true);
  AccessPoints::showWifiScanInfo(false, false);
  Logs::serialPrintln(me, F("###########################"));
}

/*******************************************************************/

String getSaltedMeshName(Storage::storageStruct &flashData) {
  String meshName = String(getMeshName(flashData));
  String meshPassword = String(flashData.meshPassword);
  if (meshPassword.length() == 0) {
    meshPassword = meshName;
  }
  // Logs::serialPrintln(me, FPSTR("MeshPass:") + String(flashData.meshPassword));
  String hash = sha1(meshPassword);
  if (hash.length() > 4) {
    hash = hash.substring(0, 3);
  }
  // Logs::serialPrintln(me, FPSTR("hash:") + hash);
  // Logs::serialPrintln(me, FPSTR("meshName:") + meshName);
  meshName.replace("_", "");
  return meshName + hash + '_';
}

char *getMeshName(Storage::storageStruct &flashData) {
  size_t meshNameLen = strlen(flashData.meshName);
  if (meshNameLen == 0) {
    return MESH_SSID_NAME;
  }
  return flashData.meshName;
}

String getMySSID(Storage::storageStruct &flashData) {
  return getSaltedMeshName(flashData) + String(getAPLevel());
}
/*******************************************************************/
bool shouldStopAccessPoint(Storage::storageStruct &flashData,
    AccessPoints::AccessPointInfo *strongestAccessPoint,
    AccessPoints::AccessPointList *accessPointlists) {
  Logs::serialPrintlnStart(me, F("shouldStopAccessPoint"));
  if (AccessPoints::getAccessPointHomeWifi() == nullptr && strongestAccessPoint == nullptr) {
    Logs::serialPrintlnEnd(me, F("No APs or WiFi found"));
    return false;
  }
  if (isConnectedToWifi() && getAPLevel() <= 0) {
    Logs::serialPrintlnEnd(me, F("AP Level is negative and I'm connected to WiFi"));
    return true;
  }
  if (Network::isAnyClientConnectedToThisAP()) {
    Logs::serialPrintlnEnd(me, F("Found node connected to my AP"));
    return false;
  }
  AccessPoints::AccessPointList *currNode = accessPointlists;
  while (currNode != nullptr) {
    if (currNode->ap->SSID == getMySSID(flashData)) {
      Logs::serialPrintlnEnd(me, F("Found other AP with same name as me"));
      return true;
    }
    currNode = currNode->next;
  }
  Logs::serialPrintlnEnd(me);
  return false;
}
/*******************************************************************/
int32_t getNextLowestAPLevelAvailable(int32_t apLevel) {
  if (apLevel == 0) {
    return 0;
  }
  Node *node = getNodesTip();
  int32_t nextLowerstAPLevelAvailable = 0;
  while (node != nullptr) {
    if (apLevel > 0 && node->apLevel > nextLowerstAPLevelAvailable && node->apLevel < apLevel) {
      nextLowerstAPLevelAvailable = node->apLevel;
    } else if (apLevel < 0 && node->apLevel < nextLowerstAPLevelAvailable &&
               node->apLevel > apLevel) {
      nextLowerstAPLevelAvailable = node->apLevel;
    }
    node = node->next;
  }
  if (apLevel > 0 && apLevel > nextLowerstAPLevelAvailable + 1) {
    nextLowerstAPLevelAvailable++;
  }
  if (apLevel < 0 && apLevel < nextLowerstAPLevelAvailable - 1) {
    nextLowerstAPLevelAvailable--;
  }
  if (AccessPoints::getAccessPointAtLevel(
          nextLowerstAPLevelAvailable, AccessPoints::getAccessPointsList()) != nullptr) {
    nextLowerstAPLevelAvailable = 0;
  }
  return nextLowerstAPLevelAvailable;
}
/*******************************************************************/
uint32_t _lastFailedConnectionAttempt = 0;

bool tryConnectingToAnAccessPoint(
    Storage::storageStruct &flashData, AccessPoints::AccessPointInfo *accessPointHomeWifi) {
  Logs::serialPrintlnStart(me, F("tryConnectingToAnAccessPoint"));

  int32_t minRRSSI = MINIMAL_SIGNAL_STRENGHT;
  int32_t minAPLevel = isAccessPointNode() ? getAPLevel() : 0;
  uint32_t findNotFailedConnectingSince = millis() - (NETWORK_SCAN_FREQUENCY_MILLIS * 3);
  AccessPoints::AccessPointInfo *strongestAccessPoint = NULL;
  // Try wifi first
  AccessPoints::AccessPointConnectionInfo *wifiInfo =
      AccessPoints::getAccessPointInfo(flashData.wifiName);
  if (wifiInfo != nullptr && accessPointHomeWifi != nullptr &&
      (wifiInfo->connectionAttempts < WIFI_MAX_FAILED_ATTEMPTS ||
          wifiInfo->lastFailedConnection <= findNotFailedConnectingSince)) {
    strongestAccessPoint = accessPointHomeWifi;
  } else {
    strongestAccessPoint = AccessPoints::getStrongestAccessPoint(
        NULL, minRRSSI, minAPLevel, WIFI_MAX_FAILED_ATTEMPTS, findNotFailedConnectingSince);
  }

  if (strongestAccessPoint == nullptr) {
    Logs::serialPrintlnEnd(me, F("Found no APs to connect to"));
    return true;
  }

  if (strongestAccessPoint == accessPointHomeWifi) {
    Network::connectToAP(accessPointHomeWifi->SSID, flashData.wifiPassword, 0, NULL);
  } else if (strlen(flashData.meshPassword) == 0) {
    Logs::serialPrintln(me, F("Can't connect to other APs unless a Mesh password is set"));
  } else {
    Network::connectToAP(strongestAccessPoint->SSID, flashData.meshPassword,
        strongestAccessPoint->wifiChannel, strongestAccessPoint->BSSID);
  }

  wifiInfo = AccessPoints::getAccessPointInfo(strongestAccessPoint->SSID);
  if (!WiFi.isConnected()) {
    if (wifiInfo->lastFailedConnection <= findNotFailedConnectingSince) {
      wifiInfo->connectionAttempts = 0;
    } else {
      wifiInfo->connectionAttempts++;
    }
    _lastFailedConnectionAttempt = millis();
    wifiInfo->lastFailedConnection = _lastFailedConnectionAttempt;
    Logs::serialPrintlnEnd(
        me, F("Times failed connecting: "), String(wifiInfo->connectionAttempts + 1));
    return false;
  } else {
    _lastFailedConnectionAttempt = 0;
    wifiInfo->connectionAttempts = 0;
    wifiInfo->lastFailedConnection = 0;
  }
  Logs::serialPrintlnEnd(me);
  return true;
}
/*******************************************************************/
void checkIfMasterNode() {
//#ifndef ARDUINO_ESP8266_GENERIC
  bool newIsMasterNode = false;
  if (getAPLevel() == 1 || getAPLevel() == -1) {
    newIsMasterNode = true;
  } else if (isAccessPointNode() && getAPLevel() > 0 && getNodesTip() != nullptr) {
    // Find node with lowest level
    newIsMasterNode = true;
    Node *node = getNodesTip();
    String myDeviceId = Utils::getChipIdString();
    while (node != nullptr) {
      if (node->apLevel > 0 && node->apLevel < getAPLevel()) {
        newIsMasterNode = false;
        break;
      }
      if (node->isMaster && isMasterNode() && node->deviceId != myDeviceId) {
        newIsMasterNode = false;
        break;
      }
      node = node->next;
    }
  }
#ifdef FORCE_MASTER_NODE
  newIsMasterNode = true;
#endif
  if (isMasterNode() != newIsMasterNode) {
    _isMasterNode = newIsMasterNode;
    if (newIsMasterNode) {
      Events::onBecomingMasterWifiNode();
    } else {
      Events::onExitMasterWifiNode();
    }
  }
//#endif
}
/*******************************************************************/
int32_t shouldLowerAPLevel() {
  uint8_t stations = Network::getConnectedStations();
  if (stations > 0) {
    return 0;
  }
  int32_t availableAPLevel = getNextLowestAPLevelAvailable(getAPLevel());
  if (availableAPLevel == 0 || availableAPLevel == getAPLevel()) {
    return 0;
  }
  // Change AP Level
  return availableAPLevel;
}
/*******************************************************************/
int32_t calculateAccessPointLevel(AccessPoints::AccessPointList *accessPointList,
    AccessPoints::AccessPointInfo *strongestAccessPoint,
    AccessPoints::AccessPointInfo *accessPointHomeWifi, uint32_t freeHeap) {
  Logs::serialPrintlnStart(me, F("calculateAccessPointLevel"));
  if (freeHeap < MIN_HEAP_TO_BE_AP) {
    Logs::serialPrintlnEnd(me, F("Not enough free Heap memory to be an access point"));
    return 0;
  }
  if (strongestAccessPoint == nullptr && accessPointHomeWifi == nullptr) {
    Logs::serialPrintlnEnd(me, F("There are no APs around"));
    return -1;
  }

  if (strongestAccessPoint != nullptr && strongestAccessPoint->RSSI >= MINIMAL_SIGNAL_STRENGHT) {
    Logs::serialPrintlnEnd(me, F("Found close AP with strong signal -> "),
        strongestAccessPoint->SSID,
        String(FPSTR(" (RSSI ")) + String(strongestAccessPoint->RSSI) + String(FPSTR(")")));
    return 0;
  }

  int32_t nextLevel = 0;
  if (accessPointHomeWifi != nullptr &&
      (strongestAccessPoint == nullptr ||
          accessPointHomeWifi->RSSI >= strongestAccessPoint->RSSI)) {
    // Is WiFi the strongest signal
    Logs::serialPrintln(me, F("WiFi Is closest AP"));
    nextLevel = 1;
  } else if (strongestAccessPoint != nullptr) {
    // There's another strong signal
    nextLevel = strongestAccessPoint->apLevel > 0 ? strongestAccessPoint->apLevel + 1
                                                  : strongestAccessPoint->apLevel - 1;
    Logs::serialPrintln(me, F("Found a close AP -> "),
        strongestAccessPoint->SSID + "(" + String(strongestAccessPoint->apLevel) + ")");
  } else {
    return 0;
  }

  AccessPoints::AccessPointInfo *nextAccessPoint =
      getAccessPointAtLevel(nextLevel, accessPointList);
  if (nextAccessPoint == nullptr) {
    Logs::serialPrintlnEnd(me, F("Found no AP in Level -> "), String(nextLevel));
    return nextLevel;
  }
  AccessPoints::AccessPointInfo *lastAccessPoint =
      AccessPoints::getAccessPointWithHighestLevel(accessPointList);
  if (lastAccessPoint != nullptr) {
    Logs::serialPrintlnEnd(me, F("Found weak AP with next AP Level -> "), lastAccessPoint->SSID,
        "(" + String(nextLevel) + ")");
    return lastAccessPoint->apLevel + 1;
  } else {
    Logs::serialPrintlnEnd(me, F("ERROR: This should never happen"));
    return 0;
  }
}
/*******************************************************************/
AccessPoints::AccessPointInfo *getBetterAccessPoint(
    AccessPoints::AccessPointInfo *accessPointHomeWifi) {
  Logs::serialPrintlnStart(me, F("getBetterAccessPoint"));
  int32_t minRRSSI =
      accessPointHomeWifi == nullptr ? MINIMAL_SIGNAL_STRENGHT : accessPointHomeWifi->RSSI;
  int32_t minAPLevel = isAccessPointNode() ? getAPLevel() : 0;
  AccessPoints::AccessPointInfo *nextStrongestAccessPoint =
      AccessPoints::getStrongestAccessPoint(NULL, minRRSSI, minAPLevel);
  if (nextStrongestAccessPoint == nullptr) {
    Logs::serialPrintln(me, F("NoneFund"));
  }
  while (nextStrongestAccessPoint != nullptr) {
    Node *node = getNodesTip();
    while (node != nullptr && node->apSSID != nextStrongestAccessPoint->SSID) {
      node = node->next;
    }
    if (node != nullptr) {
      if (node->wifiRSSI > minRRSSI) {
        Logs::serialPrint(
            me, F("Found closer AP with stronger Wifi connection: "), node->apSSID, F(" "));
        Logs::serialPrint(me, String(node->wifiRSSI), F("db to "), node->wifiSSID);
        Logs::serialPrintlnEnd(me, F(" vs "), String(minRRSSI), F("db"));
        return nextStrongestAccessPoint;
      } else {
        Logs::serialPrint(
            me, F("Close AP has Weaker wifi connection than "), String(minRRSSI), F("db: "));
        Logs::serialPrint(me, nextStrongestAccessPoint->SSID);
        Logs::serialPrintln(me, F(" ("), String(node->wifiRSSI), F("db)"));
      }
    }
    minRRSSI = nextStrongestAccessPoint->RSSI;
    nextStrongestAccessPoint =
        AccessPoints::getStrongestAccessPoint(nextStrongestAccessPoint, minRRSSI, minAPLevel);
  }
  Logs::serialPrintlnEnd(me);
  return NULL;
}

/*******************************************************************/
void scanNetworksComplete(int numberOfNetworks) {
  if (_scanNetworksCounter == 0) {
    removeDeviceFromNodeList(Utils::getChipIdString());
  }
  Storage::storageStruct flashData = Storage::readFlash();
  AccessPoints::scanNetworksAnalysis(flashData, numberOfNetworks);
  if (_scanNetworksCounter < WIFI_SCAN_TIMES) {
    WiFi.scanNetworks(true, false);
    _scanNetworksCounter++;
    return;
  }
  _scanNetworksCounter = 0;

  bool isTooSoonToMakeAnyDrasticChanges =
      _lastNodeUpdate == 0 ||
      (millis() - _lastNodeUpdate) <
          (NETWORK_SCAN_FREQUENCY_MILLIS * SCAN_TIMES_TOO_SOON_WHEN_LAST_NODE_UPDATE);

  bool wasAccessPoint = isAccessPointNode();
  AccessPoints::AccessPointInfo *accessPointHomeWifi = AccessPoints::getAccessPointHomeWifi();
  AccessPoints::AccessPointInfo *strongestAccessPoint =
      AccessPoints::getStrongestAccessPoint(accessPointHomeWifi);

  if (isAccessPointNode()) {
    if (shouldStopAccessPoint(
            flashData, strongestAccessPoint, AccessPoints::getAccessPointsList())) {
      Network::stopAccessPoint();
      Network::forceNetworkScan(random(1000, 5000));
    } 
//#ifndef ARDUINO_ESP8266_GENERIC    
    else if (!isTooSoonToMakeAnyDrasticChanges) {
      int32_t availableAPLevel = shouldLowerAPLevel();
      if (availableAPLevel != 0) {
        Logs::serialPrintln(me, F("scanNetworksComplete: Changing AP level from "),
            String(getAPLevel()) + FPSTR(" to "), String(availableAPLevel));
        _apLevel = availableAPLevel;
        Network::stopAccessPoint();
        Network::startAccessPoint();
        _lastNodeUpdate = millis();
      }
    }
//#endif
  }

  if (!isAccessPointNode()) {
    Logs::serialPrintln(me, F("scanNetworksComplete:!isAccessPointNode"));
    _apLevel = calculateAccessPointLevel(AccessPoints::getAccessPointsList(), strongestAccessPoint,
        accessPointHomeWifi, ESP.getFreeHeap());
//#ifndef ARDUINO_ESP8266_GENERIC
    if (_apLevel != 0) {
      Network::startAccessPoint();
    }
//#endif
  }

  if (!WiFi.isConnected()) {
    Logs::serialPrintln(me, F("scanNetworksComplete:!WiFi.isConnected"));
    if (!tryConnectingToAnAccessPoint(flashData, accessPointHomeWifi)) {
      Network::forceNetworkScan(0);
      return;
    }
    _nodesListWasAppended = true;
  } else if (_nodesListWasAppended) {
    Logs::serialPrintln(me, F("scanNetworksComplete:_nodesListWasAppended"));
    // Use knowledge from mesh to decide to wich AP to connect to
    // Find the closest AP with strongest WiFi signal
    _nodesListWasAppended = false;
    AccessPoints::AccessPointInfo *betterAccessPoint = getBetterAccessPoint(accessPointHomeWifi);
    if (betterAccessPoint != nullptr) {
      Network::connectToAP(betterAccessPoint->SSID, flashData.meshPassword,
          betterAccessPoint->wifiChannel, betterAccessPoint->BSSID);
    }
  } else {
    Logs::serialPrintln(me, F("scanNetworksComplete:!_nodesListWasAppended"));
  }

  if (isAccessPointNode() && isConnectedToWifi() && getAPLevel() < 0) {
    Network::stopAccessPoint();
    Network::forceNetworkScan(0);
  } else if (isAccessPointNode() != wasAccessPoint) {
    Network::forceNetworkScan(0);
  } else {
    checkIfMasterNode();
    Network::scheduleNextScan();
    Events::onScanNetworksComplete();
  }
}

}  // namespace Mesh