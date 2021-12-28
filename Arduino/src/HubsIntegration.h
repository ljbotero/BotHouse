#ifndef Hub_h
#define Hub_h
#include <IPAddress.h>
#include "Devices.h"

namespace HubsIntegration {

void setup();
void handle();
bool postDeviceEvent(const Devices::DeviceState &state, const String &type = FPSTR("deviceEvent"));
bool sendHeartbeat(const char *deviceId);

}  // namespace HubsIntegration
#endif
