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
static const Logs::caller me = Logs::caller::WebServer;

#ifndef DISABLE_WEBSERVER
const static char pageHeader[] PROGMEM = "<!doctype html>\
<html lang='en'>\
<head>\
    <meta charset='utf-8'>\
    <meta name='viewport' content='width=device-width, initial-scale=1, shrink-to-fit=no'>\
</head>\
<body>";

static ESP8266WebServer server(SERVER_PORT);

static bool serverStarted = false;
static bool serverHandlersSetup = false;

ESP8266WebServer &getServer() {
  return server;
}

/*****************************************************
                   HANDLERS
*****************************************************/
bool ICACHE_FLASH_ATTR processMeshChanges(Storage::storageStruct &flashData) {
  return true;
}

bool ICACHE_FLASH_ATTR processWifiChanges(Storage::storageStruct &flashData) {
  String wifiName = server.arg(F("wifiName"));
  String wifiPassword = server.arg(F("wifiPassword"));

  const char *oldWifiName =  String(flashData.wifiName).c_str();
  const char *oldWifiPassword =  String(flashData.wifiPassword).c_str();

  Logs::serialPrint(me, PSTR("processWifiChanges: wifiName=\""),
      oldWifiName, PSTR("\" -> \""));
  Logs::serialPrintln(me, wifiName.c_str(), PSTR("\""));
  if (strncmp(oldWifiName, wifiName.c_str(), MAX_LENGTH_WIFI_NAME) == 0 
  && strncmp(CONFIDENTIAL_STRING, oldWifiPassword, MAX_LENGTH_WIFI_NAME) == 0) {
    Logs::serialPrintln(me, PSTR("No Changes detected"));
    return false;  // No Changes
  }
  Utils::sstrncpy(flashData.wifiName, wifiName.c_str(), MAX_LENGTH_WIFI_NAME);
  Utils::sstrncpy(flashData.wifiPassword, wifiPassword.c_str(), MAX_LENGTH_WIFI_PASSWORD);
  return true;
}

bool ICACHE_FLASH_ATTR processHubChanges(Storage::storageStruct &flashData) {
  String hubNamespace = server.arg(F("hubNamespace"));
  Logs::serialPrintln(
      me, PSTR("processWifiChanges: wifiName=\""), String(flashData.hubNamespace).c_str());
  Logs::serialPrint(me, PSTR("\" -> \""), hubNamespace.c_str(), PSTR("\""));
  if (strncmp(flashData.hubNamespace, hubNamespace.c_str(), MAX_LENGTH_HUB_NAMESPACE) == 0) {
    return false;  // No Changes
  }
  Utils::sstrncpy(flashData.hubNamespace, hubNamespace.c_str(), MAX_LENGTH_HUB_NAMESPACE);
  return true;
}

void ICACHE_FLASH_ATTR handleSetupPost() {
  Logs::serialPrintlnStart(me, PSTR("handleSetupPost"));
  // Before persisting data, validate it first
  Storage::storageStruct flashData = Storage::readFlash();
  bool meshChanges = processMeshChanges(flashData);
  bool wifiChanges = processWifiChanges(flashData);
  bool hubChanges = processHubChanges(flashData);
  if (meshChanges || wifiChanges || hubChanges) {
    Storage::writeFlash(flashData);
    //String message((char *)0);
    //MessageGenerator::generateSharedInfo(message);
    //Network::broadcastEverywhere(message.c_str());
  }
  server.sendHeader(F("Connection"), F("close"), true);
  server.send(200, F("text/plain"), F("Success"));
  if (meshChanges || hubChanges) {
    Devices::restart();
  } else {
    Network::forceNetworkScan();
  }
  Logs::serialPrintlnEnd(me);
}

void ICACHE_FLASH_ATTR handlePingGet() {
  String message((char *)0);
  MessageGenerator::generateRawAction(message, F("pingDevices"));
  Network::broadcastEverywhere(message.c_str(), true, true);
  server.sendHeader(F("Connection"), F("close"), true);
  server.send(200, F("application/json"), F(""));
}

void handleCheckConnectionGet() {
  server.sendHeader(F("Connection"), F("close"), true);
  server.send(200, F("application/json"), F(""));
}

void serverSendContent(const String &content) {
  server.sendContent(content);
}

