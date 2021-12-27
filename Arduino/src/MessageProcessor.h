#ifndef MessageProcessor_h
#define MessageProcessor_h
#include <ESP8266WiFi.h>
#include "Devices.h"

namespace MessageProcessor {

void handle();
bool processMessage(const char *message, const IPAddress &sender, const uint16_t senderPort = 80,
    bool propagateMessage = true);

}  // namespace MessageProcessor
#endif