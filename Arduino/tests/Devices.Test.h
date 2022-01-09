#include "../src/Config.h"
#ifdef RUN_UNIT_TESTS
#include "../src/Devices.h"

test(Devices_setNextPinState, HappyPath) {
  // Setup
  Devices::resetState();
  uint8_t pinId = 0; 
  Devices::DeviceEventDescription* currEvent = Devices::newDeviceEventDescription(
    String("test").c_str(), pinId, 0, 0, true, 0);

  // Run use case
  int value = 0;
  bool overrideValue = false;
  Devices::setNextPinState(pinId, value, overrideValue);
  Devices::PinState* currState = Devices::getPinState(pinId);
  assertNotEqual(currState, nullptr);
}

test(Devices_setNextPinState, WithOverrideValue) {
  // Setup
  Devices::resetState();
  uint8_t pinId = 0; 
  Devices::DeviceEventDescription* currEvent = Devices::newDeviceEventDescription(
    String("test").c_str(), pinId, 0, 0, true, 0);

  int value = 0;
  bool overrideValue = false;
  Devices::setNextPinState(pinId, value, overrideValue);
  Devices::PinState* currState = Devices::getPinState(pinId);

  // Run use case
  Devices::setNextPinState(pinId, 1, true);
  assertEqual(currState->nextValue, 1);
  assertEqual(currState->overrideValue, true);
}

test(Devices_setNextPinState, WithNoOverrideValue) {
  // Setup
  Devices::resetState();
  uint8_t pinId = 0; 
  Devices::DeviceEventDescription* currEvent = Devices::newDeviceEventDescription(
    String("test").c_str(), pinId, 0, 0, true, 0);

  int value = 0;
  bool overrideValue = false;
  Devices::setNextPinState(pinId, value, overrideValue);
  Devices::PinState* currState = Devices::getPinState(pinId);

  // Run use case
  Devices::setNextPinState(pinId, 1, false);
  assertEqual(currState->value, 0);
  assertEqual(currState->nextValue, 1);
  assertEqual(currState->overrideValue, false);
}

test(Devices_setPinState, WithNoOverrideValue) {
  // Setup
  Devices::resetState();
  uint8_t pinId = 0; 
  Devices::DeviceEventDescription* currEvent = Devices::newDeviceEventDescription(
    String("test").c_str(), pinId, 1, 1, true, 0);
  Devices::DeviceEventDescription* nextEvent = Devices::newDeviceEventDescription(
    String("test").c_str(), pinId, 0, 0, true, 0);

  int value = 0;
  bool overrideValue = false;
  Devices::setNextPinState(pinId, value, overrideValue);
  Devices::PinState* currState = Devices::getPinState(pinId);
  Devices::setPinState(currState, value, currEvent);

  Devices::setNextPinState(pinId, 1, false);
 
  // // Run use case
  int proposedValue = 1;
  bool changed = Devices::setPinState(currState, proposedValue, nextEvent);
  assertEqual(currState->value, 1);
  assertEqual(currState->nextValue, 1);
  assertEqual(currState->overrideValue, false);
  assertTrue(changed);
}

test(Devices_setPinState, WithOverrideValue) {
  // Setup
  Devices::resetState();
  uint8_t pinId = 0; 
  Devices::DeviceEventDescription* currEvent = Devices::newDeviceEventDescription(
    String("test").c_str(), pinId, 1, 1, true, 0);
  Devices::DeviceEventDescription* nextEvent = Devices::newDeviceEventDescription(
    String("test").c_str(), pinId, 0, 0, true, 0);

  int value = 0;
  bool overrideValue = false;
  Devices::setNextPinState(pinId, value, overrideValue);
  Devices::PinState* currState = Devices::getPinState(pinId);
  Devices::setPinState(currState, value, currEvent);

  Devices::setNextPinState(pinId, 1, true);
 
  // Run use case
  int proposedValue = 0;
  bool changed = Devices::setPinState(currState, proposedValue, nextEvent);
  assertEqual(currState->value, 1);
  assertEqual(currState->nextValue, 1);
  assertEqual(currState->overrideValue, true);
  assertTrue(changed);
}
#endif