void handlePollGet() {
  Logs::serialPrintlnStart(me, PSTR("handlePollGet"));
  String message((char *)0);
  MessageGenerator::generateRawAction(message, F("pollDevices"));
  Network::broadcastEverywhere(message.c_str(), true, false);
  server.sendHeader(F("Connection"), F("close"), true);
  server.chunkedResponseModeStart(200, F("application/json"));
  // server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  // server.send(200, F("application/json"), F(""));
  MessageGenerator::generateChunkedMeshReport(serverSendContent);
  // server.sendContent(F(""));
  server.chunkedResponseFinalize();
  Network::forceNetworkScan(3000);
  Logs::serialPrintlnEnd(me);
}

void ICACHE_FLASH_ATTR handleAddDevicePost() {
  Logs::serialPrintlnStart(me, PSTR("handleAddDevicePost"));
  String SSID = server.arg(F("SSID"));
  server.sendHeader(F("Connection"), F("close"), true);
  server.send(200, F("text/plain"), F("Adding device"));

  String message((char *)0);
  MessageGenerator::generateRawAction(message, PSTR("addWifiDevice"), PSTR(""), PSTR(""), SSID);
  Network::broadcastEverywhere(message.c_str(), true, true);
  Logs::serialPrintlnEnd(me);
}

void ICACHE_FLASH_ATTR handleRestartPost() {
  Logs::serialPrintlnStart(me, PSTR("handleRestartPost"));
  String deviceId = server.arg(F("deviceId"));
  String message((char *)0);
  MessageGenerator::generateRawAction(message, F("restartDevice"), deviceId);
  Network::broadcastEverywhere(message.c_str(), true, true);
  Logs::serialPrintlnEnd(me);
  server.sendHeader(F("Connection"), F("close"), true);
  server.send(200, F("text/plain"), F("Restart started"));
}

void handleDeviceCommandPost() {
  Logs::serialPrintlnStart(me, PSTR("handleDeviceCommandPost"));
  String deviceId = server.arg(F("deviceId"));
  String deviceIndex = server.arg(F("deviceIndex"));
  String commandName = server.arg(F("commandName"));
  String msg((char *)0);
  MessageGenerator::generateRawAction(msg, F("handleDeviceCommand"), deviceId, deviceIndex, commandName);
  Logs::serialPrintln(me, msg.c_str());
  Network::broadcastEverywhere(msg.c_str(), true, true);
  server.sendHeader(F("Connection"), F("close"), true);
  server.send(200, F("text/plain"), F("handleDeviceCommand request issued"));
  Logs::serialPrintlnEnd(me);
}

void ICACHE_FLASH_ATTR handleUpdateDevicePost() {
  Logs::serialPrintlnStart(me, PSTR("handleUpdateDevicePost"));
  Mesh::Node deviceInfo;
  Utils::sstrncpy(
      deviceInfo.deviceId, String(server.arg(F("deviceId"))).c_str(), MAX_LENGTH_DEVICE_ID);
  Utils::sstrncpy(
      deviceInfo.deviceName, String(server.arg(F("deviceName"))).c_str(), MAX_LENGTH_DEVICE_NAME);
  String deviceInfoJson((char *)0);
  MessageGenerator::generateDeviceInfo(deviceInfoJson, deviceInfo, F("updateDevice"));
  Network::broadcastEverywhere(deviceInfoJson.c_str(), true, false);
  server.sendHeader(F("Connection"), F("close"), true);
  server.send(200, F("text/plain"), F("Device Update request issued"));
  Logs::serialPrintlnEnd(me);
}

void ICACHE_FLASH_ATTR handleRemoveDevicePost() {
  Logs::serialPrintlnStart(me, PSTR("handleRemoveDevicePost"));
  String deviceId = server.arg(F("deviceId"));
  server.sendHeader(F("Connection"), F("close"), true);
  server.send(200, F("text/plain"), F("Device removal started"));
  String message((char *)0);
  MessageGenerator::generateRawAction(message, F("removeDevice"), deviceId);
  Network::broadcastEverywhere(message.c_str(), true, true);
  Logs::serialPrintlnEnd(me);
}

void ICACHE_FLASH_ATTR handleConfigGet() {
  Logs::serialPrintlnStart(me, PSTR("handleConfigGet"));
  // Before persisting data, validate it first
  String sharedInfo((char *)0);
  MessageGenerator::generateSharedInfo(sharedInfo, true);
  server.sendHeader(F("Connection"), F("close"), true);
  server.send(200, F("application/json"), sharedInfo);
  Logs::serialPrintlnEnd(me);
}

static const char DEFAULT_PAGE[] PROGMEM = "/index.htm";

