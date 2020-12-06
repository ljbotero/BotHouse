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

const Logs::caller me = Logs::caller::MessageProcessor;

void processDeviceInfoReport(const JsonObject &content) {
  Mesh::Node newNode;
  newNode.deviceId = content[F("deviceId")].as<String>();
  newNode.deviceName = content[F("deviceName")].as<String>();
  newNode.wifiSSID = content[F("wifiSSID")].as<String>();
  newNode.wifiRSSI = content[F("wifiRSSI")];
  newNode.isMaster = content[F("isMaster")];
  newNode.IPAddress = content[F("IPAddress")].as<String>();
  newNode.apSSID = content[F("apSSID")].as<String>();
  newNode.apLevel = content[F("apLevel")];
  newNode.freeHeap = content[F("freeHeap")];
  JsonArray devices = content[F("devices")].as<JsonArray>();
  newNode.devices = Devices::deserializeDevices(devices);
  JsonArray accessPoints = content[F("accessPoints")].as<JsonArray>();
  AccessPoints::AccessPointList *currAp = NULL;
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
    currAp->ap->SSID = accessPoint[F("SSID")].as<String>();
    currAp->ap->isRecognized = accessPoint[F("isRecognized")].as<bool>();
    currAp->ap->isOpen = accessPoint[F("isOpen")].as<bool>();
    currAp->ap->RSSI = accessPoint[F("RSSI")];
  }
  Mesh::updateOrAddNodeInfoList(newNode);
}

bool processUpdateDevice(const JsonObject &content) {
  // Process nodes list
  Logs::serialPrintlnStart(me, F("processUpdateDevice"));
  Mesh::Node nodeInfo;
  nodeInfo.deviceId = content[F("deviceId")].as<String>();
  nodeInfo.deviceName = content[F("deviceName")].as<String>();
  JsonArray devices = content[F("devices")].as<JsonArray>();
  if (Utils::getChipIdString() == nodeInfo.deviceId) {
    Devices::updateDevice(nodeInfo.deviceName);
  }
  Logs::serialPrintlnEnd(me);
  return Mesh::updateExistingNodeInfo(nodeInfo);
}

bool processRemoteLog(const JsonObject &content) {
  // Process nodes list
  Logs::LogData data;
  data.timestamp = content[F("timestamp")];
  data.deviceName = content[F("deviceName")].as<String>();
  data.message = content[F("message")].as<String>();
  bool append = content[F("append")].as<bool>();
  return enqueue(data, append);
}

bool processDeviceEvent(const JsonObject &content) {
  Logs::serialPrintlnStart(me, F("processDeviceEvent"));
  // Process nodes list
  Devices::DeviceState state;
  state.deviceId = content[F("deviceId")].as<String>();
  Logs::serialPrintln(me, F("deviceId: "), state.deviceId);
  state.deviceIndex = content[F("deviceIndex")];
  Logs::serialPrint(me, F(", deviceIndex: "), String(state.deviceIndex));
  state.deviceTypeId = content[F("deviceTypeId")].as<String>();
  Logs::serialPrint(me, F(", typeId: "), state.deviceTypeId);
  state.eventName = content[F("eventName")].as<String>();
  Logs::serialPrint(me, F(", eventName: "), state.eventName);
  state.eventValue = content[F("eventValue")].as<String>();
  Logs::serialPrint(me, F(", eventValue: "), state.eventValue);
  Logs::serialPrintlnEnd(me);
  return Events::onDeviceEvent(state);
}

