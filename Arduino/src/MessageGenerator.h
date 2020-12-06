#ifndef MessageGenerator_h
#define MessageGenerator_h
#include <ESP8266WiFi.h>
#include "Devices.h"
#include "Mesh.h"
#include "MessageProcessor.h"
#include "Network.h"

namespace MessageGenerator {

// String generateMeshInfo();
String generateDeviceInfo(Mesh::Node deviceInfo, const String &action);
String generateLogMessage(const String &message, bool append);
String generateSharedInfo(bool hideConfidentialData = false);
String generateDeviceEvent(Devices::DeviceState state);
String generateAlexaDeviceEvent(Devices::DeviceState state);
String generateRawAction(String action, String deviceId = "", String deviceIndex = "", String data = "");
void generateChunkedLogsHistory(long newerThanTimestamp, void (&sendContent)(const String &content));
void generateChunkedMeshReport(void (&sendContent)(const String &content));
}  // namespace MessageGenerator
#endif