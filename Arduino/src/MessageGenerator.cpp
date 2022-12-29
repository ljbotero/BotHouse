#include "MessageGenerator.h"
#include <limits.h>
#include "Config.h"
#include "Devices.h"
#include "Events.h"
#include "Logs.h"
#include "Mesh.h"
#include "MessageProcessor.h"
#include "Network.h"
#include "Storage.h"
#include "Utils.h"

namespace MessageGenerator {

static const Logs::caller me = Logs::caller::MessageGenerator;

void ICACHE_FLASH_ATTR generateChunkedMeshReport(void (&sendContent)(const String &content)) {
  bool first = true;
  Mesh::Node *currNode = Mesh::getNodesTip();
  sendContent(F("{\"action\":\"meshInfo\",\"content\":["));
  while (currNode != nullptr) {
    String content((char *)0);
    content.reserve(300);
    if (first) {
      first = false;
    } else {
      content += FPSTR(",");
    }
    content.concat(FPSTR("{\"deviceId\":\""));
    content.concat(currNode->deviceId);
    content.concat(FPSTR("\",\"deviceName\":\""));
    content.concat(currNode->deviceName);
    content.concat(FPSTR("\",\"macAddress\":\""));
    content.concat(currNode->macAddress);
    content.concat(FPSTR("\",\"wifiSSID\":\""));
    content.concat(currNode->wifiSSID);
    content.concat(FPSTR("\",\"wifiRSSI\":"));
    content.concat(currNode->wifiRSSI);
    content.concat(FPSTR(",\"isMaster\":"));
    content.concat(currNode->isMaster);
    content.concat(FPSTR(",\"IPAddress\":\""));
    content.concat(currNode->IPAddress);
    content.concat(FPSTR("\",\"apSSID\":\""));
    content.concat(currNode->apSSID);
    content.concat(FPSTR("\",\"apLevel\":"));
    content.concat(currNode->apLevel);
    content.concat(FPSTR(",\"freeHeap\":"));
    content.concat(currNode->freeHeap);
    content.concat(FPSTR(",\"lastUpdate\":"));
    content.concat(currNode->lastUpdate);
    content.concat(FPSTR(",\"systemTime\":"));
    content.concat(currNode->systemTime);
    content.concat(FPSTR(",\"devices\":["));
    sendContent(content);
    // Logs::serialPrintln(
    //     me, PSTR("generateChunkedMeshReport:length:"), String(content.length()).c_str());

    StaticJsonDocument<512> doc;
    // auto doc = Utils::getJsonDoc();
    bool firstSub = true;
    Devices::DeviceDescription *currDevice = currNode->devices;
    while (currDevice != nullptr) {
      doc.clear();
      doc[F("deviceIndex")] = currDevice->index;
      doc[F("deviceTypeId")] = String(currDevice->typeId);
      doc[F("deviceState")] = String(currDevice->lastEventName);
      doc[F("deviceValue")] = currDevice->lastEventValue;
      String subContent((char *)0);
      subContent.reserve(300);
      serializeJson(doc, subContent);
      if (firstSub) {
        firstSub = false;
      } else {
        subContent = String(FPSTR(",")) + subContent;
      }
      sendContent(subContent);
      // Logs::serialPrintln(
      //     me, PSTR("generateChunkedMeshReport:length:"), String(content.length()).c_str());
      currDevice = currDevice->next;
    }
    sendContent(F("],\"accessPoints\":["));

    firstSub = true;
    AccessPoints::AccessPointList *currApNode = currNode->accessPoints;
    while (currApNode != nullptr) {
      if (currApNode->ap != nullptr) {
        doc.clear();
        doc[F("SSID")] = String(currApNode->ap->SSID);
        doc[F("isRecognized")] = currApNode->ap->isRecognized;
        doc[F("isOpen")] = currApNode->ap->isOpen;
        doc[F("RSSI")] = currApNode->ap->RSSI;
        String subContent((char *)0);
        subContent.reserve(300);
        serializeJson(doc, subContent);
        if (firstSub) {
          firstSub = false;
        } else {
          subContent = String(FPSTR(",")) + subContent;
        }
        sendContent(subContent);
        // Logs::serialPrintln(
        //     me, PSTR("generateChunkedMeshReport:length:"), String(content.length()).c_str());
      }
      currApNode = currApNode->next;
    }
    sendContent(F("]}"));
    currNode = currNode->next;
    yield();
  }
  sendContent(F("]}"));
}

void ICACHE_FLASH_ATTR generateDeviceInfo(
    String &outputJson, const Mesh::Node &deviceInfo, const String &action) {
  DynamicJsonDocument doc(1024 * 4);
  // auto doc = Utils::getJsonDoc();
  doc.clear();
  doc[F("action")] = action;
  JsonObject content = doc.createNestedObject(F("content"));
  content[F("deviceId")] = deviceInfo.deviceId;
  content[F("deviceName")] = deviceInfo.deviceName;
  content[F("macAddress")] = deviceInfo.macAddress;
  content[F("wifiSSID")] = deviceInfo.wifiSSID;
  content[F("wifiRSSI")] = deviceInfo.wifiRSSI;
  content[F("isMaster")] = deviceInfo.isMaster;
  content[F("IPAddress")] = deviceInfo.IPAddress;
  content[F("apSSID")] = deviceInfo.apSSID;
  content[F("apLevel")] = deviceInfo.apLevel;
  content[F("freeHeap")] = deviceInfo.freeHeap;
  content[F("systemTime")] = deviceInfo.systemTime;
  if (!Events::isSafeMode()) {
    Devices::serializeDevices(content, deviceInfo.devices);
    JsonArray accessPoints = content.createNestedArray(F("accessPoints"));
    AccessPoints::AccessPointList *currNode = deviceInfo.accessPoints;
    while (currNode != nullptr) {
      if (Mesh::isAccessPointAPotentialNode(currNode->ap->SSID)) {
        JsonObject accessPoint = accessPoints.createNestedObject();
        accessPoint[F("SSID")] = String(currNode->ap->SSID);
        accessPoint[F("isRecognized")] = currNode->ap->isRecognized;
        accessPoint[F("isOpen")] = currNode->ap->isOpen;
        accessPoint[F("RSSI")] = currNode->ap->RSSI;
      }
      currNode = currNode->next;
    }
  }
  // String json((char *)0);
  serializeJson(doc, outputJson);
  doc.clear();
  Logs::serialPrintln(me, PSTR("generateDeviceInfo:length:"), String(outputJson.length()).c_str());
  // Logs::serialPrintln(me, PSTR(" ("), String(requiredMem).c_str(), PSTR(" estimated)"));
  // return json;
}

void generateRawAction(String &outputJson, const String &action, const String &deviceId,
    const String &deviceIndex, const String &data) {
  StaticJsonDocument<512> doc;
  // auto doc = Utils::getJsonDoc();
  doc[F("action")] = action;
  if (!deviceId.isEmpty()) {
    doc[F("deviceId")] = deviceId;
  }
  if (!deviceIndex.isEmpty()) {
    doc[F("deviceIndex")] = deviceIndex;
  }
  if (!data.isEmpty()) {
    doc[F("data")] = data;
  }
  serializeJson(doc, outputJson);
  Logs::serialPrintln(me, PSTR("generateRawAction="), outputJson.c_str());
  doc.clear();
}

void ICACHE_FLASH_ATTR generateDeviceEvent(String &outputJson, const Devices::DeviceState &state) {
  if (Events::isSafeMode()) {
    return;
  }
  StaticJsonDocument<512> doc;
  // auto doc = Utils::getJsonDoc();
  doc.clear();

  doc[F("action")] = F("deviceEvent");
  JsonObject content = doc.createNestedObject(F("content"));
  content[F("deviceId")] = String(state.deviceId);
  content[F("deviceIndex")] = state.deviceIndex;
  content[F("deviceTypeId")] = String(state.deviceTypeId);
  content[F("eventName")] = String(state.eventName);
  content[F("eventValue")] = state.eventValue;
  serializeJson(doc, outputJson);
}

void ICACHE_FLASH_ATTR generateSharedInfo(String &outputJson, bool hideConfidentialData) {
  if (Events::isSafeMode()) {
    return;
  }
  StaticJsonDocument<512> doc;
  // auto doc = Utils::getJsonDoc();
  doc.clear();
  Logs::serialPrintlnStart(me, PSTR("requestSharedInfo"));

  doc[F("action")] = F("sharedInfo");
  JsonObject content = doc.createNestedObject(F("content"));
  JsonObject info = content.createNestedObject(F("info"));
  info[F("time")] = millis();
  info[F("connectedToWifiRouter")] = Mesh::isConnectedToWifi();

  Storage::storageStruct flashData = Storage::readFlash();
  JsonObject storage = content.createNestedObject(F("storage"));
  storage[F("version")] = flashData.version;
  storage[F("state")] = flashData.state;
  storage[F("wifiName")] = String(flashData.wifiName);
  storage[F("wifiPassword")] = hideConfidentialData ? CONFIDENTIAL_STRING : String(flashData.wifiPassword);
  storage[F("hubApi")] = String(flashData.hubApi);
  storage[F("hubToken")] = hideConfidentialData ? CONFIDENTIAL_STRING : String(flashData.hubToken);
  storage[F("hubNamespace")] = String(flashData.hubNamespace);
  serializeJson(doc, outputJson);
  Logs::serialPrintlnEnd(
      me, PSTR("generateSharedInfo:length:"), String(outputJson.length()).c_str());
  //  Logs::serialPrintlnEnd(me, String(outputJson.length()) + FPSTR(" Bytes"));
}

}  // namespace MessageGenerator

