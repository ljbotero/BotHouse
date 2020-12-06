#ifndef MessageProcessor_h
#define MessageProcessor_h
#include <ESP8266WiFi.h>
#include "Devices.h"

namespace MessageProcessor {

bool processMessage(
    const char *message, const IPAddress &sender = NULL, const uint16_t senderPort = 80);

}  // namespace MessageProcessor
#endif