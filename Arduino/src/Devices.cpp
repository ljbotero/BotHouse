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

  static const Logs::caller me = Logs::caller::Devices;

  static const char PATH_CONFIG_FILE[] PROGMEM = "/config.json";
  static const auto CONSEQUTIVE_CHANGES_TO_RESTART = 8;
  static const auto IGNORE_CHANGE_FASTER_THAN_MILLISECONDS = 500;
  static const auto RESET_CONSECUTIVE_CHANGE_DETECTION_AFTER_MILLIS = 3000;

  static char _deviceName[MAX_LENGTH_DEVICE_NAME] = "\0";
  static unsigned long _restartAt = 0;
  static unsigned long _nextAnalogReadAt = 0;
  static unsigned long _lastAnalogReadAt = 0;
  static unsigned long _nextAnalogCheckChangeAt;
  static auto _analogCheckReadFrequencyMillis = 5;
  static int _lastAnalogReadValue = 0;

  DeviceDescription* _rootDevice = NULL;
  PinState* _rootPinStates = NULL;

#ifdef RUN_UNIT_TESTS
  void resetState() {
    _restartAt = 0;
    _nextAnalogReadAt = 0;
    _lastAnalogReadAt = 0;
    _nextAnalogCheckChangeAt;
    _analogCheckReadFrequencyMillis = 5;
    _lastAnalogReadValue = 0;
    _rootPinStates = NULL;
    _rootDevice = NULL;
  }
