#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include "Config.h"
#include "Events.h"
#include "Files.h"
#include "Logs.h"
#include "Mesh.h"
#include "MessageGenerator.h"
#include "MessageProcessor.h"
#include "Network.h"
#include "Storage.h"
#include "UniversalPlugAndPlay.h"
#include "Utils.h"

namespace WebServer {
const Logs::caller me = Logs::caller::WebServer;

ESP8266WebServer server(SERVER_PORT);

bool serverStarted = false;

ESP8266WebServer &getServer() {
  return server;
}

/*****************************************************
                   HANDLERS
*****************************************************/
bool processMeshChanges(Storage::storageStruct &flashData) {
  String meshName = server.arg(F("meshName"));
  String meshPassword = server.arg(F("meshPassword"));
  if (strcmp(Mesh::getMeshName(flashData), meshName.c_str()) == 0 &&
      meshPassword == CONFIDENTIAL_STRING) {
    return false;  // No Changes
  }
  if (!meshName.isEmpty() > 0 && !meshName.endsWith(F("_"))) {
    meshName = meshName + F("_");
  }
  Logs::serialPrintln(
      me, F("processMeshChanges: meshName=\""), String(flashData.meshName) + "\" -> " + meshName);
  strncpy(flashData.meshName, meshName.c_str(), MAX_LENGTH_WIFI_NAME);
  strncpy(flashData.meshPassword, meshPassword.c_str(), MAX_LENGTH_WIFI_PASSWORD);
  return true;
}

bool processWifiChanges(Storage::storageStruct &flashData) {
  String wifiName = server.arg(F("wifiName"));
  String wifiPassword = server.arg(F("wifiPassword"));
  Logs::serialPrintln(
      me, F("processWifiChanges: wifiName=\""), flashData.wifiName, +"\" -> \"" + wifiName + "\"");
  if (strcmp(flashData.wifiName, wifiName.c_str()) == 0 && wifiPassword == CONFIDENTIAL_STRING) {
    return false;  // No Changes
  }
  strncpy(flashData.wifiName, wifiName.c_str(), MAX_LENGTH_WIFI_NAME);
  strncpy(flashData.wifiPassword, wifiPassword.c_str(), MAX_LENGTH_WIFI_PASSWORD);
  return true;
}

bool processHubChanges(Storage::storageStruct &flashData) {
  String hubNamespace = server.arg(F("hubNamespace"));
  Logs::serialPrintln(me, F("processWifiChanges: wifiName=\""), flashData.hubNamespace,
      +"\" -> \"" + hubNamespace + "\"");
  if (strcmp(flashData.hubNamespace, hubNamespace.c_str()) == 0) {
    return false;  // No Changes
  }
  strncpy(flashData.hubNamespace, hubNamespace.c_str(), MAX_LENGTH_HUB_NAMESPACE);
  return true;
}

// void handleSyncDevicesAlexaSkillPost() {
//   Logs::serialPrintlnStart(me, F("handleSaveAmazonProfilePost"));
//   Storage::storageStruct flashData = Storage::readFlash();

//   const String url = F("https://l32ezbt5b8.execute-api.us-east-1.amazonaws.com/default/");
//   const String auth = F("4rHOU0GUJv3rzDUEcMAUv5dq0fSweJsg3MGlpEfI");
//   const String contentType = F("application/json");
//   const String payload =
//       String(F("{ \"directive\": {  \"header\": {  \"namespace\": \"Alexa.Discovery\", \"name\": "
//                "\"Discover\",  \"payloadVersion\": \"3\",  \"messageId\": "
//                "\"1bd5d003-31b9-476f-ad03-71d471922820\" }, \"payload\": { \"scope\": { \"type\": "
//                "\"BearerToken\", \"token\": \"access-token-from-skill\" } } } }"));
//   Network::httpResponse response = Network::httpPost(url, payload, "", contentType, auth);
//   Logs::serialPrintln(me, response.returnPayload);
//   if (response.httpCode != 200) {
//     server.send(response.httpCode, F("application/json"), response.returnPayload);
//     Logs::serialPrintlnEnd(me);
//     return;
//   }
//   // StaticJsonDocument<512> doc;
//   // serializeJson(doc, response.returnPayload);
//   // String refresh_token = doc[F("refresh_token")];
//   // Logs::serialPrintln(me, access_token);
//   server.send(200, F("text/plain"), F("Success"));
//   Logs::serialPrintlnEnd(me);
// }

void handleSaveAmazonProfilePost() {
  Logs::serialPrintlnStart(me, F("handleSaveAmazonProfilePost"));

  String refresh_token = server.arg(F("refresh_token"));
  String user_id = server.arg(F("user_id"));
  String email = server.arg(F("email"));

  if (refresh_token.isEmpty() || user_id.isEmpty()) {
    server.send(500, F("text/plain"), F("No user id passed"));
    Logs::serialPrintlnEnd(me);
    return;
  }

  Storage::storageStruct flashData = Storage::readFlash();
  strncpy(flashData.amazonUserId, user_id.c_str(), MAX_LENGTH_AMAZON_USER_ID);
  strncpy(flashData.amazonEmail, email.c_str(), MAX_LENGTH_AMAZON_EMAIL);
  Storage::writeFlash(flashData);

  server.send(200, F("text/plain"), F("Success"));
  Logs::serialPrintlnEnd(me);
}

void handleSetupPost() {
  Logs::serialPrintlnStart(me, F("handleSetupPost"));
  // Before persisting data, validate it first
  Storage::storageStruct flashData = Storage::readFlash();
  bool meshChanges = processMeshChanges(flashData);
  bool wifiChanges = processWifiChanges(flashData);
  bool hubChanges = processHubChanges(flashData);
  if (meshChanges || wifiChanges || hubChanges) {
    Storage::writeFlash(flashData);
    String json = MessageGenerator::generateSharedInfo();
    Network::broadcastMessage(json);
  }
  server.send(200, F("text/plain"), F("Success"));
  if (meshChanges || hubChanges) {
    Devices::restart();
  } else {
    Network::forceNetworkScan();
  }
  Logs::serialPrintlnEnd(me);
}

void handlePingGet() {
  String json = MessageGenerator::generateRawAction(F("pingDevices"));
  Network::broadcastMessage(json, true, false);
}

void serverSendContent(const String &content) {
  server.sendContent(content);
}

void handlePollGet() {
  Logs::serialPrintlnStart(me, F("handlePollGet"));
  String message = MessageGenerator::generateRawAction(F("pollDevices"));
  Network::broadcastMessage(message, true, false);
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, F("application/json"), F(""));
  MessageGenerator::generateChunkedMeshReport(serverSendContent);
  server.sendContent(F(""));
  Network::forceNetworkScan(3000);
  Logs::serialPrintlnEnd(me);
}

