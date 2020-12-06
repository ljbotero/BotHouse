#include "Devices.h"
#include "Config.h"
#include "HubsIntegration.h"
#include "Logs.h"
#include "Mesh.h"
#include "MessageGenerator.h"
#include "Network.h"
#include "Storage.h"
#include "Utils.h"

namespace Devices {

const Logs::caller me = Logs::caller::Devices;

const String PATH_CONFIG_FILE = "/config.json";
const uint8_t CONSEQUTIVE_CHANGES_TO_RESTART = 8;
const uint16_t IGNORE_CHANGE_FASTER_THAN_MILLISECONDS = 500;
const uint16_t RESET_CONSECUTIVE_CHANGE_DETECTION_AFTER_MILLIS = 3000;

String _deviceName = "";
unsigned long _restartAt = 0;

DeviceDescription* _rootDevice = NULL;
PinState* _rootPinStates = NULL;

DeviceDescription* getRootDevice() {
  return _rootDevice;
}

DeviceDescription* getDeviceFromIndex(DeviceDescription* root, uint8_t index) {
  while (root != nullptr) {
    if (root->index == index) {
      return root;
    }
    root = root->next;
  }
  return NULL;
}

DeviceTriggerDescription* deserializeTriggers(JsonObject& device, JsonArray& triggersArray) {
  DeviceTriggerDescription* triggers = NULL;
  DeviceTriggerDescription* currTrigger = NULL;
  for (JsonObject trigger : triggersArray) {
    if (currTrigger == nullptr) {
      currTrigger = new DeviceTriggerDescription;
      triggers = currTrigger;
    } else {
      currTrigger->next = new DeviceTriggerDescription;
      currTrigger = currTrigger->next;
    }
    currTrigger->next = NULL;
    currTrigger->onEvent = device[F("onEvent")].as<String>();
    currTrigger->fromDeviceId = device[F("fromDeviceId")].as<String>();
    currTrigger->runCommand = device[F("runCommand")].as<String>();
  }
  return triggers;
}

DeviceCommandDescription* deserializeCommands(JsonObject& device, JsonArray& commandsArray) {
  DeviceCommandDescription* commands = NULL;
  DeviceCommandDescription* currCommand = NULL;
  for (JsonObject command : commandsArray) {
    if (currCommand == nullptr) {
      currCommand = new DeviceCommandDescription;
      commands = currCommand;
    } else {
      currCommand->next = new DeviceCommandDescription;
      currCommand = currCommand->next;
    }
    currCommand->next = NULL;
    currCommand->commandName = device[F("commandName")].as<String>();
    currCommand->action = device[F("action")].as<String>();
    currCommand->pinId = device[F("pinId")].as<uint8_t>();
    currCommand->value = device[F("value")];
    currCommand->values = device[F("values")].as<String>();
  }
  return commands;
}

DeviceEventDescription* deserializeEvents(JsonObject& device, JsonArray& eventsArray) {
  DeviceEventDescription* events = NULL;
  DeviceEventDescription* currEvent = NULL;
  for (JsonObject event : eventsArray) {
    if (currEvent == nullptr) {
      currEvent = new DeviceEventDescription;
      events = currEvent;
    } else {
      currEvent->next = new DeviceEventDescription;
      currEvent = currEvent->next;
    }
    currEvent->next = NULL;
    currEvent->eventName = device[F("eventName")].as<String>();
    currEvent->pinId = device[F("pinId")].as<uint8_t>();
    currEvent->startRange = device[F("startRange")];
    currEvent->endRange = device[F("endRange")];
    currEvent->isDigital = device[F("isDigital")];
    currEvent->delay = device[F("delay")].as<uint16_t>();
  }
  return events;
}

DeviceDescription* deserializeDevices(JsonArray& devicesArray) {
  DeviceDescription* devices = NULL;
  Devices::DeviceDescription* currDevice = NULL;
  for (JsonObject device : devicesArray) {
    if (currDevice == nullptr) {
      currDevice = new Devices::DeviceDescription;
      devices = currDevice;
    } else {
      currDevice->next = new Devices::DeviceDescription;
      currDevice = currDevice->next;
    }
    currDevice->next = NULL;
    currDevice->index = device[F("index")].as<uint8_t>();
    currDevice->typeId = device[F("typeId")].as<String>();
    currDevice->lastEventName = device[F("lastEventName")].as<String>();
    JsonArray events = device[F("events")].as<JsonArray>();
    currDevice->events = deserializeEvents(device, events);
    JsonArray commands = device[F("commands")].as<JsonArray>();
    currDevice->commands = deserializeCommands(device, commands);
    JsonArray triggers = device[F("triggers")].as<JsonArray>();
    currDevice->triggers = deserializeTriggers(device, triggers);
  }
  return devices;
}

void serializeDevices(JsonObject& container, DeviceDescription* devicesObject) {
  JsonArray devices = container.createNestedArray(F("devices"));
  while (devicesObject != nullptr) {
    JsonObject device = devices.createNestedObject();
    device[F("index")] = devicesObject->index;
    device[F("typeId")] = devicesObject->typeId;
    device[F("lastEventName")] = devicesObject->lastEventName;
    JsonArray events = device.createNestedArray(F("events"));
    DeviceEventDescription* currEvent = devicesObject->events;
    while (currEvent != nullptr) {
      JsonObject event = events.createNestedObject();
      event[F("eventName")] = currEvent->eventName;
      event[F("pinId")] = currEvent->pinId;
      event[F("startRange")] = currEvent->startRange;
      event[F("endRange")] = currEvent->endRange;
      event[F("isDigital")] = currEvent->isDigital;
      event[F("delay")] = currEvent->delay;
      currEvent = currEvent->next;
    }

    JsonArray commands = device.createNestedArray(F("commands"));
    DeviceCommandDescription* currCommand = devicesObject->commands;
    while (currCommand != nullptr) {
      JsonObject command = commands.createNestedObject();
      command[F("commandName")] = currCommand->commandName;
      command[F("action")] = currCommand->action;
      command[F("pinId")] = currCommand->pinId;
      command[F("value")] = currCommand->value;
      command[F("values")] = currCommand->values;
      currCommand = currCommand->next;
    }

    JsonArray triggers = device.createNestedArray(F("triggers"));
    DeviceTriggerDescription* currTrigger = devicesObject->triggers;
    while (currTrigger != nullptr) {
      JsonObject trigger = triggers.createNestedObject();
      trigger[F("onEvent")] = currTrigger->onEvent;
      trigger[F("fromDeviceId")] = currTrigger->fromDeviceId;
      trigger[F("runCommand")] = currTrigger->runCommand;
      currTrigger = currTrigger->next;
    }

    devicesObject = devicesObject->next;
  }
}

void deleteDevices(DeviceDescription* root) {
  if (root == getRootDevice()) {
    return;
  }
  while (root != nullptr) {
    DeviceEventDescription* event = root->events;
    while (event != nullptr) {
      DeviceEventDescription* nextEvent = event->next;
      delete event;
      event = nextEvent;
    }
    DeviceCommandDescription* command = root->commands;
    while (command != nullptr) {
      DeviceCommandDescription* nextCommand = command->next;
      delete command;
      command = nextCommand;
    }
    DeviceTriggerDescription* trigger = root->triggers;
    while (trigger != nullptr) {
      DeviceTriggerDescription* nextTrigger = trigger->next;
      delete trigger;
      trigger = nextTrigger;
    }
    DeviceDescription* nextRoot = root->next;
    delete root;
    root = nextRoot;
  }
}

void restart() {
  _restartAt = millis() + 3000;
  Logs::serialPrintln(me, F("RESTARTING..."));
}

String getDeviceName(Storage::storageStruct* flashDataPtr) {
  if (flashDataPtr != nullptr) {
    _deviceName = flashDataPtr->deviceName;
  } else if (_deviceName.isEmpty()) {
    Storage::storageStruct flashData = Storage::readFlash();
    _deviceName = flashData.deviceName;
  }

  if (_deviceName.isEmpty()) {
    _deviceName = Utils::getChipIdString();
  }
  return _deviceName;
}

void updateDevice(const String& deviceName) {
  Logs::serialPrintlnStart(me, F("updateDevice('"), deviceName, F("')"));
  Storage::storageStruct flashData = Storage::readFlash();
  strncpy(flashData.deviceName, deviceName.c_str(), MAX_LENGTH_DEVICE_NAME);
  Storage::writeFlash(flashData, false);
  _deviceName = "";
  Logs::serialPrintlnEnd(me);
}

void detectConsecutiveChanges(PinState* currState) {
  if (millis() > currState->nextConsecutiveChangeBefore) {
    currState->consecutiveChangesCount = 0;
    return;
  }
  currState->consecutiveChangesCount++;
  currState->nextConsecutiveChangeBefore =
      millis() + RESET_CONSECUTIVE_CHANGE_DETECTION_AFTER_MILLIS;
  Logs::serialPrintln(
      me, F("Consecutive change detected #"), String(currState->consecutiveChangesCount));
  if (currState->consecutiveChangesCount >= CONSEQUTIVE_CHANGES_TO_RESTART) {
    Logs::serialPrintln(me, F("Restarting..."));
    restart();
  }
}

PinState* getPinState(uint8_t pinId) {
  PinState* currState = _rootPinStates;
  while (currState != nullptr) {
    if (currState->pinId == pinId) {
      return currState;
    }
    currState = currState->next;
  }
  return NULL;
}

void executeTrigger(
    DeviceDescription* currDevice, const String& eventName, DeviceTriggerDescription* currTrigger) {
  if (currTrigger->fromDeviceId != "" && currTrigger->fromDeviceId != Utils::getChipIdString()) {
    return;
  }
  if (currTrigger->runCommand == "Broadcast") {
    DeviceState state = {
        Utils::getChipIdString(), currDevice->index, currDevice->typeId, eventName, ""};
    String msg = MessageGenerator::generateDeviceEvent(state);
    Network::broadcastMessage(msg, true, false);
    Logs::serialPrintln(me, F("executeTrigger: Broadcast"));
    return;
  }
  // Search for command that can run the trigger
  if (handleCommand(currDevice, currTrigger->runCommand)) {
    Logs::serialPrintln(me, F("executeTrigger: "), currTrigger->runCommand);
  }
}

void checkTriggers(DeviceDescription* currDevice, const String& eventName) {
  // Check if there are any triggers subscribed to this event
  DeviceTriggerDescription* currTrigger = currDevice->triggers;
  while (currTrigger != nullptr) {
    if (currTrigger->onEvent == eventName) {
      executeTrigger(currDevice, eventName, currTrigger);
    }
    currTrigger = currTrigger->next;
  }
}

PinState* createNewPinState(uint8_t pinId, int value, uint16_t delay = 100) {
  PinState* currState = new PinState;
  currState->pinId = pinId;
  currState->nextAllowedChange = millis() + delay;
  currState->value = -value;  // setting it negative to force change detection
  currState->nextValue = value;
  currState->lastReadValue = value;
  currState->consecutiveChangesCount = 0;
  if (_rootPinStates == nullptr) {
    currState->next = NULL;
  } else {
    currState->next = _rootPinStates;
  }
  _rootPinStates = currState;
  return currState;
}

void setNextPinState(uint8_t pinId, int value) {
  PinState* currState = getPinState(pinId);
  if (currState == nullptr) {
    createNewPinState(pinId, value);
  } else {
    currState->nextValue = value;
  }
}

bool setPinState(PinState* currState, int value, uint16_t delay = 100) {
  bool changed = false;
  if (currState->nextValue != currState->value) {
    currState->value = currState->nextValue;
    changed = true;
    Logs::serialPrint(me, F("Pin "), String(currState->pinId));
    Logs::serialPrintln(me, F(" changed to proposed: "), String(value));
  } else if (currState->lastReadValue != value && currState->nextAllowedChange < millis()) {
    currState->nextAllowedChange = millis() + delay;
    currState->value = value;
    currState->nextValue = value;
    currState->lastReadValue = value;
    changed = true;
    Logs::serialPrint(me, F("Pin "), String(currState->pinId));
    Logs::serialPrintln(me, F(" changed to: "), String(value));
    detectConsecutiveChanges(currState);
  }
  return changed;
}

bool handleCommand(DeviceDescription* currDevice, String commandName) {
  Logs::serialPrint(me, F("handleCommand:"), commandName);
  // find command
  DeviceCommandDescription* currCommand = currDevice->commands;
  bool handled = false;
  while (currCommand != nullptr) {
    if (currCommand->commandName == commandName) {
      // Execute different actions
      if (currCommand->action == "toggleDigital") {
        int nextValue = LOW;
        PinState* pinState = getPinState(currCommand->pinId);
        if (pinState != nullptr && pinState->value == LOW) {
          nextValue = HIGH;
        }
        Logs::serialPrintln(me, F(""));
        Logs::serialPrint(me, F("toggleDigital:"), String(pinState->value));
        Logs::serialPrintln(me, F("->"), String(nextValue));
        setNextPinState(currCommand->pinId, nextValue);
        handled = true;
      } else if (currCommand->action == "writeDigital") {
        digitalWrite(currCommand->pinId, currCommand->value);
        setNextPinState(currCommand->pinId, currCommand->value);
        handled = true;
      } else if (currCommand->action == "writeSerial") {
        Logs::serialPrintln(me, F("writeSerial:"), currCommand->values);
        const char *inStr = currCommand->values.c_str();
        char tmp[] = "12", c = 'x';
        int i = 0; int len = strlen(inStr); int n = 0;
        for (i = 0; i < len; i += 2) {
          tmp[0] = inStr[i];
          tmp[1] = inStr[i + 1];
          n = strtol(tmp, NULL, 16); 
          c = (char)n;
          Serial.write(c);
        }
        setNextPinState(currCommand->pinId, currCommand->value);
        handled = true;
      } else if (currCommand->action == "setState") {
        setNextPinState(currCommand->pinId, currCommand->value);
        handled = true;
      }
    }
    currCommand = currCommand->next;
  }
  if (handled) {
    Logs::serialPrintln(me, F(":handled"));
  } else {
    Logs::serialPrintln(me, F(":unhandled"));
  }
  return handled;
}

// Finds and runs a command with and action=setState and value=state
bool setDeviceSate(uint8_t deviceIndex, int state) {
  DeviceDescription* device = getDeviceFromIndex(getRootDevice(), deviceIndex);
  if (device == nullptr) {
    return false;
  }
  // Find command that sets state and matches value
  DeviceCommandDescription* currCommand = device->commands;
  bool handled = false;
  while (currCommand != nullptr) {
    if ((currCommand->action == "setState" || currCommand->action == "writeDigital" ||
            currCommand->action == "writeSerial") &&
        currCommand->value == state) {
      Logs::serialPrintln(me, F("setDeviceSate:"), currCommand->commandName);
      handleCommand(device, currCommand->commandName);
      return true;
    }
    currCommand = currCommand->next;
  }
  Logs::serialPrintln(me, F("setDeviceSate: FAILED"));
  return false;
}

// This is for reader devices
void detectEvents() {
  DeviceDescription* currDevice = _rootDevice;
  while (currDevice != nullptr) {
    DeviceEventDescription* currEvent = currDevice->events;
    while (currEvent != nullptr) {
      int value = 0;
      PinState* currState = getPinState(currEvent->pinId);
      if (currState != nullptr && currState->nextValue != currState->value) {
        value = currState->nextValue;
      } else if (currEvent->isDigital) {
        value = digitalRead(currEvent->pinId);
      } else {
        value = analogRead(currEvent->pinId);
      }

      if (currState == nullptr) {
        currState = createNewPinState(currEvent->pinId, value, currEvent->delay);
        Logs::serialPrint(me, F("Pin "), String(currEvent->pinId));
        Logs::serialPrintln(me, F(" started to: "), String(value));
      }

      if (currEvent->startRange <= value && currEvent->endRange >= value) {
        bool changed = setPinState(currState, value, currEvent->delay);
        if (changed) {
          Logs::serialPrintln(me, F("Event detected "), currEvent->eventName);
          currDevice->lastEventName = currEvent->eventName;
          checkTriggers(currDevice, currEvent->eventName);
        }
      }
      currEvent = currEvent->next;
    }
    currDevice = currDevice->next;
  }
}

void loadSetup(DeviceDescription* currDevice, const JsonObject& jsonSetup) {
  uint8_t pinId = jsonSetup[F("pinId")].as<uint8_t>();
  String mode = jsonSetup[F("mode")].as<String>();
  if (mode == "INPUT_PULLUP") {
    pinMode(pinId, INPUT_PULLUP);
    Logs::serialPrintln(me, F("   Setup:"), pinId + ":mode=" + mode);
  } else if (mode == "INPUT") {
    pinMode(pinId, INPUT);
    Logs::serialPrintln(me, F("   Setup:"), pinId + ":mode=" + mode);
  } else if (mode == "OUTPUT") {
    bool isDigital = jsonSetup[F("isDigital")].as<bool>();
    int initialValue = jsonSetup[F("initialValue")].as<int>();
    pinMode(pinId, OUTPUT);
    if (isDigital) {
      digitalWrite(pinId, initialValue);
    } else {
      analogWrite(pinId, initialValue);
    }
    setNextPinState(pinId, initialValue);
    Logs::serialPrintln(me, F("   Setup:"), pinId + ":mode=" + mode);
  }
}

void loadEvent(DeviceDescription* currDevice, const JsonObject& jsonEvent) {
  DeviceEventDescription* currEvent = new DeviceEventDescription;
  if (currDevice->events == nullptr) {
    currDevice->events = currEvent;
    currDevice->events->next = NULL;
  } else {
    currEvent->next = currDevice->events;
    currDevice->events = currEvent;
  }
  currEvent->eventName = jsonEvent[F("eventName")].as<String>();
  currEvent->pinId = jsonEvent[F("pinId")].as<uint8_t>();
  currEvent->startRange = jsonEvent[F("startRange")].as<int>();
  currEvent->endRange = jsonEvent[F("endRange")].as<int>();
  currEvent->isDigital = jsonEvent[F("isDigital")].as<bool>();
  currEvent->delay = jsonEvent[F("delay")].as<uint16_t>();
  Logs::serialPrintln(me, F("   Event:"),
      currEvent->eventName + ":" + currEvent->pinId + ":startRange=" +
          String(currEvent->startRange) + ":endRange=" + String(currEvent->endRange) +
          ":isDigital=" + String(currEvent->isDigital) + ":delay=" + String(currEvent->delay));
}

void loadCommand(DeviceDescription* currDevice, const JsonObject& jsonCommand) {
  DeviceCommandDescription* currCommand = new DeviceCommandDescription;
  if (currDevice->commands == nullptr) {
    currDevice->commands = currCommand;
    currDevice->commands->next = NULL;
  } else {
    currCommand->next = currDevice->commands;
    currDevice->commands = currCommand;
  }
  currCommand->commandName = jsonCommand[F("commandName")].as<String>();
  currCommand->action = jsonCommand[F("action")].as<String>();
  currCommand->pinId = jsonCommand[F("pinId")].as<uint8_t>();
  currCommand->value = jsonCommand[F("value")].as<int>();
  currCommand->values = jsonCommand[F("values")].as<String>();
  if (currCommand->values == nullptr) {
    currCommand->values = "";
  }
  Logs::serialPrintln(me, F("   Command:"),
      currCommand->commandName + ":" + currCommand->action +
          ":pinId=" + String(currCommand->pinId) + ":value=" + String(currCommand->value) +
          +":values=" + currCommand->values);
}

void loadTrigger(DeviceDescription* currDevice, const JsonObject& jsonTrigger) {
  DeviceTriggerDescription* currTrigger = new DeviceTriggerDescription;
  if (currDevice->triggers == nullptr) {
    currDevice->triggers = currTrigger;
    currDevice->triggers->next = NULL;
  } else {
    currTrigger->next = currDevice->triggers;
    currDevice->triggers = currTrigger;
  }
  currTrigger->onEvent = jsonTrigger[F("onEvent")].as<String>();
  currTrigger->fromDeviceId = jsonTrigger[F("fromDeviceId")].as<String>();
  currTrigger->runCommand = jsonTrigger[F("runCommand")].as<String>();
  Logs::serialPrintln(me, F("   Trigger:"),
      currTrigger->onEvent + ":" + currTrigger->fromDeviceId + ":" + currTrigger->runCommand);
}

void loadConfig() {
  String configFile = "/config_" + Utils::getChipIdString() + ".json";
  if (!SPIFFS.exists(configFile)) {
    configFile = PATH_CONFIG_FILE;
    if (!SPIFFS.exists(configFile)) {
      Logs::serialPrintln(me, F("ERROR:loadConfig:Config.json NOT FOUND!!"));
      return;
    }
  }
  Logs::serialPrintln(me, F("loadConfig"));
  File file = SPIFFS.open(configFile, "r");
  DynamicJsonDocument doc(1024 * 5);
  deserializeJson(doc, file);
  doc.shrinkToFit();
  file.close();

  uint8_t currIndex = 0;
  DeviceDescription* currDevice = NULL;
  JsonArray jsonDevices = doc[F("devices")].as<JsonArray>();
  for (JsonObject jsonDevice : jsonDevices) {
    String deviceId = jsonDevice[F("deviceId")].as<String>();
    if (deviceId.length() > 0 && deviceId != Utils::getChipIdString()) {
      Logs::serialPrint(me, F("skip:"), deviceId);
      Logs::serialPrintln(me, F(" != "), Utils::getChipIdString());
      continue;
    }
    Logs::serialPrintln(me, F("found:"), deviceId);
    currDevice = new DeviceDescription;
    if (_rootDevice == nullptr) {
      currDevice->next = NULL;
    } else {
      currDevice->next = _rootDevice;
    }
    _rootDevice = currDevice;
    currDevice->index = currIndex++;
    currDevice->events = NULL;
    currDevice->commands = NULL;
    currDevice->triggers = NULL;
    currDevice->typeId = jsonDevice[F("typeId")].as<String>();
    String description = jsonDevice[F("description")].as<String>();
    Logs::serialPrintln(me, F("Name: "), description);
    Logs::serialPrintln(me, F("Index: "), String(currDevice->index));
    Logs::serialPrintln(me, F("Device: "), deviceId + ":" + currDevice->typeId);
    JsonArray jsonSetups = jsonDevice[F("setup")].as<JsonArray>();
    for (JsonObject jsonSetup : jsonSetups) {
      loadSetup(currDevice, jsonSetup);
    }
    JsonArray jsonEvents = jsonDevice[F("events")].as<JsonArray>();
    for (JsonObject jsonEvent : jsonEvents) {
      loadEvent(currDevice, jsonEvent);
    }
    JsonArray jsonCommands = jsonDevice[F("commands")].as<JsonArray>();
    for (JsonObject jsonCommand : jsonCommands) {
      loadCommand(currDevice, jsonCommand);
    }
    JsonArray jsonTriggers = jsonDevice[F("triggers")].as<JsonArray>();
    for (JsonObject jsonTrigger : jsonTriggers) {
      loadTrigger(currDevice, jsonTrigger);
    }
  }
}

void setup() {
  loadConfig();
  pinMode(BUILTIN_LED, OUTPUT);
}

void handle() {
  if (_restartAt > 0 && _restartAt < millis()) {
    ESP.restart();
  }
  detectEvents();
}

}  // namespace Devices
