#include "MessageGenerator.h"
#include <limits.h>
#include "Config.h"
#include "Devices.h"
#include "Logs.h"
#include "Mesh.h"
#include "MessageProcessor.h"
#include "Network.h"
#include "Storage.h"
#include "Utils.h"

namespace MessageGenerator {

const Logs::caller me = Logs::caller::MessageGenerator;

void generateChunkedMeshReport(void (&sendContent)(const String &content)) {
  bool first = true;
  Mesh::Node *currNode = Mesh::getNodesTip();
  sendContent(F("{\"action\":\"meshInfo\",\"content\":["));
  while (currNode != nullptr) {
    String content = "";
    if (first) {
      first = false;
    } else {
      content += ",";
    }
    content.concat(FPSTR("{\"deviceId\":\""));
    content.concat(currNode->deviceId);
    content.concat(FPSTR("\",\"deviceName\":\""));
    content.concat(currNode->deviceName);
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
    content.concat(FPSTR(",\"devices\":["));
    sendContent(content);

    StaticJsonDocument<512> doc;
    bool firstSub = true;
    Devices::DeviceDescription *currDevice = currNode->devices;
    while (currDevice != nullptr) {
      content = "";
      doc[F("deviceIndex")] = currDevice->index;
      doc[F("deviceTypeId")] = currDevice->typeId;
      doc[F("deviceState")] = currDevice->lastEventName;
      serializeJson(doc, content);
      if (firstSub) {
        firstSub = false;
      } else {
        content = "," + content;
      }
      sendContent(content);
      currDevice = currDevice->next;
    }
    sendContent(F("],\"accessPoints\":["));

    doc.clear();
    firstSub = true;
    AccessPoints::AccessPointList *currApNode = currNode->accessPoints;
    while (currApNode != nullptr) {
      if (currApNode->ap != nullptr) {
        content = "";
        doc[F("SSID")] = currApNode->ap->SSID;
        doc[F("isRecognized")] = currApNode->ap->isRecognized;
        doc[F("isOpen")] = currApNode->ap->isOpen;
        doc[F("RSSI")] = currApNode->ap->RSSI;
        serializeJson(doc, content);
        if (firstSub) {
          firstSub = false;
        } else {
          content = "," + content;
        }
        sendContent(content);
      }
      currApNode = currApNode->next;
    }
    sendContent(F("]}"));

    currNode = currNode->next;
  }
  sendContent(F("]}"));
}

void generateChunkedLogsHistory(
    long newerThanTimestamp, void (&sendContent)(const String &content)) {
  Logs::LogHistory *logHistory = Logs::getLogHistoryQueue();
  if (logHistory != nullptr) {
    if (logHistory->data.timestamp < newerThanTimestamp) {
      newerThanTimestamp = 0;
    }
  }
  bool first = true;
  sendContent(F("["));
  while (logHistory != nullptr) {
    if (logHistory->data.timestamp > newerThanTimestamp && logHistory->data.deviceName != nullptr &&
        logHistory->data.message != nullptr) {
      if (first) {
        first = false;
      } else {
        sendContent(F(","));
      }

      StaticJsonDocument<512> doc;
      doc[F("timestamp")] = logHistory->data.timestamp;
      doc[F("deviceName")] = String(logHistory->data.deviceName);
      doc[F("message")] = String(logHistory->data.message);
      String message = "";
      serializeJson(doc, message);
      sendContent(message);
    }
    logHistory = logHistory->next;
  }
  sendContent(F("]"));
}

String generateDeviceInfo(Mesh::Node deviceInfo, const String &action) {
  DynamicJsonDocument doc(1024 * 6);
  doc[F("action")] = action;
  JsonObject content = doc.createNestedObject(F("content"));
  content[F("deviceId")] = deviceInfo.deviceId;
  content[F("deviceName")] = deviceInfo.deviceName;
  content[F("wifiSSID")] = deviceInfo.wifiSSID;
  content[F("wifiRSSI")] = deviceInfo.wifiRSSI;
  content[F("isMaster")] = deviceInfo.isMaster;
  content[F("IPAddress")] = deviceInfo.IPAddress;
  content[F("apSSID")] = deviceInfo.apSSID;
  content[F("apLevel")] = deviceInfo.apLevel;
  content[F("freeHeap")] = deviceInfo.freeHeap;
  Devices::serializeDevices(content, deviceInfo.devices);
  JsonArray accessPoints = content.createNestedArray(F("accessPoints"));
  AccessPoints::AccessPointList *currNode = deviceInfo.accessPoints;
  while (currNode != nullptr) {
    if (Mesh::isAccessPointAPotentialNode(currNode->ap->SSID)) {
      JsonObject accessPoint = accessPoints.createNestedObject();
      accessPoint[F("SSID")] = currNode->ap->SSID;
      accessPoint[F("isRecognized")] = currNode->ap->isRecognized;
      accessPoint[F("isOpen")] = currNode->ap->isOpen;
      accessPoint[F("RSSI")] = currNode->ap->RSSI;
    }
    currNode = currNode->next;
  }
  doc.shrinkToFit();
  String json;
  serializeJson(doc, json);
  Logs::serialPrintln(me, F("generateDeviceInfo:length:"), String(json.length()));
  return json;
}

String generateLogMessage(const String &message, bool append) {
  StaticJsonDocument<512> doc;
  doc[F("action")] = F("remoteLog");
  JsonObject content = doc.createNestedObject(F("content"));
  content[F("timestamp")] = Utils::getNormailzedTime();
  content[F("deviceName")] = Utils::getChipIdString();
  content[F("message")] = message.c_str();
  content[F("append")] = append;
  String json;
  serializeJson(doc, json);
  return json;
}

String generateRawAction(String action, String deviceId, String deviceIndex, String data) {
  StaticJsonDocument<512> doc;
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
  String message;
  serializeJson(doc, message);
  Logs::serialPrintln(me, F("generateRawAction="), message);
  return message;
}

String generateAlexaDeviceEvent(Devices::DeviceState state) {
  Storage::storageStruct flashData = Storage::readFlash();
  DynamicJsonDocument doc(2048);

  JsonObject directive = doc.createNestedObject(F("directive"));
  JsonObject header = directive.createNestedObject(F("header"));
  header[F("namespace")] = F("Localbot");
  header[F("name")] = F("ChangeReport");
  JsonObject endpoint = directive.createNestedObject(F("endpoint"));
  endpoint[F("endpointId")] = state.deviceId + FPSTR(".Button.") + String(state.deviceIndex);
  endpoint[F("userId")] = flashData.amazonUserId;
  JsonObject payload = directive.createNestedObject(F("payload"));
  JsonObject change = payload.createNestedObject(F("change"));
  JsonObject cause = change.createNestedObject(F("cause"));
  cause[F("type")] = F("PHYSICAL_INTERACTION");
  JsonArray properties = change.createNestedArray(F("properties"));
  JsonObject property = properties.createNestedObject();
  property[F("instance")] = PSTR("Button.") + String(state.deviceIndex);
  property[F("name")] = F("toggleState");
  property[F("namespace")] = F("Alexa.ToggleController");
  property[F("uncertaintyInMilliseconds")] = 500;
  if (state.eventName == FPSTR("Opened")) {
    property[F("value")] = F("OFF");
  } else if (state.eventName == FPSTR("Closed")) {
    property[F("value")] = F("ON");
  } else if (state.eventName == FPSTR("Heartbeat")) {
    if (state.eventValue == FPSTR("Opened")) {
      property[F("value")] = F("OFF");
    } else if (state.eventValue == FPSTR("Closed")) {
      property[F("value")] = F("ON");
    } else {
      Logs::serialPrintln(me, F("ERROR: Unhandled Heartbeat Alexa event: "), state.eventValue);
      return "";
    }
  } else {
    Logs::serialPrintln(me, F("ERROR: Unhandled Alexa event: "), state.eventValue);
    return "";
  }
  doc.shrinkToFit();
  String message;
  serializeJson(doc, message);
  // Logs::serialPrintln(me, F("generateAlexaDeviceEvent:"), message);
  return message;
}

String generateDeviceEvent(Devices::DeviceState state) {
  StaticJsonDocument<512> doc;

  doc[F("action")] = F("deviceEvent");
  JsonObject content = doc.createNestedObject(F("content"));
  content[F("deviceId")] = state.deviceId;
  content[F("deviceIndex")] = state.deviceIndex;
  content[F("deviceTypeId")] = state.deviceTypeId;
  content[F("eventName")] = state.eventName;
  content[F("eventValue")] = state.eventValue;
  String message;
  serializeJson(doc, message);
  return message;
}

String generateSharedInfo(bool hideConfidentialData) {
  DynamicJsonDocument doc(2048);
  Logs::serialPrintlnStart(me, F("requestSharedInfo"));

  doc[F("action")] = F("sharedInfo");
  JsonObject content = doc.createNestedObject(F("content"));
  JsonObject info = content.createNestedObject(F("info"));
  info[F("time")] = millis();
  info[F("connectedToWifiRouter")] = Mesh::isConnectedToWifi();

  Storage::storageStruct flashData = Storage::readFlash();
  JsonObject storage = content.createNestedObject(F("storage"));
  storage[F("version")] = flashData.version;
  storage[F("state")] = flashData.state;
  storage[F("meshName")] = Mesh::getMeshName(flashData);
  storage[F("meshPassword")] = hideConfidentialData ? CONFIDENTIAL_STRING : flashData.meshPassword;
  storage[F("wifiName")] = flashData.wifiName;
  storage[F("wifiPassword")] = hideConfidentialData ? CONFIDENTIAL_STRING : flashData.wifiPassword;
  storage[F("hubApi")] = flashData.hubApi;
  storage[F("hubToken")] = hideConfidentialData ? CONFIDENTIAL_STRING : flashData.hubToken;
  storage[F("hubNamespace")] = flashData.hubNamespace;
  storage[F("amazonUserId")] = flashData.amazonUserId;
  storage[F("amazonEmail")] = flashData.amazonEmail;
  doc.shrinkToFit();
  String message;
  serializeJson(doc, message);
  Logs::serialPrintlnEnd(me, String(message.length()) + FPSTR(" Bytes"));
  return message;
}

}  // namespace MessageGenerator