void handleLogsGet() {
  Logs::pauseLogging(true);
  long newerThanTimestamp = server.arg(F("newerThanTimestamp")).toInt();
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, F("application/json"), F(""));
  MessageGenerator::generateChunkedLogsHistory(newerThanTimestamp, serverSendContent);
  // String msg = MessageGenerator::generateRawAction("broadcastLogs");
  // Network::broadcastMessage(msg);
  server.sendContent(F(""));
  Logs::pauseLogging(false);
}

void handleAddDevicePost() {
  Logs::serialPrintlnStart(me, F("handleAddDevicePost"));
  String SSID = server.arg(F("SSID"));
  server.send(200, F("text/plain"), F("Adding device"));
  String message = MessageGenerator::generateSharedInfo();

  Logs::serialPrintln(me, F("Disconnecting from WiFi"));
  WiFi.disconnect(false);
  Logs::serialPrintln(me, F("Connecting to AP: "), SSID);
  const bool connected = Network::connectToAP(SSID, F(""), 0, NULL);
  if (connected) {
    // send node info
    Logs::serialPrintln(me, F("Broadcasting mesh info"));
    Network::broadcastMessage(message);
    delay(3000);
  } else {
    Logs::serialPrintln(me, F("Failed connecting to access point"));
  }
  Logs::serialPrintln(me, F("Disconnecting from AP"));
  WiFi.disconnect(false);
  Logs::serialPrintlnEnd(me);
}

