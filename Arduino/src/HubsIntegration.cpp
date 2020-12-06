#include "HubsIntegration.h"
#include <ArduinoJson.h>
#include "Config.h"
#include "Logs.h"
#include "MessageGenerator.h"
#include "Network.h"
#include "Storage.h"
#include "UniversalPlugAndPlay.h"
#include "Utils.h"
#include "WebServer.h"

namespace HubsIntegration {

const Logs::caller me = Logs::caller::Hub;
String amazonUserId;

typedef enum { nohub, hubitat } supportedHubs;
const String supportedHubIds[] = {"nohub", "hubitat"};
const String supportedHubNames[] = {"No Hub", "Hubitat"};
supportedHubs selectedHub;

uint32_t nextHubRefreshTimeMillis = 0;

bool _isHubEnabled = false;
bool isHubEnabled(const Storage::storageStruct &flashData) {
  if (!_isHubEnabled) {
    Logs::serialPrintln(me, F("isHubEnabled: NotInitialized"));
    return false;
  } else if (strlen(flashData.hubApi) == 0) {
    Logs::serialPrintln(me, F("isHubEnabled: NoHubApiDefined"));
    return false;
  } else if (strlen(flashData.hubToken) == 0) {
    Logs::serialPrintln(me, F("isHubEnabled: NoHubTokenDefined"));
    return false;
  }
  return true;
}

/*****************************************************
                   HUBITAT
*****************************************************/

void handleRegisterHub() {
  // storeHubDetails
  ESP8266WebServer &server = WebServer::getServer();
  if (!server.hasArg(F("plain"))) {
    return;
  }
  Logs::serialPrintln(me, F("Validating Hub"));
  String plainJson = server.arg(F("plain"));
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, plainJson);
  doc.shrinkToFit();

  Storage::storageStruct flashData = Storage::readFlash();
  strcpy(flashData.hubApi, doc[F("api")].as<const char *>());
  strcpy(flashData.hubToken, doc[F("token")].as<const char *>());
  strcpy(flashData.hubNamespace, doc[F("namespace")].as<const char *>());
  Logs::serialPrintln(me, F("Stored Hub Details: "), flashData.hubApi,
      String(FPSTR(":")) + String(flashData.hubNamespace));
  Storage::writeFlash(flashData);
  server.send(200, F("text/plain"), F("success"));
}

bool handleAlexaDeviceEvent(Devices::DeviceState state) {
#ifndef DISABLE_ALEXA_SKILL
  if (String(state.deviceTypeId) != FPSTR("contact")) {
    return true;
  }
  const String alexaLambdaAPIKey = F("4rHOU0GUJv3rzDUEcMAUv5dq0fSweJsg3MGlpEfI");
  const String alexaLambdaURL = F("https://l32ezbt5b8.execute-api.us-east-1.amazonaws.com/default");

  Logs::serialPrintlnStart(me, F("handleAlexaDeviceEvent:"));
  String payload = MessageGenerator::generateAlexaDeviceEvent(state);
  String contentType = F("application/json");
  Network::httpsPost(alexaLambdaURL, payload, F(""), contentType, alexaLambdaAPIKey, ROOT_CA);
  Logs::serialPrintlnEnd(me);
#endif
  return true;
}

bool handleHubitatDeviceEvent(Devices::DeviceState state) {
  Storage::storageStruct flashData = Storage::readFlash();
  if (!isHubEnabled(flashData)) {
    return false;
  }

  Logs::serialPrintlnStart(me, F("handleHubitatDeviceEvent:"),
      state.deviceId + ":" + state.deviceTypeId + ":" + state.eventName + ":" + state.eventValue);

  char *uuid = UniversalPlugAndPlay::getUUID(state.deviceId, state.deviceIndex);
  state.deviceId = uuid;
  String hubApi = String(flashData.hubApi);
  String separator = FPSTR("");
  if (!hubApi.endsWith(FPSTR("/"))) {
    separator = FPSTR("/");
  }
  String path = hubApi + separator + FPSTR("deviceEvent");
  String auth = FPSTR("Bearer ");
  auth.concat(flashData.hubToken);
  String payload = MessageGenerator::generateDeviceEvent(state);
  String contentType = F("application/json");
  Network::httpsPost(path, payload, auth, contentType);
  Logs::serialPrintlnEnd(me);
  return true;
}

bool setupHubitat() {
  ESP8266WebServer &server = WebServer::getServer();
  server.on("/registerHub", HTTP_POST, handleRegisterHub);
  return true;
}

/*****************************************************
                   GENERAL
*****************************************************/
bool postDeviceEvent(Devices::DeviceState state) {
  if (!WiFi.isConnected()) {
    return false;
  }

  if (!amazonUserId.isEmpty()) {
    handleAlexaDeviceEvent(state);
  }

  if (selectedHub == supportedHubs::hubitat) {
    return handleHubitatDeviceEvent(state);
  } else {
    Logs::serialPrintln(me, F("postDeviceEvent:"), String(selectedHub));
  }

  return false;
}

bool sendHeartbeat(const String &deviceId) {
  Mesh::Node *node = Mesh::getNodesTip();
  while (node != nullptr) {
    if (node->deviceId != deviceId) {
      node = node->next;
      continue;
    }
    Devices::DeviceDescription *currDevice = node->devices;
    while (currDevice != nullptr) {
      Devices::DeviceState state = {node->deviceId, currDevice->index, currDevice->typeId,
          "Heartbeat", currDevice->lastEventName};
      if (!postDeviceEvent(state)) {
        return false;
      }
      Logs::serialPrint(me, F("sendHeartbeat: "), node->deviceId);
      Logs::serialPrintln(me, F("-"), String(currDevice->index));
      currDevice = currDevice->next;
    }
    return true;
  }
  return false;
}

void setup() {
  Storage::storageStruct flashData = Storage::readFlash();
  String hubNamespace = String(flashData.hubNamespace);
  selectedHub = supportedHubs::nohub;

  if (hubNamespace == supportedHubIds[supportedHubs::hubitat]) {
    selectedHub = supportedHubs::hubitat;
    _isHubEnabled = setupHubitat();
  }
  Logs::serialPrintln(me, supportedHubNames[selectedHub], F(" has been setup"));

  // Setup Alexa
  amazonUserId = String(flashData.amazonUserId);
}

void handle() {
}

}  // namespace HubsIntegration