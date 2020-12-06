#include "Storage.h"
#include <EEPROM.h>
#include "Devices.h"
#include "Events.h"
#include "Logs.h"

#define STATE_INITIALIZED 128

namespace Storage {
const Logs::caller me = Logs::caller::Storage;
// Logs::caller me = Logs::caller.

void writeFlash(storageStruct &data, bool updateVersion) {
  storageStruct oldData = readFlash();
  if (strcmp(oldData.hubNamespace, data.hubNamespace) == 0 &&
      strcmp(oldData.hubApi, data.hubApi) == 0 && strcmp(oldData.hubToken, data.hubToken) == 0 &&
      oldData.state == data.state && oldData.version == data.version &&
      strcmp(oldData.wifiName, data.wifiName) == 0 &&
      strcmp(oldData.wifiPassword, data.wifiPassword) == 0 &&
      strcmp(oldData.meshName, data.meshName) == 0 &&
      strcmp(oldData.meshPassword, data.meshPassword) == 0 &&
      strcmp(oldData.deviceName, data.deviceName) == 0 &&
      strcmp(oldData.amazonUserId, data.amazonUserId) == 0 &&
      strcmp(oldData.amazonEmail, data.amazonEmail) == 0) {
    Logs::serialPrintln(me, F("writeFlash: no changes found"));
    return;
  }
  Logs::serialPrintln(me, F("writeFlash: "), String(sizeof(data)), F(" bytes"));
  if (updateVersion) {
    data.version++;
  }
  EEPROM.put(0, data);
  EEPROM.commit();
}

storageStruct readFlash() {
  // Logs::serialPrintln(me, F("readFlash"));
  storageStruct data;
  EEPROM.begin(sizeof(data));  // Loads the content of flash into a byte-array cache in RAM
  EEPROM.get(0, data);
  // EEPROM.end();
  return data;
}

void setup() {
  storageStruct data = readFlash();
  if (data.state != STATE_INITIALIZED) {
    storageStruct emptyData;
    emptyData.state = STATE_INITIALIZED;
    writeFlash(emptyData, true);
    Logs::serialPrintln(me, F("EEPROM Initialized"));
  }
}
}  // namespace Storage