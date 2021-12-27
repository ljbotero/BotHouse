#ifndef Utils_h
#define Utils_h
#include <ArduinoJson.h>

namespace Utils {

// DynamicJsonDocument &getJsonDoc();
bool isHeapAvailable(size_t required);
bool copyStringFromJson(char *dest, JsonObject &json, const String &name);
unsigned long getNormailzedTime();
void setTimeOffset(long offset);
void freeUpMemory();
void sstrncpy(char *dest, const char *source, size_t length);

}  // namespace Utils
#endif