void handleRestartPost() {
  Logs::serialPrintlnStart(me, F("handleRestartPost"));
  String deviceId = server.arg(F("deviceId"));
  String msg = MessageGenerator::generateRawAction(F("restartDevice"), deviceId);
  Network::broadcastMessage(msg, true, true);
  Logs::serialPrintlnEnd(me);
  server.send(200, F("text/plain"), F("Restart started"));
}

void handleToggleDeviceStatePost() {
  Logs::serialPrintlnStart(me, F("handleToggleDeviceStatePost"));
  String deviceId = server.arg(F("deviceId"));
  String deviceIndex = server.arg(F("deviceIndex"));
  String msg = MessageGenerator::generateRawAction(F("toggleDeviceState"), deviceId, deviceIndex);
  Logs::serialPrintln(me, msg);
  Network::broadcastMessage(msg, true, true);
  server.send(200, F("text/plain"), F("Toggle device state request issued"));
  Logs::serialPrintlnEnd(me);
}

void handleSetDeviceStatePost() {
  Logs::serialPrintlnStart(me, F("handleSetDeviceStatePost"));
  String deviceId = server.arg(F("deviceId"));
  String deviceIndex = server.arg(F("deviceIndex"));
  String state = server.arg(F("state"));
  String msg =
      MessageGenerator::generateRawAction(F("setDeviceState"), deviceId, deviceIndex, state);
  Logs::serialPrintln(me, msg);
  Network::broadcastMessage(msg, true, true);
  server.send(200, F("text/plain"), F("Set device state request issued"));
  Logs::serialPrintlnEnd(me);
}

void handleUpdateDevicePost() {
  Logs::serialPrintlnStart(me, F("handleUpdateDevicePost"));
  Mesh::Node deviceInfo;
  deviceInfo.deviceId = server.arg(F("deviceId"));
  deviceInfo.deviceName = server.arg(F("deviceName"));
  String deviceInfoJson = MessageGenerator::generateDeviceInfo(deviceInfo, F("updateDevice"));
  Network::broadcastMessage(deviceInfoJson, true, false);
  server.send(200, F("text/plain"), F("Device Update request issued"));
  Logs::serialPrintlnEnd(me);
}

void handleRemoveDevicePost() {
  Logs::serialPrintlnStart(me, F("handleRemoveDevicePost"));
  String deviceId = server.arg(F("deviceId"));
  server.send(200, F("text/plain"), F("Device removal started"));
  String msg = MessageGenerator::generateRawAction(F("removeDevice"), deviceId);
  Network::broadcastMessage(msg, true, true);
  Logs::serialPrintlnEnd(me);
}

void handleConfigGet() {
  Logs::serialPrintlnStart(me, F("handleConfigGet"));
  // Before persisting data, validate it first
  String sharedInfo = MessageGenerator::generateSharedInfo(true);
  server.send(200, F("application/json"), sharedInfo);
  Logs::serialPrintlnEnd(me);
}

const String DEFAULT_PAGE = "/index.htm";

String getContentType(const String &path) {
  if (path.endsWith(FPSTR(".htm"))) {
    return FPSTR("text/html");
  } else if (path.endsWith(".js")) {
    return FPSTR("application/javascript; charset=utf-8");
  } else if (path.endsWith(".css")) {
    return FPSTR("text/css; charset=utf-8");
  } else {
    return FPSTR("*/*");
  }
}