// void generateChunkedLogsHistory(
//     long newerThanTimestamp, void (&sendContent)(const String &content)) {
//   Logs::LogHistory *logHistory = Logs::getLogHistoryQueue();
//   if (logHistory != nullptr) {
//     if (logHistory->data.timestamp < newerThanTimestamp) {
//       newerThanTimestamp = 0;
//     }
//   }
//   bool first = true;
//   sendContent(F("["));
//   while (logHistory != nullptr) {
//     if (logHistory->data.timestamp > newerThanTimestamp && logHistory->data.deviceName != nullptr
//     &&
//         logHistory->data.message != nullptr) {
//       if (first) {
//         first = false;
//       } else {
//         sendContent(F(","));
//       }

//       StaticJsonDocument<512> doc;
//       // DynamicJsonDocument doc(512);
//       doc[F("timestamp")] = logHistory->data.timestamp;
//       doc[F("deviceName")] = String(logHistory->data.deviceName);
//       doc[F("message")] = String(logHistory->data.message);
//       String message = "";
//       serializeJson(doc, message);
//       sendContent(message);
//     }
//     logHistory = logHistory->next;
//   }
//   sendContent(F("]"));
// }

// String generateLogMessage(const String &message, bool append) {
//   StaticJsonDocument<512> doc;
//   doc.clear();
//   // auto doc = Utils::getJsonDoc();
//   doc[F("action")] = F("remoteLog");
//   JsonObject content = doc.createNestedObject(F("content"));
//   content[F("timestamp")] = Utils::getNormailzedTime();
//   content[F("deviceName")] = chipId.c_str();
//   content[F("message")] = message.c_str();
//   content[F("append")] = append;
//   String json((char *)0);
//   serializeJson(doc, json);
//   doc.clear();
//   return json;
// }
