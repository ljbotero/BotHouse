#ifndef MessageGenerator_h
#define MessageGenerator_h
#include <ESP8266WiFi.h>
#include "Devices.h"
#include "Mesh.h"
#include "MessageProcessor.h"
#include "Network.h"

namespace MessageGenerator {

// String generateMeshInfo();
void generateDeviceInfo(String &outputJson, const Mesh::Node &deviceInfo, const String &action);
// void generateLogMessage(String &outputJson, const String &message, bool append);
void generateSharedInfo(String &outputJson, bool hideConfidentialData = false);
void generateDeviceEvent(String &outputJson, const Devices::DeviceState &state);
void generateRawAction(String &outputJson, const String &action, const String &deviceId = "",
    const String &deviceIndex = "", const String &data = "");
// void generateChunkedLogsHistory(long newerThanTimestamp, void (&sendContent)(const String
// &content));
void generateChunkedMeshReport(void (&sendContent)(const String &content));
}  // namespace MessageGenerator
#endif