void handleNotFound() {
  // if (redirectToIP()) { return; }
  String path = server.uri();
  if (path.isEmpty() || path == "/") {
    path = DEFAULT_PAGE;
  }

  // String acceptEncoding = server.header(FPSTR("Accept-Encoding"));
  // if (acceptEncoding.indexOf("gzip") >= 0 &&
  String contentType = getContentType(path);
  if (Files::exists(path + ".gz")) {
    Logs::serialPrintln(me, F("Path found:"), path, F(" (Compressed)"));
    Files::streamFile(path + FPSTR(".gz"), contentType, server);
  } else if (Files::exists(path)) {
    Logs::serialPrintln(me, F("Path found:"), path);
    Files::streamFile(path, contentType, server);
  } else if (!Events::onWebServerNotFound(server)) {
    Logs::serialPrintln(me, F("Path not found:"), path);
    server.send(404, F("text/plain"), F("404: Not found"));
  }
}

String getPageHeader() {
  return F("<!doctype html>\
<html lang='en'>\
<head>\
    <meta charset='utf-8'>\
    <meta name='viewport' content='width=device-width, initial-scale=1, shrink-to-fit=no'>\
</head>\
<body>");
}

void handleFilesPage() {
  Logs::serialPrintln(me, F("handleFilesPage"));
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, F("text/html"), F(""));
  server.sendContent(getPageHeader());
  server.sendContent(F("<h1>File System</h1>"));
  Files::getFiles(server);
  server.sendContent(F("<hr>\
    <form method='post' action='uploadFile' enctype='multipart/form-data'>\
        <input type='file' name='name'>\
        <input class='button' type='submit' value='Upload'>\
    </form>\
    <hr>\
    <form method='post' action='deleteFile'>\
        <label for='fname'>Delete Filename:</label>\
        <input type='text' id='fname' name='fname'>\
        <input class='button' type='submit' value='Delete'>\
    </form>\
    "));
  server.sendContent(F("</body></html>"));
  server.sendContent(F(""));
}

void stop() {
  server.stop();
  serverStarted = false;
}

void setup() {
  if (serverStarted) {
    return;
  }
  Logs::serialPrintln(me, F("setup"));
  server.stop();

  server.on("/saveAmazonProfile", HTTP_POST, handleSaveAmazonProfilePost);
  //erver.on("/syncDevicesAlexaSkill", HTTP_POST, handleSyncDevicesAlexaSkillPost);

  server.on("/update", HTTP_POST, handleSetupPost);
  server.on("/updateDevice", HTTP_POST, handleUpdateDevicePost);
  server.on("/addDevice", HTTP_POST, handleAddDevicePost);
  server.on("/removeDevice", HTTP_POST, handleRemoveDevicePost);
  server.on("/toggleDeviceState", HTTP_POST, handleToggleDeviceStatePost);
  server.on("/setDeviceState", HTTP_POST, handleSetDeviceStatePost);
  server.on("/restart", HTTP_POST, handleRestartPost);
  server.on("/config", HTTP_GET, handleConfigGet);
  server.on("/logs", HTTP_GET, handleLogsGet);
  server.on("/ping", HTTP_GET, handlePingGet);
  server.on("/poll", HTTP_GET, handlePollGet);
  server.on(
      "/uploadFile", HTTP_POST, []() { server.send(200); }, []() { Files::fileUpload(server); });
  server.on("/deleteFile", HTTP_POST, []() { Files::fileDelete(server.arg(F("fname")), server); });
  server.on("/files", HTTP_GET, handleFilesPage);

  Events::onStartingWebServer(server);
  server.onNotFound(handleNotFound);
  server.begin();
  serverStarted = true;

  Logs::serialPrintln(me, F("HTTP server started"));
}

void handle() {
  server.handleClient();
}

}  // namespace WebServer