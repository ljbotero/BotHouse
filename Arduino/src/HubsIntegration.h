#ifndef Hub_h
#define Hub_h
#include <IPAddress.h>
#include "Devices.h"

namespace HubsIntegration {

void setup();
void handle();
bool postDeviceEvent(const Devices::DeviceState &state);
bool sendHeartbeat(const char *deviceId);

}  // namespace HubsIntegration
#endif
