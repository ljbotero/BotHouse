#include "Utils.h"
#include <ArduinoJson.h>
#include <TypeConversionFunctions.h>
#include "Devices.h"
#include "Logs.h"
#include "Mesh.h"

namespace Utils {
const Logs::caller me = Logs::caller::Utils;

bool isHeapAvailable(size_t required) {
  size_t available = ESP.getFreeHeap();
  if (available < required) {
    Logs::serialPrintln(me, F("WARNING: Failed allocating requested heap of "), String(required));
    Logs::serialPrintln(me, F("Current free heap: "), String(available));
    // return false;
    return true;
  }
  return true;
}

uint32_t getMaxAvailableHeap() {
#ifdef ARDUINO_ARCH_ESP32
  return ESP.getMaxAllocHeap();
#elif defined(ARDUINO_ARCH_ESP8266)
  return ESP.getMaxFreeBlockSize();
#else
  return ESP.getMaxFreeBlockSize();
#endif
}

bool copyStringFromJson(char *dest, JsonObject &json, String name) {
  bool changed = false;
  const char *value = json[name].as<const char *>();
  if (value) {
    changed = strcmp(dest, value) != 0;
    strcpy(dest, value);
  } else {
    Logs::serialPrintln(me, F("ERROR: Empty Json value for "), name);
  }
  return changed;
}

long _timeOffset = 0;
void setTimeOffset(long offset) {
  if (abs(_timeOffset) > 0 && abs(_timeOffset) < abs(offset)) {
    return;
  }
  Logs::serialPrint(me, F("Adjusted this node's time by "));
  Logs::serialPrintln(me, String(offset) + "ms");
  _timeOffset = offset;
}

unsigned long getNormailzedTime() {
  return millis() + _timeOffset;
}

uint16_t GetDeviceId() {
#if defined(ARDUINO_ARCH_ESP32)
  return ESP.getEfuseMac();
#else
  return ESP.getChipId();
#endif
}

String getChipIdString() {
  return uint64ToString(GetDeviceId());
}

void freeUpMemory() {
  size_t before = ESP.getFreeHeap();
  Logs::deleteHistoryQueue();    
  Mesh::purgeDevicesFromNodeList(0);
  size_t after = ESP.getFreeHeap();
  Logs::serialPrintln(me, F("Freed "), String(before - after), F(" bytes of memory "));
}

}  // namespace Utils