void getContentType(String &output, const String &path) {
  if (path.endsWith(FPSTR(".htm"))) {
    output = FPSTR("text/html");
  } else if (path.endsWith(".js")) {
    output = FPSTR("application/javascript; charset=utf-8");
  } else if (path.endsWith(".css")) {
    output = FPSTR("text/css; charset=utf-8");
  } else {
    output = FPSTR("*/*");
  }
}

void ICACHE_FLASH_ATTR handleNotFound() {
  // if (redirectToIP()) { return; }
  String path = ESP8266WebServer::urlDecode(server.uri());  // required to read paths with blanks
  if (path.isEmpty() || path == "/") {
    path = DEFAULT_PAGE;
  }

  // String acceptEncoding = server.header(FPSTR("Accept-Encoding"));
  // if (acceptEncoding.indexOf("gzip") >= 0 &&
  String contentType((char *)0);
  getContentType(contentType, path);
  String compressPath = path + FPSTR(".gz");
  if (Files::exists(compressPath)) {
    Logs::serialPrintln(me, PSTR("Path found:"), path.c_str(), PSTR(" (Compressed)"));
    // server.serveStatic(compressPath.c_str(), SPIFFS, compressPath.c_str(), ("max-age=43200"));
    Files::streamFile(compressPath, contentType, server);
  } else if (Files::exists(path)) {
    Logs::serialPrintln(me, PSTR("Path found:"), path.c_str());
    // server.serveStatic(path.c_str(), SPIFFS, path.c_str(), ("max-age=43200"));
    Files::streamFile(path, contentType, server);
  } else if (!Events::onWebServerNotFound(server)) {
    Logs::serialPrintln(me, PSTR("Path not found:"), path.c_str());
    server.sendHeader(F("Connection"), F("close"), true);
    server.send(404, F("text/plain"), F("404: Not found"));
  }
}

void ICACHE_FLASH_ATTR handleFilesPage() {
  Logs::serialPrintln(me, PSTR("handleFilesPage"));
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.sendHeader(F("Connection"), F("close"), true);
  server.send(200, F("text/html"), F(""));
  server.sendContent(pageHeader);
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

void ICACHE_FLASH_ATTR stop() {
  server.stop();
  serverStarted = false;
}

void ICACHE_FLASH_ATTR setup() {
  if (serverStarted) {
    return;
  }
  Logs::serialPrintln(me, PSTR("setup"));
  server.stop();

  if (!serverHandlersSetup) {
    server.on("/update", HTTP_POST, handleSetupPost);
    server.on("/updateDevice", HTTP_POST, handleUpdateDevicePost);
    server.on("/addDevice", HTTP_POST, handleAddDevicePost);
    server.on("/removeDevice", HTTP_POST, handleRemoveDevicePost);
    server.on("/handleDeviceCommand", HTTP_POST, handleDeviceCommandPost);
    server.on("/restart", HTTP_POST, handleRestartPost);
    server.on("/config", HTTP_GET, handleConfigGet);
    // server.on("/logs", HTTP_GET, handleLogsGet);
    server.on("/ping", HTTP_GET, handlePingGet);
    server.on("/check", HTTP_GET, handleCheckConnectionGet);
    server.on("/poll", HTTP_GET, handlePollGet);
    server.on(
        "/uploadFile", HTTP_POST, []() { server.send(200); }, []() { Files::fileUpload(server); });
    server.on(
        "/deleteFile", HTTP_POST, []() { Files::fileDelete(server.arg(F("fname")), server); });
    server.on("/files", HTTP_GET, handleFilesPage);
    Events::onStartingWebServer(server);
    server.onNotFound(handleNotFound);
    serverHandlersSetup = true;
  }
  server.begin();
  serverStarted = true;

  Logs::serialPrintln(me, PSTR("HTTP server started"));
}

void handle() {
  server.handleClient();
}
#endif

}  // namespace WebServer

// void handleLogsGet() {
//   Logs::pauseLogging(true);
//   long newerThanTimestamp = server.arg(F("newerThanTimestamp")).toInt();
//   server.setContentLength(CONTENT_LENGTH_UNKNOWN);
//   server.send(200, F("application/json"), F(""));
//   MessageGenerator::generateChunkedLogsHistory(newerThanTimestamp, serverSendContent);
//   // String msg = MessageGenerator::generateRawAction("broadcastLogs");
//   // Network::broadcastEverywhere(msg);
//   server.sendContent(F(""));
//   Logs::pauseLogging(false);
// }
