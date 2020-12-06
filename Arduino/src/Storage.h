#ifndef Storage_h
#define Storage_h

#include <Arduino.h>
#include "Config.h"

namespace Storage {

typedef struct storageStruct {
  int state = 0;
  uint32_t version = 0;
  char meshName[MAX_LENGTH_WIFI_NAME] = "\0";
  char meshPassword[MAX_LENGTH_WIFI_PASSWORD] = "\0";
  char wifiName[MAX_LENGTH_WIFI_NAME] = "\0";
  char wifiPassword[MAX_LENGTH_WIFI_PASSWORD] = "\0";
  char hubApi[MAX_LENGTH_HUB_API] = "\0";
  char hubToken[MAX_LENGTH_HUB_TOKEN] = "\0";
  char hubNamespace[MAX_LENGTH_HUB_NAMESPACE] = "\0";
  char deviceName[MAX_LENGTH_DEVICE_NAME] = "\0";
  char amazonUserId[MAX_LENGTH_AMAZON_USER_ID] = "\0";
  char amazonEmail[MAX_LENGTH_AMAZON_EMAIL]  = "\0";
};

void setup();
storageStruct readFlash();
void writeFlash(storageStruct &data, bool updateVersion = true);
}  // namespace Storage
#endif
