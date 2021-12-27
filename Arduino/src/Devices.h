#ifndef devices_h
#define devices_h

#include <Arduino.h>
#include "Config.h"
#include "Storage.h"

#define MAX_DEVICE_EVENTS 2

namespace Devices {

struct DeviceEventDescription {
  char eventName[MAX_LENGTH_EVENT_NAME];
  uint8_t pinId;
  int startRange;
  int endRange;
  bool isDigital;
  uint16_t delay;
  DeviceEventDescription* next;
};
struct DeviceCommandDescription {
  char commandName[MAX_LENGTH_COMMAND_NAME];
  char action[MAX_LENGTH_ACTION];
  uint8_t pinId;
  int value;
  char values[MAX_LENGTH_VALUES];
  DeviceCommandDescription* next;
};

struct DeviceTriggerDescription {
  char onEvent[MAX_LENGTH_EVENT_NAME];
  char fromDeviceId[MAX_LENGTH_DEVICE_ID];
  char runCommand[MAX_LENGTH_COMMAND_NAME];
  bool disableHardReset;
  DeviceTriggerDescription* next;
};

struct DeviceDescription {
  uint8_t index;
  char typeId[MAX_LENGTH_DEVICE_TYPE_ID];  // "push-button", "contact", "on-off-switch", "switch-relay"
  char lastEventName[MAX_LENGTH_EVENT_NAME];
  DeviceEventDescription* events;
  DeviceCommandDescription* commands;
  DeviceTriggerDescription* triggers;
  DeviceDescription* next;
};

struct PinState {
  uint8_t pinId;
  uint32_t nextAllowedChange;
  uint32_t nextBroadcast;
  uint32_t broadcastCount;
  int lastReadValue;
  int nextValue;
  int value;
  uint8_t consecutiveChangesCount;
  uint32_t nextConsecutiveChangeBefore;
  PinState* next;
};

struct DeviceState {
  char deviceId[MAX_LENGTH_UUID];
  uint8_t deviceIndex;
  char deviceTypeId[MAX_LENGTH_DEVICE_TYPE_ID];
  char eventName[MAX_LENGTH_EVENT_NAME];
  char eventValue[MAX_LENGTH_EVENT_NAME];;
};

void setup();
void handle();
void restart();
DeviceDescription* getRootDevice();
DeviceDescription* getDeviceFromIndex(DeviceDescription* root, uint8_t index);
void deleteDevices(DeviceDescription* root);
DeviceDescription* deserializeDevices(const JsonArray& devicesArray);
void serializeDevices(JsonObject& container, DeviceDescription* devicesObject);
void updateDevice(const char *deviceName);
void processEventsFromOtherDevices(const Devices::DeviceState& state);
char *getDeviceName(Storage::storageStruct* flashData = NULL);
bool handleCommand(DeviceDescription* currDevice, const char *commandName);
bool setDeviceSate(uint8_t deviceIndex, int state);

}  // namespace Devices
#endif
