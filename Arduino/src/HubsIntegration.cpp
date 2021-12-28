#include "HubsIntegration.h"
#include "Config.h"
#include "Logs.h"
#include "MessageGenerator.h"
#include "Network.h"
#include "Storage.h"
#include "UniversalPlugAndPlay.h"
#include "Utils.h"
#include "WebServer.h"

namespace HubsIntegration {

#ifndef DISABLE_HUBS
static const Logs::caller me = Logs::caller::Hub;

enum supportedHubs { nohub, hubitat };
static const String supportedHubIds[] = {FPSTR("nohub"), FPSTR("hubitat")};
static const String supportedHubNames[] = {FPSTR("No Hub"), FPSTR("Hubitat")};
static supportedHubs selectedHub;

uint32_t nextHubRefreshTimeMillis = 0;

bool _isHubEnabled = false;
bool isHubEnabled(const Storage::storageStruct &flashData) {
  if (!_isHubEnabled) {
    Logs::serialPrintln(me, PSTR("isHubEnabled: NotInitialized"));
    return false;
  } else if (strlen(flashData.hubApi) == 0) {
    Logs::serialPrintln(me, PSTR("isHubEnabled: NoHubApiDefined"));
    return false;
  } else if (strlen(flashData.hubToken) == 0) {
    Logs::serialPrintln(me, PSTR("isHubEnabled: NoHubTokenDefined"));
    return false;
  }
  return true;
}

/*****************************************************
                   HUBITAT
*****************************************************/

void ICACHE_FLASH_ATTR handleRegisterHub() {
  // storeHubDetails
#ifndef DISABLE_WEBSERVER
  ESP8266WebServer &server = WebServer::getServer();
  if (!server.hasArg(F("plain"))) {
    return;
  }
  Logs::serialPrintln(me, PSTR("Validating Hub"));
  String plainJson = server.arg(F("plain"));
  DynamicJsonDocument doc(1024);
  // auto doc = Utils::getJsonDoc();
  doc.clear();
  deserializeJson(doc, plainJson);
  doc.shrinkToFit();

  Storage::storageStruct flashData = Storage::readFlash();
  strcpy(flashData.hubApi, doc[F("api")].as<const char *>());
  strcpy(flashData.hubToken, doc[F("token")].as<const char *>());
  strcpy(flashData.hubNamespace, doc[F("namespace")].as<const char *>());
  Logs::serialPrint(me, PSTR("Stored Hub Details: "), String(flashData.hubApi).c_str());
  Logs::serialPrintln(me, PSTR(":"), String(flashData.hubNamespace).c_str());
  Storage::writeFlash(flashData);
  server.send(200, F("text/plain"), F("success"));
  doc.clear();
#endif
}

bool handleHubitatDeviceEvent(const Devices::DeviceState &state, const String &type) {
  Storage::storageStruct flashData = Storage::readFlash();
  if (!isHubEnabled(flashData)) {
    return false;
  }

  Logs::serialPrintlnStart(me, PSTR("handleHubitatDeviceEvent:"), String(state.deviceId).c_str());
  Logs::serialPrint(
      me, String(state.deviceTypeId).c_str(), PSTR(":"), String(state.eventName).c_str());
  Logs::serialPrintln(me, PSTR(":"), String(state.eventValue).c_str());

  char *uuid = UniversalPlugAndPlay::getUUID(state.deviceId, state.deviceIndex);
  Devices::DeviceState copyState;
  Utils::sstrncpy(copyState.deviceId, uuid, MAX_LENGTH_UUID);
  copyState.deviceIndex = state.deviceIndex;
  Utils::sstrncpy(copyState.deviceTypeId, state.deviceTypeId, MAX_LENGTH_DEVICE_TYPE_ID);
  Utils::sstrncpy(copyState.eventName, state.eventName, MAX_LENGTH_EVENT_NAME);
  copyState.eventValue = state.eventValue;
  
  String hubApi = String(flashData.hubApi);
  String separator((char *)0);
  if (!hubApi.endsWith(FPSTR("/"))) {
    separator = FPSTR("/");
  }
  String path = hubApi + separator + type;
  String auth = FPSTR("Bearer ");
  auth.concat(flashData.hubToken);
  String deviceEventJson((char *)0);
  MessageGenerator::generateDeviceEvent(deviceEventJson, copyState);
  String contentType = F("application/json");
  Network::httpPost(path, deviceEventJson, auth, contentType);
  Logs::serialPrintlnEnd(me);
  return true;
}

bool ICACHE_FLASH_ATTR setupHubitat() {
#ifndef DISABLE_WEBSERVER
  ESP8266WebServer &server = WebServer::getServer();
  server.on("/registerHub", HTTP_POST, handleRegisterHub);
#endif
  return true;
}
#endif

/*****************************************************
                   GENERAL
*****************************************************/
bool ICACHE_FLASH_ATTR postDeviceEvent(const Devices::DeviceState &state, const String &type) {
#ifndef DISABLE_HUBS
  if (!Mesh::isConnectedToWifi()) {
    return false;
  }

  if (selectedHub == supportedHubs::hubitat) {
    return handleHubitatDeviceEvent(state, type);
  } else {
    Logs::serialPrintln(me, PSTR("postDeviceEvent:"), String(selectedHub).c_str());
  }
#endif
  return false;
}

bool sendHeartbeat(const char *deviceId) {
#ifndef DISABLE_HUBS
  Mesh::Node *node = Mesh::getNodesTip();
  while (node != nullptr) {
    if (strncmp(node->deviceId, deviceId, MAX_LENGTH_DEVICE_ID) != 0) {
      node = node->next;
      continue;
    }
    Devices::DeviceDescription *currDevice = node->devices;
    while (currDevice != nullptr) {
      Devices::DeviceState state;
      Utils::sstrncpy(state.deviceId, node->deviceId, MAX_LENGTH_UUID);
      state.deviceIndex = currDevice->index;
      Utils::sstrncpy(state.deviceTypeId, currDevice->typeId, MAX_LENGTH_DEVICE_TYPE_ID);
      Utils::sstrncpy(state.eventName, currDevice->lastEventName, MAX_LENGTH_EVENT_NAME);
      state.eventValue = currDevice->lastEventValue;
      if (!postDeviceEvent(state, PSTR("heartbeat"))) {
        return false;
      }
      Logs::serialPrint(me, PSTR("sendHeartbeat: "), String(node->deviceId).c_str());
      Logs::serialPrintln(me, PSTR("-"), String(currDevice->index).c_str());
      currDevice = currDevice->next;
      yield();
    }
    return true;
  }
#endif
  return false;
}

void ICACHE_FLASH_ATTR setup() {
#ifndef DISABLE_HUBS
  Storage::storageStruct flashData = Storage::readFlash();
  String hubNamespace = String(flashData.hubNamespace);
  selectedHub = supportedHubs::nohub;

  if (hubNamespace == supportedHubIds[supportedHubs::hubitat]) {
    selectedHub = supportedHubs::hubitat;
    _isHubEnabled = setupHubitat();
  }
  Logs::serialPrintln(me, supportedHubNames[selectedHub].c_str(), PSTR(" has been setup"));
#endif
}

void handle() {
}

}  // namespace HubsIntegration