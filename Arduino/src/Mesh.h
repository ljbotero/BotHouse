#ifndef Mesh_h
#define Mesh_h

#include "AccessPoints.h"
#include "Config.h"
#include "Devices.h"
#include "Storage.h"

namespace Mesh {

struct Node {
  String deviceId;
  String deviceName;
  String wifiSSID;
  int32_t wifiRSSI;
  bool isMaster;
  String IPAddress;
  String apSSID;
  int32_t apLevel;
  uint32_t freeHeap;
  unsigned long lastUpdate;
  Devices::DeviceDescription *devices = NULL;
  AccessPoints::AccessPointList *accessPoints = NULL;
  Node *next = NULL;
};

bool isAccessPointAPotentialNode(String &ssid);
Node *getNodesTip();
void setNodesTip(Node *tip);
Node getNodeInfo();
String getMySSID(Storage::storageStruct &flashData);
String getSaltedMeshName(Storage::storageStruct &flashData);
char *getMeshName(Storage::storageStruct &flashData);
void scanNetworksComplete(int numberOfNetworks);
bool updateOrAddNodeInfoList(Node &nodeInfo);
void removeDeviceFromNodeList(const String &deviceId);
bool updateExistingNodeInfo(Node nodeInfo);
bool isConnectedToWifi();
void resetMasterWifiNode();
bool isAccessPointNode();
void setAccessPointNode(bool isAccessPointNode);
void showNodeInfo();
bool isMasterNode();
void purgeDevicesFromNodeList(uint16_t ifNotSeenForNumberOfScans = 20);

#ifdef RUN_UNIT_TESTS
int32_t calculateAccessPointLevel(AccessPoints::AccessPointList *accessPointList,
    AccessPoints::AccessPointInfo *closestRecognizedAccessPoint,
    AccessPoints::AccessPointInfo *accessPointHomeWifi, uint32_t freeHeap);

#endif
}  // namespace Mesh
#endif