bool processSharedInfo(const JsonObject &doc) {
  Logs::serialPrintlnStart(me, F("processMeshInfo"));
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
    Logs::serialPrintln(me, F("remoteStorage[version]="), String(version));
    Logs::serialPrintln(me, F("myStorage[version]    ="), String(flashData.version));
    Logs::serialPrintln(me, F("Updating flash data"));
    flashData.version = version;
    flashData.state = storage[F("state")];
    bool changed1 = Utils::copyStringFromJson(flashData.meshName, storage, F("meshName"));
    bool changed2 = Utils::copyStringFromJson(flashData.meshPassword, storage, F("meshPassword"));
    bool changed3 = Utils::copyStringFromJson(flashData.wifiName, storage, F("wifiName"));
    bool changed4 = Utils::copyStringFromJson(flashData.wifiPassword, storage, F("wifiPassword"));
    bool changed5 = Utils::copyStringFromJson(flashData.hubApi, storage, F("hubApi"));
    bool changed6 = Utils::copyStringFromJson(flashData.hubToken, storage, F("hubToken"));
    bool changed7 = Utils::copyStringFromJson(flashData.hubNamespace, storage, F("hubNamespace"));
    bool changed8 = Utils::copyStringFromJson(flashData.amazonUserId, storage, F("amazonUserId"));
    bool changed9 = Utils::copyStringFromJson(flashData.amazonEmail, storage, F("amazonEmail"));
    Storage::writeFlash(flashData, false);
    if (changed1 || changed2 || changed3 || changed4 || changed5 || changed6 || changed7 ||
        changed8 || changed9) {
      Devices::restart();
    }
  }
  Logs::serialPrintlnEnd(me);
  return true;
}

bool processRequestSharedInfo(const IPAddress &sender) {
  if (!Mesh::isAccessPointNode() || strlen(Storage::readFlash().meshPassword) == 0) {
    return false;
  }
  // Only return sharedInfo when Mesh is setup with a password
  const String message = MessageGenerator::generateSharedInfo();
  if (!sender.isSet()) {
    Logs::serialPrintln(me, F("requestSharedInfo:broadcasting"));
    Network::broadcastMessage(message);
  } else {
    Logs::serialPrintln(me, F("requestSharedInfo:replyingTo:"), sender.toString());
    Network::sendUdpMessage(sender, BROADCAST_PORT, message.c_str());
  }
  return true;
}

