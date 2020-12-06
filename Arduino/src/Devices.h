#ifndef devices_h
#define devices_h

#include <Arduino.h>
#include <ArduinoJson.h>
#include "Config.h"
#include "Storage.h"

#define MAX_DEVICE_EVENTS 2

namespace Devices {

struct DeviceEventDescription {
  String eventName;
  uint8_t pinId;
  int startRange;
  int endRange;
  bool isDigital;
  uint16_t delay;
  DeviceEventDescription* next;
};
struct DeviceCommandDescription {
  String commandName;
  String action;
  uint8_t pinId;
  int value;
  String values;
  DeviceCommandDescription* next;
};

struct DeviceTriggerDescription {
  String onEvent;
  String fromDeviceId;
  String runCommand;
  DeviceTriggerDescription* next;
};

struct DeviceDescription {
  uint8_t index;
  String typeId;
  String lastEventName;
  DeviceEventDescription* events;
  DeviceCommandDescription* commands;
  DeviceTriggerDescription* triggers;
  DeviceDescription* next;
};

struct PinState {
  uint8_t pinId;
  uint32_t nextAllowedChange;
  int lastReadValue;
  int nextValue;
  int value;
  uint8_t consecutiveChangesCount;
  uint32_t nextConsecutiveChangeBefore;
  PinState* next;
};

struct DeviceState {
  String deviceId;
  uint8_t deviceIndex;
  String deviceTypeId;
  String eventName;
  String eventValue;
};

void setup();
void handle();
void restart();
DeviceDescription* getRootDevice();
DeviceDescription* getDeviceFromIndex(DeviceDescription* root, uint8_t index);
void deleteDevices(DeviceDescription* root);
DeviceDescription* deserializeDevices(JsonArray& devicesArray);
void serializeDevices(JsonObject& container, DeviceDescription* devicesObject);
void updateDevice(const String& deviceName);
String getDeviceName(Storage::storageStruct* flashData = NULL);
bool handleCommand(DeviceDescription* currDevice, String commandName);
bool setDeviceSate(uint8_t deviceIndex, int state);

}  // namespace Devices
#endif