#endif


  DeviceDescription* getRootDevice() {
    return _rootDevice;
  }

  ICACHE_FLASH_ATTR DeviceDescription* getDeviceFromIndex(DeviceDescription* root, uint8_t index) {
    while (root != nullptr) {
      if (root->index == index) {
        return root;
      }
      root = root->next;
    }
    return NULL;
  }

  // ICACHE_FLASH_ATTR DeviceTriggerDescription* deserializeTriggers(JsonObject& device, JsonArray&
  // triggersArray) {
  //   DeviceTriggerDescription* triggers = NULL;
  //   DeviceTriggerDescription* currTrigger = NULL;
  //   for (JsonObject trigger : triggersArray) {
  //     if (currTrigger == nullptr) {
  //       currTrigger = new DeviceTriggerDescription;
  //       triggers = currTrigger;
  //     } else {
  //       currTrigger->next = new DeviceTriggerDescription;
  //       currTrigger = currTrigger->next;
  //     }
  //     currTrigger->next = NULL;
  //     Utils::sstrncpy(
  //         currTrigger->onEvent, device[F("onEvent")].as<char*>(), MAX_LENGTH_EVENT_NAME);
  //     Utils::sstrncpy(
  //         currTrigger->fromDeviceId, device[F("fromDeviceId")].as<char*>(), MAX_LENGTH_DEVICE_ID);
  //     Utils::sstrncpy(
  //         currTrigger->runCommand, device[F("runCommand")].as<char*>(), MAX_LENGTH_COMMAND_NAME);
  //   }
  //   return triggers;
  // }

  // ICACHE_FLASH_ATTR DeviceCommandDescription* deserializeCommands(JsonObject& device, JsonArray&
  // commandsArray) {
  //   DeviceCommandDescription* commands = NULL;
  //   DeviceCommandDescription* currCommand = NULL;
  //   for (JsonObject command : commandsArray) {
  //     if (currCommand == nullptr) {
  //       currCommand = new DeviceCommandDescription;
  //       commands = currCommand;
  //     } else {
  //       currCommand->next = new DeviceCommandDescription;
  //       currCommand = currCommand->next;
  //     }
  //     currCommand->next = NULL;
  //     Utils::sstrncpy(
  //         currCommand->commandName, device[F("commandName")].as<char*>(), MAX_LENGTH_COMMAND_NAME);
  //     Utils::sstrncpy(currCommand->action, device[F("action")].as<char*>(), MAX_LENGTH_ACTION);
  //     currCommand->pinId = device[F("pinId")].as<uint8_t>();
  //     currCommand->value = device[F("value")];
  //     Utils::sstrncpy(currCommand->values, device[F("values")].as<char*>(), MAX_LENGTH_VALUES);
  //   }
  //   return commands;
  // }

  // ICACHE_FLASH_ATTR DeviceEventDescription* deserializeEvents(JsonObject& device, JsonArray&
  // eventsArray) {
  //   DeviceEventDescription* events = NULL;
  //   DeviceEventDescription* currEvent = NULL;
  //   for (JsonObject event : eventsArray) {
  //     if (currEvent == nullptr) {
  //       currEvent = new DeviceEventDescription;
  //       events = currEvent;
  //     } else {
  //       currEvent->next = new DeviceEventDescription;
  //       currEvent = currEvent->next;
  //     }
  //     currEvent->next = NULL;
  //     Utils::sstrncpy(
  //         currEvent->eventName, device[F("eventName")].as<char*>(), MAX_LENGTH_EVENT_NAME);
  //     currEvent->pinId = device[F("pinId")].as<uint8_t>();
  //     currEvent->startRange = device[F("startRange")];
  //     currEvent->endRange = device[F("endRange")];
  //     currEvent->isDigital = device[F("isDigital")];
  //     currEvent->delay = device[F("delay")].as<uint16_t>();
  //   }
  //   return events;
  // }

  ICACHE_FLASH_ATTR DeviceDescription* deserializeDevices(const JsonArray& devicesArray) {
    DeviceDescription* devices = NULL;
    Devices::DeviceDescription* currDevice = NULL;
    for (JsonObject device : devicesArray) {
      if (currDevice == nullptr) {
        currDevice = new Devices::DeviceDescription;
        devices = currDevice;
      }
      else {
        currDevice->next = new Devices::DeviceDescription;
        currDevice = currDevice->next;
      }
      currDevice->next = NULL;
      currDevice->index = device[F("index")].as<uint8_t>();
      Utils::sstrncpy(currDevice->typeId, device[F("typeId")].as<char*>(), MAX_LENGTH_DEVICE_TYPE_ID);
      Utils::sstrncpy(
        currDevice->lastEventName, device[F("lastEventName")].as<char*>(), MAX_LENGTH_EVENT_NAME);
      currDevice->lastEventValue = device[F("lastEventValue")].as<int>();
      // JsonArray events = device[F("events")].as<JsonArray>();
      currDevice->events = NULL;  // deserializeEvents(device, events);
      // JsonArray commands = device[F("commands")].as<JsonArray>();
      currDevice->commands = NULL;  // deserializeCommands(device, commands);
      // JsonArray triggers = device[F("triggers")].as<JsonArray>();
      currDevice->triggers = NULL;  // deserializeTriggers(device, triggers);
    }
    return devices;
  }

  void ICACHE_FLASH_ATTR serializeDevices(JsonObject& container, DeviceDescription* devicesObject) {
    JsonArray devices = container.createNestedArray(F("devices"));
    while (devicesObject != nullptr) {
      JsonObject device = devices.createNestedObject();
      device[F("index")] = devicesObject->index;
      device[F("typeId")] = devicesObject->typeId;
      device[F("lastEventName")] = devicesObject->lastEventName;
      device[F("lastEventValue")] = devicesObject->lastEventValue;
      JsonArray events = device.createNestedArray(F("events"));
      // DeviceEventDescription* currEvent = devicesObject->events;
      // while (currEvent != nullptr) {
      //   JsonObject event = events.createNestedObject();
      //   event[F("eventName")] = currEvent->eventName;
      //   event[F("pinId")] = currEvent->pinId;
      //   event[F("startRange")] = currEvent->startRange;
      //   event[F("endRange")] = currEvent->endRange;
      //   event[F("isDigital")] = currEvent->isDigital;
      //   event[F("delay")] = currEvent->delay;
      //   currEvent = currEvent->next;
      // }

      JsonArray commands = device.createNestedArray(F("commands"));
      // DeviceCommandDescription* currCommand = devicesObject->commands;
      // while (currCommand != nullptr) {
      //   JsonObject command = commands.createNestedObject();
      //   command[F("commandName")] = currCommand->commandName;
      //   command[F("action")] = currCommand->action;
      //   command[F("pinId")] = currCommand->pinId;
      //   command[F("value")] = currCommand->value;
      //   command[F("values")] = currCommand->values;
      //   currCommand = currCommand->next;
      // }

      JsonArray triggers = device.createNestedArray(F("triggers"));
      // DeviceTriggerDescription* currTrigger = devicesObject->triggers;
      // while (currTrigger != nullptr) {
      //   JsonObject trigger = triggers.createNestedObject();
      //   trigger[F("onEvent")] = currTrigger->onEvent;
      //   trigger[F("fromDeviceId")] = currTrigger->fromDeviceId;
      //   trigger[F("runCommand")] = currTrigger->runCommand;
      //   currTrigger = currTrigger->next;
      // }

      devicesObject = devicesObject->next;
      yield();
    }
  }

  void ICACHE_FLASH_ATTR deleteDevices(DeviceDescription* root) {
    if (root == getRootDevice()) {
      Logs::serialPrintln(me, PSTR("[WARNING] Tried deleting local device definitions"));
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
      yield();
    }
  }

  void ICACHE_FLASH_ATTR restart() {
    _restartAt = millis() + 3000;
    Logs::serialPrintln(me, PSTR("RESTARTING..."));
  }

  char* getDeviceName(Storage::storageStruct* flashDataPtr) {
    if (flashDataPtr != nullptr) {
      Utils::sstrncpy(_deviceName, flashDataPtr->deviceName, MAX_LENGTH_DEVICE_NAME);
    }
    else if (strlen(_deviceName) == 0) {
      Storage::storageStruct flashData = Storage::readFlash();
      Utils::sstrncpy(_deviceName, flashData.deviceName, MAX_LENGTH_DEVICE_NAME);
    }

    if (strlen(_deviceName) == 0) {
      Utils::sstrncpy(_deviceName, chipId.c_str(), MAX_LENGTH_DEVICE_NAME);
    }
    return _deviceName;
  }

  void ICACHE_FLASH_ATTR updateDevice(const char* deviceName) {
    Logs::serialPrintlnStart(me, PSTR("updateDevice('"), deviceName, PSTR("')"));
    Storage::storageStruct flashData = Storage::readFlash();
    Utils::sstrncpy(flashData.deviceName, deviceName, MAX_LENGTH_DEVICE_NAME);
    Storage::writeFlash(flashData, false);
    _deviceName[0] = '\0';
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
    Logs::serialPrintln(me, PSTR("Consecutive change detected #"),
      String(currState->consecutiveChangesCount).c_str());
    if (currState->consecutiveChangesCount >= CONSEQUTIVE_CHANGES_TO_RESTART) {
      Logs::serialPrintln(me, PSTR("Restarting..."));
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

  void ICACHE_FLASH_ATTR broadcastTrigger(DeviceDescription* currDevice, char* onEvent, int eventValue) {
    currDevice->lastEventValue = eventValue;
    DeviceState state;
    Utils::sstrncpy(state.deviceId, chipId.c_str(), MAX_LENGTH_DEVICE_ID);
    state.deviceIndex = currDevice->index;
    Utils::sstrncpy(state.deviceTypeId, currDevice->typeId, MAX_LENGTH_DEVICE_TYPE_ID);
    Utils::sstrncpy(state.eventName, onEvent, MAX_LENGTH_EVENT_NAME);
    state.eventValue = eventValue;
    String deviceEventJson((char*)0);
    MessageGenerator::generateDeviceEvent(deviceEventJson, state);
    Network::broadcastEverywhere(deviceEventJson.c_str(), true, false);
  }

  void ICACHE_FLASH_ATTR executeTrigger(
    DeviceDescription* currDevice, DeviceTriggerDescription* currTrigger, PinState* currState,
    bool overrideValue = false) {
    if (strncmp(currTrigger->runCommand, "BroadcastMinuteCount", MAX_LENGTH_COMMAND_NAME) == 0
      && currState != nullptr) {
      currState->broadcastCount++;
    }
    else if (strncmp(currTrigger->runCommand, "Broadcast", MAX_LENGTH_COMMAND_NAME) == 0
      && currState != nullptr) {
      broadcastTrigger(currDevice, currTrigger->onEvent, currState->value);
    }
    else if (!handleCommand(currDevice, currTrigger->runCommand, overrideValue)) {
      Logs::serialPrintln(
        me, PSTR("FAILED:executeTrigger: "), String(currTrigger->runCommand).c_str());
      return;
    }
    Logs::serialPrintln(me, PSTR("executeTrigger: "), String(currTrigger->runCommand).c_str());
  }

  void checkScheduledTriggers(DeviceDescription* currDevice, const char* eventName, PinState* pinState) {
    // Check if there are any triggers subscribed to this event
    DeviceTriggerDescription* currTrigger = currDevice->triggers;
    while (currTrigger != nullptr) {
      if (strncmp(currTrigger->runCommand, "BroadcastMinuteCount", MAX_LENGTH_EVENT_NAME) == 0) {
        if (pinState->nextBroadcast < millis()) {
          Logs::serialPrintln(me, PSTR("checkScheduledTriggers:BroadcastMinuteCount:"),
            String(pinState->broadcastCount).c_str());
          if (currDevice->lastEventValue != pinState->broadcastCount) {
            broadcastTrigger(currDevice, currTrigger->onEvent, pinState->broadcastCount);
          }
          pinState->nextBroadcast = millis() + (1000 * 30);
          pinState->broadcastCount = 0;
        }
      }
      currTrigger = currTrigger->next;
    }
  }

  void checkTriggers(DeviceDescription* currDevice, const char* eventName, PinState* pinState) {
    // Check if there are any triggers subscribed to this event
    bool triggerFound = false;
    DeviceTriggerDescription* currTrigger = currDevice->triggers;
    while (currTrigger != nullptr) {
      if (strncmp(currTrigger->onEvent, eventName, MAX_LENGTH_EVENT_NAME) == 0 &&
        (currTrigger->fromDeviceId[0] == '\0' || String(currTrigger->fromDeviceId) == chipId)) {
        triggerFound = true;
        executeTrigger(currDevice, currTrigger, pinState);
        if (!currTrigger->disableHardReset) {
          detectConsecutiveChanges(pinState);
        }
      }
      currTrigger = currTrigger->next;
    }
    if (!triggerFound) {
      Logs::serialPrintln(me, PSTR("No trigger found for event: "), eventName);
    }
  }

  void ICACHE_FLASH_ATTR processEventsFromOtherDevices(const Devices::DeviceState& state) {
    Logs::serialPrintlnStart(me, PSTR("processEventsFromOtherDevices:"));
    String fromDeviceId;
    fromDeviceId.reserve(16);
    fromDeviceId += String(state.deviceId);
    fromDeviceId += FPSTR(":");
    fromDeviceId += String(state.deviceIndex);
    DeviceDescription* currDevice = _rootDevice;
    while (currDevice != nullptr) {
      DeviceTriggerDescription* currTrigger = currDevice->triggers;
      while (currTrigger != nullptr) {
        if (currTrigger->fromDeviceId[0] != '\0') {
          if (strncmp(currTrigger->fromDeviceId, fromDeviceId.c_str(), fromDeviceId.length()) == 0) {
            Logs::serialPrintln(me, PSTR("Match: "), fromDeviceId.c_str());
            if (strncmp(currTrigger->onEvent, state.eventName, MAX_LENGTH_EVENT_NAME) == 0) {
              executeTrigger(currDevice, currTrigger, NULL, true);
            }
            // } else {
            //   Logs::serialPrintln(me, PSTR("NoMatch:trigger:  "), currTrigger->fromDeviceId);
            //   Logs::serialPrintln(me, PSTR("NoMatch:Incoming: "), fromDeviceId.c_str());
          }
        }
        currTrigger = currTrigger->next;
        yield();
      }
      currDevice = currDevice->next;
    }
    Logs::serialPrintlnEnd(me);
  }

  PinState* createNewPinState(uint8_t pinId, int value,
    bool overrideValue, unsigned long delay = 100UL) {
    PinState* currState = new PinState;
    currState->pinId = pinId;
    currState->nextAllowedChange = millis() + delay;
    currState->nextBroadcast = millis() + (1000 * 30);
    currState->broadcastCount = 0;
    currState->value = -value;  // setting it negative to force change detection
    currState->nextValue = value;
    currState->overrideValue = overrideValue;
    currState->consecutiveChangesCount = 0;
    if (_rootPinStates == nullptr) {
      currState->next = NULL;
    }
    else {
      currState->next = _rootPinStates;
    }
    _rootPinStates = currState;
    return currState;
  }

  void setNextPinState(uint8_t pinId, int value, bool overrideValue = false) {
    PinState* currState = getPinState(pinId);
    if (currState == nullptr) {
      createNewPinState(pinId, value, overrideValue);
    }
    else if (overrideValue) {
      currState->nextValue = value;
      currState->overrideValue = overrideValue;
    }
    else if (!currState->overrideValue) {
      currState->nextValue = value;
    }
  }

  // bool ICACHE_FLASH_ATTR setPinState(PinState* currState, int value, DeviceEventDescription* currEvent) {
  //   bool changed = false;
  //   // if (currState->nextValue != currState->value) {
  //   //   changed = !(currEvent->startRange <= currState->nextValue && currEvent->endRange >= currState->nextValue);
  //   //   currState->value = currState->nextValue;
  //   //   if (changed) {
  //   //     Logs::serialPrint(me, PSTR("Pin "), String(currState->pinId).c_str());
  //   //     Logs::serialPrintln(me, PSTR(" changed to proposed: "), String(value).c_str());
  //   //   }
  //   // }
  //   // else 
  //   if (currState->value != value && currState->nextAllowedChange < millis()) {
  //     // Changed is true when last event value is not within the range for this event
  //     changed = !(currEvent->startRange <= currState->value && currEvent->endRange >= currState->value);
  //     currState->nextAllowedChange = millis() + currEvent->delay;
  //     currState->value = value;
  //     // currState->nextValue = value;
  //     if (changed) {
  //       Logs::serialPrint(me, PSTR("Pin "), String(currState->pinId).c_str());
  //       Logs::serialPrintln(me, PSTR(" changed to: "), String(value).c_str());
  //     }
  //   }
  //   // else if (currState->value == value) {
  //   //   currState->overrideValue = false;
  //   // }
  //   return changed;
  // }

  void ICACHE_FLASH_ATTR writeSerial(const char* inStr) {
    char tmp[] = "12";
    int i = 0;
    int len = strlen(inStr);
    for (i = 0; i < len; i += 2) {
      tmp[0] = inStr[i];
      tmp[1] = inStr[i + 1];
      auto n = strtol(tmp, NULL, 16);
      char c = (char)n;
      Serial.write(c);
    }
  }

  bool ICACHE_FLASH_ATTR handleCommand(DeviceDescription* currDevice, const char* commandName,
    bool overrideValue = false) {
    Logs::serialPrintln(me, PSTR("handleCommand:"), commandName);
    // find command
    DeviceCommandDescription* currCommand = currDevice->commands;
    bool handled = false;
    while (currCommand != nullptr && commandName != nullptr) {
      if (strncmp(currCommand->commandName, commandName, MAX_LENGTH_COMMAND_NAME) == 0) {
        // Execute different actions
        if (strcmp_P(currCommand->action, PSTR("toggleDigital")) == 0) {
          int nextValue = LOW;
          PinState* pinState = getPinState(currCommand->pinId);
          if (pinState != nullptr && pinState->value == LOW) {
            nextValue = HIGH;
          }
          Logs::serialPrint(me, PSTR("toggleDigital:"));
          if (pinState != nullptr) {
            Logs::serialPrint(me, String(pinState->value).c_str());
          }
          Logs::serialPrint(me, PSTR("->"), String(nextValue).c_str());
          setNextPinState(currCommand->pinId, nextValue, true);
          handled = true;
        }
        else if (strcmp_P(currCommand->action, PSTR("writeDigital")) == 0) {
          digitalWrite(currCommand->pinId, currCommand->value);
          Logs::serialPrint(me, PSTR("Pin:"), String(currCommand->pinId).c_str());
          Logs::serialPrintln(me, PSTR(" - digitalWrite:"), String(currCommand->value).c_str());
          setNextPinState(currCommand->pinId, currCommand->value, overrideValue);
          handled = true;
        }
        else if (strcmp_P(currCommand->action, PSTR("writeAnalog")) == 0) {
          analogWrite(currCommand->pinId, currCommand->value);
          Logs::serialPrint(me, PSTR("Pin:"), String(currCommand->pinId).c_str());
          Logs::serialPrintln(me, PSTR(" - analogWrite:"), String(currCommand->value).c_str());
          setNextPinState(currCommand->pinId, currCommand->value, overrideValue);
          handled = true;
        }
        else if (strcmp_P(currCommand->action, PSTR("writeSerial")) == 0) {
          // Logs::disableSerialLog(true);
          Logs::serialPrintln(
            me, PSTR("writeSerial:'"), String(currCommand->values).c_str(), PSTR("'"));
          writeSerial(currCommand->values);
          setNextPinState(currCommand->pinId, currCommand->value, overrideValue);
          handled = true;
        }
        else if (strcmp_P(currCommand->action, PSTR("readSerial")) == 0) {
          const uint32_t timeoutReadSerial = 10000;
          // Logs::disableSerialLog(true);
          Logs::serialPrint(me, PSTR("readSerial:"));
          uint32_t timeoutLimitReadSerial = millis() + timeoutReadSerial;
          while (Serial.available() == 0) {
            //delay(1);
            yield();
            if (millis() > timeoutLimitReadSerial) {
              Logs::serialPrint(me, PSTR("timeout!"));
              break;
            }
          }
          while (Serial.available() > 0) {
            Serial.read();
          }
          Logs::serialPrintln(me, PSTR(""));
          // setNextPinState(currCommand->pinId, currCommand->value);
          handled = true;
        }
        else if (strcmp_P(currCommand->action, PSTR("setState")) == 0) {
          setNextPinState(currCommand->pinId, currCommand->value, true);
          handled = true;
        }
        else {
          Logs::serialPrint(me, PSTR(":unhandledAction:"), String(currCommand->action).c_str(), PSTR(":"));
        }
      }
      currCommand = currCommand->next;
      yield();
    }
    if (handled) {
      Logs::serialPrintln(me, PSTR(":handled"));
    }
    else {
      Logs::serialPrintln(me, PSTR(":unhandled"));
    }
    return handled;
  }

  // Finds and runs a command with and action=setState and value=state
  bool ICACHE_FLASH_ATTR setDeviceSate(uint8_t deviceIndex, int state) {
    DeviceDescription* device = getDeviceFromIndex(getRootDevice(), deviceIndex);
    if (device == nullptr) {
      return false;
    }
    // Find command that sets state and matches value
    DeviceCommandDescription* currCommand = device->commands;
    while (currCommand != nullptr) {
      if ((strncmp(currCommand->action, "setState", MAX_LENGTH_ACTION) == 0 ||
        strncmp(currCommand->action, "writeDigital", MAX_LENGTH_ACTION) == 0 ||
        strncmp(currCommand->action, "writeSerial", MAX_LENGTH_ACTION) == 0) &&
        currCommand->value == state) {
        Logs::serialPrintln(me, PSTR("setDeviceSate:"), String(currCommand->commandName).c_str());
        handleCommand(device, currCommand->commandName);
        return true;
      }
      currCommand = currCommand->next;
    }
    Logs::serialPrintln(me, PSTR("[ERROR] setDeviceSate: FAILED"));
    return false;
  }

  int analogReadWithDelay(uint8_t pinId) {
    if (_nextAnalogReadAt >= millis()) {
      return _lastAnalogReadValue;
    }
    int value = analogRead(pinId);
    _nextAnalogReadAt = millis() + _analogCheckReadFrequencyMillis;
    if (_lastAnalogReadValue - 5 > value || _lastAnalogReadValue + 5 < value) {
      _lastAnalogReadAt = millis();
      _lastAnalogReadValue = value;
      if (_analogCheckReadFrequencyMillis != 5) {
        Logs::serialPrintln(me, PSTR("Analog signals detected - scanning more frequently"));
        _analogCheckReadFrequencyMillis = 5;
      }
    }
    if (_nextAnalogCheckChangeAt < millis()) {
      auto THREE_MINS = (1000 * 60 * 3);
      if (_lastAnalogReadAt < _nextAnalogCheckChangeAt - THREE_MINS
        && _analogCheckReadFrequencyMillis != 100) {
        Logs::serialPrintln(me, PSTR("No analog signals detected in last 3 minutes - scanning lass frequently"));
        _analogCheckReadFrequencyMillis = 100;
      }
      _nextAnalogCheckChangeAt = millis() + THREE_MINS;
    }
    //delay(3); // https://github.com/esp8266/Arduino/issues/1634
    return value;
  }

  // This is for reader devices
  void detectEvents() {
    DeviceDescription* currDevice = _rootDevice;
    while (currDevice != nullptr) {
      DeviceEventDescription* currEvent = currDevice->events;
      while (currEvent != nullptr) {
        yield();
        int value = 0;
        PinState* pinState = getPinState(currEvent->pinId);
        if (pinState != nullptr && pinState->nextValue != pinState->value) {
          value = pinState->nextValue;
        }
        else if (currEvent->isDigital) {
          value = digitalRead(currEvent->pinId);
        }
        else {
          value = analogReadWithDelay(currEvent->pinId);
        }

        if (pinState == nullptr) {
          pinState = createNewPinState(currEvent->pinId, value, false, currEvent->delay);
          Logs::serialPrint(me, PSTR("Pin "), String(currEvent->pinId).c_str());
          Logs::serialPrintln(me, PSTR(" started to: "), String(value).c_str());
        }

        if (pinState->nextValue == pinState->value && pinState->overrideValue) {
          if (value == pinState->value) {
            pinState->overrideValue = false;
          }
          else {
            currEvent = currEvent->next;
            continue;
          }
        }

        // If new values are within the range for this event 
        // and time for next allowed changed has passed, then...
        if (value >= currEvent->startRange && value <= currEvent->endRange
          && pinState->value != value && pinState->nextAllowedChange < millis()) {

          // An change event has occurred if previous values are not within this range of values
          // Or change difference exceeds a given value
          bool changed = pinState->value < currEvent->startRange
            || pinState->value > currEvent->endRange;

          bool changeOutsideRange = abs(pinState->value - value) > currEvent->raiseIfChanges
            && currEvent->raiseIfChanges > 0;

          if (currEvent->raiseIfChanges <= 0 || changeOutsideRange) {
            Logs::serialPrint(me, PSTR("Pin "), String(pinState->pinId).c_str());
            Logs::serialPrint(me, PSTR(" changed from: "), String(pinState->value).c_str());
            Logs::serialPrintln(me, PSTR(" to: "), String(value).c_str());
            pinState->nextAllowedChange = millis() + currEvent->delay;
            pinState->value = value;
            pinState->nextValue = value;
          }
          if (changed || changeOutsideRange) {
            Logs::serialPrintln(me, PSTR("Event detected "), String(currEvent->eventName).c_str());
            Utils::sstrncpy(currDevice->lastEventName, currEvent->eventName, MAX_LENGTH_EVENT_NAME);
            checkTriggers(currDevice, currEvent->eventName, pinState);
          }
        }
        checkScheduledTriggers(currDevice, currEvent->eventName, pinState);

        currEvent = currEvent->next;
      }
      currDevice = currDevice->next;
    }
  }


  void ICACHE_FLASH_ATTR loadSetup(DeviceDescription* currDevice, const JsonObject& jsonSetup) {
    uint8_t pinId = jsonSetup[F("pinId")].as<uint8_t>();
    String mode = jsonSetup[F("mode")].as<String>();
    String runCommand = jsonSetup[F("runCommand")].as<String>();
    if (mode == "INPUT_PULLUP") {
      pinMode(pinId, INPUT_PULLUP);
      Logs::serialPrint(me, PSTR("   Setup:"));
      Logs::serialPrintln(me, String(pinId).c_str(), PSTR(":mode="), mode.c_str());
    }
    else if (strcmp_P(mode.c_str(), PSTR("INPUT")) == 0) {
      pinMode(pinId, INPUT);
      Logs::serialPrint(me, PSTR("   Setup:"));
      Logs::serialPrintln(me, String(pinId).c_str(), PSTR(":mode="), mode.c_str());
    }
    else if (strcmp_P(mode.c_str(), PSTR("OUTPUT")) == 0) {
      bool isDigital = jsonSetup[F("isDigital")].as<bool>();
      int initialValue = jsonSetup[F("initialValue")].as<int>();
      pinMode(pinId, OUTPUT);
      if (isDigital) {
        digitalWrite(pinId, initialValue);
      }
      else {
        analogWrite(pinId, initialValue);
      }
      setNextPinState(pinId, initialValue, false);
      Logs::serialPrint(me, PSTR("   Setup:"));
      Logs::serialPrintln(me, String(pinId).c_str(), PSTR(":mode="), mode.c_str());
    }
    else {
      int initialValue = jsonSetup[F("initialValue")].as<int>();
      setNextPinState(pinId, initialValue, false);
    }

    if (runCommand != nullptr && !runCommand.isEmpty()) {
      Logs::serialPrintln(me, PSTR("   Setup:"), PSTR(":command="), runCommand.c_str());
      handleCommand(currDevice, runCommand.c_str());
    }
  }

#ifdef RUN_UNIT_TESTS
  ICACHE_FLASH_ATTR DeviceEventDescription* newDeviceEventDescription(
    const char* eventName, uint8_t pinId, int startRange, int endRange,
    bool isDigital, uint16_t delay) {
    DeviceEventDescription* currEvent = new DeviceEventDescription;
    strncpy(currEvent->eventName, eventName, MAX_LENGTH_EVENT_NAME);
    currEvent->pinId = pinId;
    currEvent->startRange = startRange;
    currEvent->endRange = endRange;
    currEvent->isDigital = true;
    currEvent->delay = delay;
    return currEvent;
  }
#endif

  void ICACHE_FLASH_ATTR loadEvent(DeviceDescription* currDevice, const JsonObject& jsonEvent) {
    DeviceEventDescription* currEvent = new DeviceEventDescription;
    if (currDevice->events == nullptr) {
      currDevice->events = currEvent;
      currDevice->events->next = NULL;
    }
    else {
      currEvent->next = currDevice->events;
      currDevice->events = currEvent;
    }
    Utils::sstrncpy(
      currEvent->eventName, jsonEvent[F("eventName")].as<char*>(), MAX_LENGTH_EVENT_NAME);
    currEvent->pinId = jsonEvent[F("pinId")].as<uint8_t>();
    currEvent->startRange = jsonEvent.containsKey("startRange") ?
      jsonEvent[F("startRange")].as<int>() : 0;
    currEvent->endRange = jsonEvent.containsKey("startRange") ?
      jsonEvent[F("endRange")].as<int>() : 0;
    currEvent->raiseIfChanges = jsonEvent.containsKey("raiseIfChanges") ?
      jsonEvent[F("raiseIfChanges")].as<int>() : 0;
    currEvent->isDigital = jsonEvent[F("isDigital")].as<bool>();
    currEvent->delay = jsonEvent[F("delay")].as<uint16_t>();
    Logs::serialPrint(me, PSTR("   Event:"), String(currEvent->eventName).c_str(), PSTR(":"));
    Logs::serialPrint(me, String(currEvent->pinId).c_str(), PSTR(":startRange="),
      String(currEvent->startRange).c_str());
    Logs::serialPrint(me, PSTR(":endRange="), String(currEvent->endRange).c_str());
    Logs::serialPrint(me, PSTR(":raiseIfChanges="), String(currEvent->raiseIfChanges).c_str());
    Logs::serialPrintln(me, PSTR(":isDigital="), String(currEvent->isDigital).c_str());
    Logs::serialPrintln(me, PSTR(":delay="), String(currEvent->delay).c_str());
  }

  void ICACHE_FLASH_ATTR loadCommand(DeviceDescription* currDevice, const JsonObject& jsonCommand) {
    DeviceCommandDescription* currCommand = new DeviceCommandDescription;
    if (currDevice->commands == nullptr) {
      currDevice->commands = currCommand;
      currDevice->commands->next = NULL;
    }
    else {
      currCommand->next = currDevice->commands;
      currDevice->commands = currCommand;
    }
    Utils::sstrncpy(
      currCommand->commandName, jsonCommand[F("commandName")].as<char*>(), MAX_LENGTH_COMMAND_NAME);
    Utils::sstrncpy(currCommand->action, jsonCommand[F("action")].as<char*>(), MAX_LENGTH_ACTION);
    currCommand->pinId = jsonCommand[F("pinId")].as<uint8_t>();
    currCommand->value = jsonCommand[F("value")].as<int>();
    Utils::sstrncpy(currCommand->values, jsonCommand[F("values")].as<char*>(), MAX_LENGTH_VALUES);
    if (currCommand->values == nullptr) {
      currCommand->values[0] = '\0';
    }
    Logs::serialPrint(me, PSTR("   Command:"), String(currCommand->commandName).c_str(), PSTR(":"));
    Logs::serialPrint(
      me, String(currCommand->action).c_str(), PSTR(":pinId="), String(currCommand->pinId).c_str());
    Logs::serialPrint(me, PSTR(":value="), String(currCommand->value).c_str());
    Logs::serialPrintln(me, PSTR(":values="), String(currCommand->values).c_str());
  }

  void ICACHE_FLASH_ATTR loadTrigger(DeviceDescription* currDevice, const JsonObject& jsonTrigger) {
    DeviceTriggerDescription* currTrigger = new DeviceTriggerDescription;
    if (currDevice->triggers == nullptr) {
      currDevice->triggers = currTrigger;
      currDevice->triggers->next = NULL;
    }
    else {
      DeviceTriggerDescription* nextTrigger = currDevice->triggers;
      while (nextTrigger->next != nullptr) {
        nextTrigger = nextTrigger->next;
      }
      nextTrigger->next = currTrigger;
      currTrigger->next = NULL;
    }
    Utils::sstrncpy(
      currTrigger->onEvent, jsonTrigger[F("onEvent")].as<char*>(), MAX_LENGTH_EVENT_NAME);
    Utils::sstrncpy(
      currTrigger->fromDeviceId, jsonTrigger[F("fromDeviceId")].as<char*>(), MAX_LENGTH_DEVICE_ID + 2);
    Utils::sstrncpy(
      currTrigger->runCommand, jsonTrigger[F("runCommand")].as<char*>(), MAX_LENGTH_COMMAND_NAME);
    currTrigger->disableHardReset = false;
    if (jsonTrigger.containsKey(F("disableHardReset"))) {
      currTrigger->disableHardReset = jsonTrigger.containsKey(F("disableHardReset"));
    }
    Logs::serialPrint(me, PSTR("   Trigger:"), String(currTrigger->onEvent).c_str(), PSTR(":"));
    Logs::serialPrintln(me, String(currTrigger->fromDeviceId).c_str(), PSTR(":"),
      String(currTrigger->runCommand).c_str());
  }

  void ICACHE_FLASH_ATTR loadConfig() {
    String configFile;
    configFile += String(FPSTR("/config_"));
    configFile += chipId;
    configFile += String(FPSTR(".json"));
    if (!SPIFFS.exists(configFile)) {
      configFile = PATH_CONFIG_FILE;
      if (!SPIFFS.exists(configFile)) {
        Logs::serialPrintln(me, PSTR("[ERROR] loadConfig:Config.json NOT FOUND!!"));
        return;
      }
    }
    Logs::serialPrintln(me, PSTR("loadConfig"));
    File file = SPIFFS.open(configFile, "r");
    DynamicJsonDocument doc(1024 * 4);
    // auto doc = Utils::getJsonDoc();
    doc.clear();
    deserializeJson(doc, file);
    doc.shrinkToFit();
    file.close();

    uint8_t currIndex = 0;
    DeviceDescription* currDevice = NULL;
    JsonArray jsonDevices = doc[F("devices")].as<JsonArray>();
    for (JsonObject jsonDevice : jsonDevices) {
      String deviceId = jsonDevice[F("deviceId")].as<String>();
      if (deviceId.length() > 0 && deviceId != chipId) {
        Logs::serialPrint(me, PSTR("skip:"), String(deviceId).c_str());
        Logs::serialPrintln(me, PSTR(" != "), chipId.c_str());
        continue;
      }
      Logs::serialPrintln(me, PSTR("found:"), String(deviceId).c_str());
      currDevice = new DeviceDescription;
      if (_rootDevice == nullptr) {
        currDevice->next = NULL;
      }
      else {
        currDevice->next = _rootDevice;
      }
      _rootDevice = currDevice;
      currDevice->index = currIndex++;
      currDevice->events = NULL;
      currDevice->commands = NULL;
      currDevice->triggers = NULL;
      Utils::sstrncpy(currDevice->typeId, jsonDevice[F("typeId")].as<char*>(), MAX_LENGTH_DEVICE_TYPE_ID);
      String description = jsonDevice[F("description")].as<String>();
      Logs::serialPrintln(me, PSTR("Name: "), String(description).c_str());
      Logs::serialPrintln(me, PSTR("Index: "), String(currDevice->index).c_str());
      Logs::serialPrintln(me, PSTR("Device: "));
      Logs::serialPrintln(
        me, String(deviceId).c_str(), PSTR(":"), String(currDevice->typeId).c_str());
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
      JsonArray jsonSetups = jsonDevice[F("setup")].as<JsonArray>();
      for (JsonObject jsonSetup : jsonSetups) {
        loadSetup(currDevice, jsonSetup);
      }
    }
    doc.clear();
  }

  void ICACHE_FLASH_ATTR setup() {
    loadConfig();
    pinMode(LED_BUILTIN, OUTPUT);
  }

  void handle() {
    if (_restartAt > 0 && _restartAt < millis()) {
      ESP.restart();
    }
    detectEvents();
  }

}  // namespace Devices
