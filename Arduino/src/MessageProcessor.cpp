#include "MessageProcessor.h"
#include <limits.h>
#include "Config.h"
#include "Devices.h"
#include "Events.h"
#include "HubsIntegration.h"
#include "Logs.h"
#include "MDns.h"
#include "Mesh.h"
#include "MessageGenerator.h"
#include "Network.h"
#include "Storage.h"
#include "Utils.h"

namespace MessageProcessor {

static const Logs::caller me = Logs::caller::MessageProcessor;

void ICACHE_FLASH_ATTR processDeviceInfoReport(const JsonObject &content) {
  Mesh::Node newNode;
  newNode.buildNumber = content[F("buildNumber")];
  Utils::sstrncpy(newNode.deviceId, content[F("deviceId")], MAX_LENGTH_DEVICE_ID);
  Utils::sstrncpy(
      newNode.deviceName, content[F("deviceName")], MAX_LENGTH_DEVICE_NAME);
  Utils::sstrncpy(newNode.macAddress, content[F("macAddress")], MAX_LENGTH_MAC);
  Utils::sstrncpy(newNode.wifiSSID, content[F("wifiSSID")], MAX_LENGTH_SSID);
  newNode.wifiRSSI = content[F("wifiRSSI")];
  newNode.isMaster = content[F("isMaster")];
  Utils::sstrncpy(newNode.IPAddress, content[F("IPAddress")], MAX_LENGTH_IP);
  Utils::sstrncpy(newNode.apSSID, content[F("apSSID")], MAX_LENGTH_SSID);
  newNode.apLevel = content[F("apLevel")];
  newNode.freeHeap = content[F("freeHeap")];
  newNode.systemTime = content[F("systemTime")];
  JsonArray devices = content[F("devices")].as<JsonArray>();
  newNode.devices = Devices::deserializeDevices(devices);
  JsonArray accessPoints = content[F("accessPoints")].as<JsonArray>();
  AccessPoints::AccessPointList *currAp = NULL;
  newNode.accessPoints = NULL;
  for (JsonObject accessPoint : accessPoints) {
    if (currAp == nullptr) {
      currAp = new AccessPoints::AccessPointList;
      newNode.accessPoints = currAp;
    } else {
      currAp->next = new AccessPoints::AccessPointList;
      currAp = currAp->next;
    }
    currAp->next = NULL;
    currAp->ap = new AccessPoints::AccessPointInfo;
    Utils::sstrncpy(currAp->ap->SSID, accessPoint[F("SSID")], MAX_LENGTH_SSID);
    currAp->ap->isRecognized = accessPoint[F("isRecognized")].as<bool>();
    currAp->ap->isHomeWifi = accessPoint[F("isHomeWifi")].as<bool>();
    currAp->ap->isOpen = accessPoint[F("isOpen")].as<bool>();
    currAp->ap->RSSI = accessPoint[F("RSSI")];
  }
  Mesh::updateOrAddNodeInfoList(newNode);
}

bool ICACHE_FLASH_ATTR processUpdateDevice(const JsonObject &content) {
  // Process nodes list
  Logs::serialPrintlnStart(me, PSTR("processUpdateDevice"));
  Mesh::Node nodeInfo;
  Utils::sstrncpy(nodeInfo.deviceId, content[F("deviceId")], MAX_LENGTH_DEVICE_ID);
  Utils::sstrncpy(nodeInfo.deviceName, content[F("deviceName")], MAX_LENGTH_DEVICE_NAME);
  if (strncmp(nodeInfo.deviceId, chipId.c_str(), MAX_LENGTH_DEVICE_ID) == 0) {
    Devices::updateDevice(nodeInfo.deviceName);
  }
  Logs::serialPrintlnEnd(me);
  return Mesh::updateExistingNodeInfo(nodeInfo);
}

bool processDeviceEvent(const JsonObject &content) {
  Logs::serialPrintlnStart(me, PSTR("processDeviceEvent"));
  // Process nodes list
  Devices::DeviceState state;
  Utils::sstrncpy(state.deviceId, content[F("deviceId")], MAX_LENGTH_UUID);
  Logs::serialPrint(me, PSTR("deviceId: "), String(state.deviceId).c_str());
  state.deviceIndex = content[F("deviceIndex")];
  Logs::serialPrint(me, PSTR(", deviceIndex: "), String(state.deviceIndex).c_str());
  Utils::sstrncpy(state.deviceTypeId, content[F("deviceTypeId")], MAX_LENGTH_DEVICE_TYPE_ID);
  Logs::serialPrint(me, PSTR(", typeId: "), state.deviceTypeId);
  Utils::sstrncpy(state.eventName, content[F("eventName")], MAX_LENGTH_EVENT_NAME);
  Logs::serialPrint(me, PSTR(", eventName: "), state.eventName);
  state.eventValue = content[F("eventValue")];
  Logs::serialPrintln(me, PSTR(", eventValue: "), String(state.eventValue).c_str());
  Logs::serialPrintlnEnd(me);
  return Events::onDeviceEvent(state);
}

bool ICACHE_FLASH_ATTR processSharedInfo(const JsonObject &doc) {
  Logs::serialPrintlnStart(me, PSTR("processMeshInfo"));
  // Process nodes list
  JsonObject info = doc[F("info")];
  uint32_t timestamp = info[F("time")];
  unsigned long myTime = Utils::getNormailzedTime();
  // Only adjust time if the other node has a greather timestamp
  if (myTime - 200 < timestamp) {
    Utils::setTimeOffset(timestamp - myTime);
  }
  JsonObject storage = doc[F("storage")];
  uint32_t version = storage[F("version")];
  Storage::storageStruct flashData = Storage::readFlash();
  if (flashData.version < version) {
    Logs::serialPrintln(me, PSTR("remoteStorage[version]="), String(version).c_str());
    Logs::serialPrintln(me, PSTR("myStorage[version]    ="), String(flashData.version).c_str());
    Logs::serialPrintln(me, PSTR("Updating flash data"));
    flashData.version = version;
    flashData.state = storage[F("state")];
    bool changed1 = false; //Utils::copyStringFromJson(flashData.wifiName, storage, F("wifiName"));
    bool changed2 = false; //Utils::copyStringFromJson(flashData.wifiPassword, storage, F("wifiPassword"));
    bool changed3 = Utils::copyStringFromJson(flashData.hubApi, storage, F("hubApi"));
    bool changed4 = Utils::copyStringFromJson(flashData.hubToken, storage, F("hubToken"));
    bool changed5 = Utils::copyStringFromJson(flashData.hubNamespace, storage, F("hubNamespace"));
    // if (strlen(flashData.wifiName) <= 2 && strlen(flashData.wifiPassword) <= 2) {
    //   Logs::serialPrintln(me, PSTR("[WARNING] Refusing to update from blank data"));
    // } else {
      Storage::writeFlash(flashData, false);
      if (changed1 || changed2 || changed3 || changed4 || changed5) {
        Devices::restart();
      }
    // }
  }
  Logs::serialPrintlnEnd(me);
  return true;
}

bool ICACHE_FLASH_ATTR processRequestSharedInfo(const IPAddress &sender) {
  if (!Mesh::isMasterNode()) {
    Logs::serialPrintln(me, PSTR("requestSharedInfo:cannot Process Request Shared Info since this is not the master node"));
    return false;
  }

  String sharedInfo((char *)0);
  MessageGenerator::generateSharedInfo(sharedInfo);
  if (!sender.isSet()) {
    Logs::serialPrintln(me, PSTR("requestSharedInfo:broadcasting"));
    Network::broadcastEverywhere(sharedInfo.c_str());
  } else {
    Logs::serialPrintln(me, PSTR("requestSharedInfo:replyingTo:"), sender.toString().c_str());
    Network::sendUdpMessage(sender, BROADCAST_PORT, sharedInfo.c_str());
  }
  return true;
}

bool ICACHE_FLASH_ATTR processAddWifiDevice(const char *SSID) {
  AccessPoints::AccessPointList *currNode = AccessPoints::getAccessPointsList();
  while (currNode != nullptr) {
    if (strncmp(SSID, currNode->ap->SSID, MAX_LENGTH_SSID) == 0) {
      break;
    }
    currNode = currNode->next;    
  }
  if (currNode == nullptr) {
  // if (!AccessPoints::isAccessPointInRange(SSID)) {
    return false;
  }
  String sharedInfo((char *)0);
  MessageGenerator::generateSharedInfo(sharedInfo);

  Logs::serialPrintln(me, PSTR("Disconnecting from WiFi"));
  WiFi.disconnect(false);
  Logs::serialPrintln(me, PSTR("Connecting to AP: "), SSID);
  int8_t connected = Network::connectToAP(SSID, F(""), 0, NULL);
  if (connected == WL_CONNECTED) {
    // send node info
    Logs::serialPrintln(me, PSTR("Broadcasting mesh info"));
    Network::broadcastEverywhere(sharedInfo.c_str());
    delay(3000);
  } else {
    Logs::serialPrintln(me, PSTR("Failed connecting to access point"));
  }
  Logs::serialPrintln(me, PSTR("Disconnecting from AP"));
  WiFi.disconnect(false);
  return true;
}

bool pollDevices = false;

bool processMessage(
    const char *message, const IPAddress &sender, const uint16_t senderPort, bool propagateMessage) {
  // Process nodes list
  Logs::serialPrintlnStart(me, PSTR("processMessage"));
  if (message == nullptr || strlen(message) <= 4) {
    Logs::serialPrintlnEnd(me);
    return false;
  }

  DynamicJsonDocument doc(1024 * 4);
  auto error = deserializeJson(doc, message);
  if (error != DeserializationError::Ok) {
    Logs::serialPrint(me, PSTR("[ERROR]  Could not deserialize Json for message: "));
    Logs::serialPrintln(me, message);
    doc.clear();
    Logs::serialPrintlnEnd(me);
    return false;
  }
  doc.shrinkToFit();
  String action = doc[F("action")].as<String>();
  if (action == nullptr) {
    Logs::serialPrint(me, PSTR("[ERROR]  Could not deserialize action for Json message: "));
    Logs::serialPrintln(me, message);
    doc.clear();
    Logs::serialPrintlnEnd(me);
    return false;
  }

  bool handled = false;

  const char* actionChar = action.c_str();
  if (strcmp_P(actionChar, PSTR("addWifiDevice")) == 0) {
    const char *SSID = doc[F("data")];
    handled = processAddWifiDevice(SSID);
  } else if (strcmp_P(actionChar, PSTR("setAPLevel")) == 0) {
#ifndef DISABLE_MESH    
    bool changedNetwork = false;
    String deviceId = doc[F("deviceId")].as<String>();
    String apSSID = doc[F("deviceIndex")].as<String>();
    String apLevelStr = doc[F("data")].as<String>();
    if (WiFi.SSID() == apSSID) {
      WiFi.disconnect();
      changedNetwork = true;
    }
    Logs::serialPrint(me, PSTR("setAPLevel:"), String(deviceId).c_str());
    Logs::serialPrintln(me, PSTR(":"), apLevelStr.c_str());
    if (chipId.equalsIgnoreCase(deviceId)) {
      handled = true;
      propagateMessage = false;
      int apLevel = apLevelStr.toInt();
      Mesh::setAPLevel(apLevel);
      changedNetwork = true;
    }
    if (changedNetwork) {
      Network::forceNetworkScan(random(1000, 5000));
    }
#endif
  } else if (strcmp_P(actionChar, PSTR("requestSharedInfo")) == 0) {
    handled = processRequestSharedInfo(sender);
  } else if (strcmp_P(actionChar, PSTR("sharedInfo")) == 0) {
    Logs::serialPrintln(me, PSTR("processMessage:sharedInfo"));
    JsonObject content = doc[F("content")];
    processSharedInfo(content);
  } else if (strcmp_P(actionChar, PSTR("deviceEvent")) == 0) {
    JsonObject content = doc[F("content")];
    handled = processDeviceEvent(content);
    if (handled) {
      Logs::serialPrintln(me, PSTR("processMessage:deviceEvent:handled"));
    } else {
      Logs::serialPrintln(me, PSTR("processMessage:deviceEvent:unhandled"));
    }
  } else if (strcmp_P(actionChar, PSTR("deviceInfo")) == 0) {
    Logs::serialPrintln(me, PSTR("processMessage:deviceInfo"));
    JsonObject content = doc[F("content")];
    processDeviceInfoReport(content);
  } else if (strcmp_P(actionChar, PSTR("heartbeat")) == 0) {
    Logs::serialPrintln(me, PSTR("processMessage:heartbeat"));
    const char *deviceId = doc[F("deviceId")];
    handled = HubsIntegration::sendHeartbeat(deviceId);
  } else if (strcmp_P(actionChar, PSTR("updateDevice")) == 0) {
    JsonObject content = doc[F("content")];
    handled = processUpdateDevice(content);
  } else if (strcmp_P(actionChar, PSTR("pollDevices")) == 0) {
    handled = true;
    pollDevices = true;
  } else if (strcmp_P(actionChar, PSTR("pingDevices")) == 0) {
    // Logs::serialPrintln(me, PSTR("Pong"));
    Logs::logEspInfo();
    Mesh::showNodeInfo();
    handled = true;
  } else if (strcmp_P(actionChar, PSTR("handleDeviceCommand"))  == 0) {
    Logs::serialPrintln(me, PSTR("processMessage:handleDeviceCommand"));
    String deviceId = doc[F("deviceId")].as<String>();
    if (chipId.equalsIgnoreCase(deviceId)) {
      propagateMessage = false;
      String deviceNumberStr = doc[F("deviceIndex")].as<String>();
      int deviceIndex = deviceNumberStr.toInt();
      String commandName = doc[F("data")].as<String>();
      Devices::DeviceDescription *device =
          Devices::getDeviceFromIndex(Devices::getRootDevice(), deviceIndex);
      if (device != nullptr) {
        handled = true;
        Devices::handleCommand(device, commandName.c_str(), true);
      } else {
        Logs::serialPrintln(me, PSTR("processMessage:handleDeviceCommand:differentDeviceIndex"));
      }
    }
  } else if (strcmp_P(actionChar, PSTR("restartDevice")) == 0) {
    String deviceId = doc[F("deviceId")].as<String>();
    if (chipId.equalsIgnoreCase(deviceId)) {
      handled = true;
      propagateMessage = false;
      Logs::serialPrintln(me, PSTR("restartDevice:started"));
      delay(3000);
      Devices::restart();
    }
  } else if (strcmp_P(actionChar, PSTR("removeDevice")) == 0) {
    String deviceId = doc[F("deviceId")].as<String>();
    Mesh::removeDeviceFromNodeList(deviceId.c_str());
    if (chipId.equalsIgnoreCase(deviceId)) {
      handled = true;
      propagateMessage = false;
      Logs::serialPrintln(me, PSTR("removeDevice:started"));
      Storage::storageStruct emptyData;
      Storage::writeFlash(emptyData, true);
      delay(3000);
      Devices::restart();
    }
  } else if (strcmp_P(actionChar, PSTR("onExitMasterWifiNode")) == 0) {
    Logs::serialPrintln(me, PSTR("processMessage:onExitMasterWifiNode"));
    Mesh::resetMasterWifiNode();
    Network::forceNetworkScan(random(500, 3000));
  } else if (action != nullptr) {
    Logs::serialPrintln(
        me, PSTR("[WARNING] Action no recognized: "), action.c_str(), PSTR(" Msg:["));
    Logs::serialPrint(me, message, PSTR("]"));
    propagateMessage = false;
    // Devices::restart();  // TODO: Workaround in the meantime until finding root cause
  }

  doc.clear();
  if (propagateMessage) {
    Logs::pauseLogging(true);
    // if (Network::isIpInSubnet(sender, accessPointIP)) {
    if (Network::isOneOfMyClients(sender)) {
      // When it comes from one of my connected clients, then propagate to the network I am
      // connected to
      Network::broadcastToWifi(message);
    } else {
      // When it comes from the network I'm connected to, propagate to my clients
       Network::broadcastToMyAPNodes(message);
    }
    Logs::pauseLogging(false);
  }
  // doc.clear();
  Logs::serialPrintlnEnd(me);
  return handled;
}

void handle() {
  if (pollDevices) {
    Logs::serialPrintln(me, PSTR("processMessage:pollDevices"));
    Mesh::Node nodeInfo = Mesh::getNodeInfo();
    String deviceInfo((char *)0);
    MessageGenerator::generateDeviceInfo(deviceInfo, nodeInfo, F("deviceInfo"));
    Network::broadcastEverywhere(deviceInfo.c_str(), true, false);
    pollDevices = false;
  }
}

}  // namespace MessageProcessor

// bool processRemoteLog(const JsonObject &content) {
//   // Process nodes list
//   Logs::LogData data;
//   data.timestamp = content[F("timestamp")];
//   data.deviceName = content[F("deviceName")].as<String>();
//   data.message = content[F("message")].as<String>();
//   bool append = content[F("append")].as<bool>();
//   return enqueue(data, append);
// }

// } else if (action == F("broadcastLogs")) {
//   Logs::pauseLogging(true);
//   Logs::receivedBroadcastLogsRequest();
//   Logs::pauseLogging(false);
// } else if (action == F("remoteLog")) {
//   Logs::pauseLogging(true);
//   JsonObject content = doc[F("content")];
//   handled = processRemoteLog(content);
//   if (handled) {
//     propagateMessage = false;
//   }
//   Logs::pauseLogging(false);
