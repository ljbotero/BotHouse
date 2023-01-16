#include "Storage.h"
#include <EEPROM.h>
#include "Devices.h"
#include "Events.h"
#include "Logs.h"

#define STATE_INITIALIZED 128

namespace Storage {
  static const Logs::caller me = Logs::caller::Storage;
  // Logs::caller me = Logs::caller.

  void writeFlash(storageStruct& data, bool updateVersion) {
    storageStruct oldData = readFlash();
    if (memcmp(&oldData, &data, sizeof(storageStruct)) == 0) {
        Logs::serialPrintln(me, PSTR("writeFlash: no changes found"));
      return;
    }
    Logs::serialPrintln(me, PSTR("writeFlash: "), String(sizeof(data)).c_str(), PSTR(" bytes"));
    if (updateVersion) {
      data.version++;
    }
    EEPROM.put(0, data);
    EEPROM.commit();
  }

  storageStruct readFlash() {
    // Logs::serialPrintln(me, PSTR("readFlash"));
    storageStruct data;
    EEPROM.begin(sizeof(data));  // Loads the content of flash into a byte-array cache in RAM
    EEPROM.get(0, data);
    // EEPROM.end();
    return data;
  }

  void ICACHE_FLASH_ATTR setup() {
    storageStruct data = readFlash();
    if (data.state != STATE_INITIALIZED) {
      storageStruct emptyData;
      emptyData.state = STATE_INITIALIZED;
      writeFlash(emptyData, true);
      Logs::serialPrintln(me, PSTR("EEPROM Initialized"));
    }
  }
}  // namespace Storage