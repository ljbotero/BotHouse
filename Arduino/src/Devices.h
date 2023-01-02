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
  int raiseIfChanges;
  char source[MAX_LENGHT_SOURCE];
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
  bool enableHardReset;
  DeviceTriggerDescription* next;
};

struct DeviceDescription {
  uint8_t index;
  char typeId[MAX_LENGTH_DEVICE_TYPE_ID];  // "push-button", "contact", "on-off-switch", 
                                           //"switch-relay", "flow-rate", "motion-sensor", "temp-sensor"
  char lastEventName[MAX_LENGTH_EVENT_NAME] = "\0";
  int lastEventValue;
  DeviceEventDescription* events;
  DeviceCommandDescription* commands;
  DeviceTriggerDescription* triggers;
  DeviceDescription* next;
};

struct PinState {
  uint8_t pinId;
  unsigned long nextAllowedChange = 0;
  unsigned long nextBroadcast = 0;
  uint32_t broadcastCount = 0;
  char source[MAX_LENGHT_SOURCE];
  int lastValue;
  int nextValue;
  int value;
  bool overrideValue;
  uint8_t consecutiveChangesCount;
  uint32_t nextConsecutiveChangeBefore;
  PinState* next;
};

struct DeviceState {
  char deviceId[MAX_LENGTH_UUID];
  uint8_t deviceIndex;
  char deviceTypeId[MAX_LENGTH_DEVICE_TYPE_ID];
  char eventName[MAX_LENGTH_EVENT_NAME];
  int eventValue;
};

void setup();
void handle();
void restart();
DeviceDescription* getRootDevice();
DeviceDescription* getDeviceFromIndex(DeviceDescription* root, uint8_t index);
void deleteDevices(DeviceDescription* root);
DeviceDescription* deserializeDevices(const JsonArray devicesArray);
void serializeDevices(JsonObject container, DeviceDescription* devicesObject);
void updateDevice(const char *deviceName);
void processEventsFromOtherDevices(const Devices::DeviceState& state);
char *getDeviceName(Storage::storageStruct* flashData = NULL);
bool handleCommand(DeviceDescription* currDevice, const char *commandName, bool overrideValue);

#ifdef RUN_UNIT_TESTS
void resetState();
PinState* getPinState(uint8_t pinId);
void setNextPinState(uint8_t pinId, int value, bool overrideValue);
bool setPinState(PinState* currState, int value, DeviceEventDescription* currEvent);
PinState* createNewPinState(uint8_t pinId, int value, bool overrideValue, unsigned long delay);
DeviceEventDescription* newDeviceEventDescription(
  const char* eventName, uint8_t pinId, int startRange, int endRange, 
  bool isDigital, uint16_t delay);
#endif
}  // namespace Devices
#endif
