#include "Utils.h"
#include "Devices.h"
#include "Logs.h"
#include "Mesh.h"

namespace Utils {
static const Logs::caller me = Logs::caller::Utils;

// DynamicJsonDocument doc(1024 * 4);
// DynamicJsonDocument &getJsonDoc() {
//   return doc;
// }

bool isHeapAvailable(size_t required) {
  size_t available = ESP.getFreeHeap();
  if (available < required) {
    Logs::serialPrintln(
        me, PSTR("[WARNING] Failed allocating requested heap of "), String(required).c_str());
    Logs::serialPrintln(me, PSTR("Current free heap: "), String(available).c_str());
    // return false;
    return true;
  }
  return true;
}

bool copyStringFromJson(char *dest, JsonObject &json, const String &name) {
  bool changed = false;
  const char *value = json[name].as<const char *>();
  if (value) {
    changed = strcmp(dest, value) != 0;
    strcpy(dest, value);
  } else {
    Logs::serialPrintln(me, PSTR("[ERROR]  Empty Json value for "), name.c_str());
  }
  return changed;
}

long _timeOffset = 0;
void setTimeOffset(long offset) {
  if (abs(_timeOffset) > 0 && abs(_timeOffset) < abs(offset)) {
    return;
  }
  Logs::serialPrint(me, PSTR("Adjusted this node's time by "));
  Logs::serialPrintln(me, String(offset).c_str(), PSTR("ms"));
  _timeOffset = offset;
}

unsigned long getNormailzedTime() {
  return millis() + _timeOffset;
}

void freeUpMemory() {
  size_t before = ESP.getFreeHeap();
  Mesh::purgeDevicesFromNodeList(0);
  size_t after = ESP.getFreeHeap();
  Logs::serialPrintln(
      me, PSTR("Freed "), String(before - after).c_str(), PSTR(" bytes of memory "));
}

void sstrncpy(char *dest, const char *source, size_t length) {
  if (dest == nullptr) {
    return;
  }
  if (source != nullptr) {
    strncpy_P(dest, (PGM_P)source, length); 
  } else {
    dest[0] = '\0';
  }
}

}  // namespace Utils