bool processMessage(const char *message, const IPAddress &sender, const uint16_t senderPort) {
  // Process nodes list
  if (message == nullptr || strlen(message) <= 4) {
    return false;
  }
  DynamicJsonDocument doc(1024 * 6);
  deserializeJson(doc, message);
  doc.shrinkToFit();

  if (doc.isNull()) {
    Logs::serialPrint(me, F("ERROR: Could not deserialize Json for message: "));
    Logs::serialPrintln(me, message);
    return false;
  }
  String action = doc[F("action")];
  JsonObject content = doc[F("content")];

  // doc.garbageCollect();
  bool handled = false;
  bool propagateMessage = Mesh::isAccessPointNode() || Network::isAnyClientConnectedToThisAP();

  if (action == F("requestSharedInfo")) {
    handled = processRequestSharedInfo(sender);
  } else if (action == F("sharedInfo")) {
    Logs::serialPrintln(me, F("processMessage:sharedInfo"));
    processSharedInfo(content);
  } else if (action == F("deviceEvent")) {
    handled = processDeviceEvent(content);
    if (handled) {
      Logs::serialPrintln(me, F("processMessage:deviceEvent:handled"));
    } else {
      Logs::serialPrintln(me, F("processMessage:deviceEvent:unhandled"));
    }
    // } else if (action == F("broadcastLogs")) {
    //   Logs::pauseLogging(true);
    //   Logs::receivedBroadcastLogsRequest();
    //   Logs::pauseLogging(false);
  } else if (action == F("remoteLog")) {
    Logs::pauseLogging(true);
    handled = processRemoteLog(content);
    if (handled) {
      propagateMessage = false;
    }
    Logs::pauseLogging(false);
  } else if (action == F("deviceInfo")) {
    Logs::serialPrintln(me, F("processMessage:deviceInfo"));
    processDeviceInfoReport(content);
  } else if (action == F("heartbeat")) {
    Logs::serialPrintln(me, F("processMessage:heartbeat"));
    String deviceId = doc[F("deviceId")];
    handled = HubsIntegration::sendHeartbeat(deviceId);
  } else if (action == F("updateDevice")) {
    handled = processUpdateDevice(content);
  } else if (action == F("pollDevices")) {
    Logs::serialPrintln(me, F("processMessage:pollDevices"));
    Mesh::Node nodeInfo = Mesh::getNodeInfo();
    String deviceInfo = MessageGenerator::generateDeviceInfo(nodeInfo, F("deviceInfo"));
    Network::broadcastMessage(deviceInfo, true, false);
  } else if (action == F("pingDevices")) {
    Logs::serialPrintln(me, F("Pong"));
  } else if (action == F("toggleDeviceState")) {
    Logs::serialPrintln(me, F("processMessage:toggleDeviceState"));
    String deviceId = doc[F("deviceId")];
    if (deviceId == Utils::getChipIdString()) {
      handled = true;
      propagateMessage = false;
      String deviceNumberStr = doc[F("deviceIndex")];
      int deviceIndex = deviceNumberStr.toInt();
      Devices::DeviceDescription *device =
          Devices::getDeviceFromIndex(Devices::getRootDevice(), deviceIndex);
      if (device != nullptr) {
        Devices::handleCommand(device, "Toggle");
      }
    }
  } else if (action == F("setDeviceState")) {
    Logs::serialPrintln(me, F("processMessage:setDeviceState"));
    String deviceId = doc[F("deviceId")];
    if (deviceId == Utils::getChipIdString()) {
      handled = true;
      propagateMessage = false;
      String deviceNumberStr = doc[F("deviceIndex")];
      int deviceIndex = deviceNumberStr.toInt();
      String stateStr = doc[F("data")];
      int state = stateStr.toInt();
      Devices::setDeviceSate(deviceIndex, state);
    }
  } else if (action == F("restartDevice")) {
    String deviceId = doc[F("deviceId")];
    if (deviceId == Utils::getChipIdString()) {
      handled = true;
      propagateMessage = false;
      Logs::serialPrintln(me, F("restartDevice:started"));
      delay(3000);
      Devices::restart();
    }
  } else if (action == F("removeDevice")) {
    String deviceId = doc[F("deviceId")];
    Mesh::removeDeviceFromNodeList(deviceId);
    if (deviceId == Utils::getChipIdString()) {
      handled = true;
      propagateMessage = false;
      Logs::serialPrintln(me, F("removeDevice:started"));
      Storage::storageStruct emptyData;
      Storage::writeFlash(emptyData, true);
      delay(3000);
      Devices::restart();
    }
  } else if (action == F("onExitMasterWifiNode")) {
    Logs::serialPrintln(me, F("processMessage:onExitMasterWifiNode"));
    Mesh::resetMasterWifiNode();
    Network::forceNetworkScan(random(500, 3000));
  } else {
    Logs::serialPrintln(
        me, F("WARNING: Action no recognized: "), action + FPSTR(" Msg:[") + message, F("]"));
    propagateMessage = false;
    Devices::restart();  // TODO: Workaround in the meantime until finding root cause
  }

  if (propagateMessage) {
    Logs::pauseLogging(true);
    // if (Network::isIpInSubnet(sender, accessPointIP)) {
    if (Network::isOneOfMyClients(sender)) {
      // When it comes from one of my connected clients, then propagate to the network I am
      // connected to
      Logs::serialPrintln(me, F("propagating:broadcastMessage"));
      Network::broadcastMessage(message, false, false);
    } else {
      // When it comes from the network I'm connected to, propagate to my clients
      Logs::serialPrintln(me, F("propagating:forwardMessage"));
      Network::forwardMessage(message);
    }
    Logs::pauseLogging(false);
  }
  return handled;
}

}  // namespace MessageProcessor