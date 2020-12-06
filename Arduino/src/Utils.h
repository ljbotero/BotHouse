#ifndef Utils_h
#define Utils_h
#include <ArduinoJson.h>

namespace Utils {

bool isHeapAvailable(size_t required);
uint32_t getMaxAvailableHeap();
bool copyStringFromJson(char *dest, JsonObject &json, String name);
String getChipIdString();
unsigned long getNormailzedTime();
void setTimeOffset(long offset);
void freeUpMemory();

}  // namespace Utils
#endif
