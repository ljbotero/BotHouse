#ifndef Utils_h
#define Utils_h
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>

namespace Utils {

  // DynamicJsonDocument &getJsonDoc();
  String EMPTY_STR();
  bool isHeapAvailable(size_t required);
  bool copyStringFromJson(char* dest, JsonObject& json, const String& name);
  unsigned long getNormailzedTime();
  void setTimeOffset(long offset);
  void freeUpMemory();
  void sstrncpy(char* dest, const char* source, size_t length);
  String getBSSIDStr(const uint8_t* bssid);
  bool areBSSIDEqual(const uint8_t* bssid1, const uint8_t* bssid2);
  const char* getWifiStatusText(int8_t status);

}  // namespace Utils
#endif
