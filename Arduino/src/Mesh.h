#ifndef Mesh_h
#define Mesh_h

#include "AccessPoints.h"
#include "Config.h"
#include "Devices.h"
#include "Storage.h"

namespace Mesh {

struct Node {
  char deviceId[MAX_LENGTH_DEVICE_ID];
  char deviceName[MAX_LENGTH_DEVICE_NAME];
  char macAddress[MAX_LENGTH_MAC];
  char wifiSSID[MAX_LENGTH_SSID];
  int32_t wifiRSSI;
  bool isMaster;
  char IPAddress[MAX_LENGTH_IP];
  char apSSID[MAX_LENGTH_SSID];
  int32_t apLevel;
  uint32_t freeHeap;
  unsigned long lastUpdate;
  unsigned long systemTime;
  Devices::DeviceDescription *devices = NULL;
  AccessPoints::AccessPointList *accessPoints = NULL;
  Node *next = NULL;
};

bool isAccessPointAPotentialNode(const char *SSID);
Node *getNodesTip();
void setNodesTip(Node *tip);
Node getNodeInfo();
void setAPLevel(int32_t apLevel);
void getMySSID(String &out, Storage::storageStruct &flashData);
void getSaltedMeshName(String &out, Storage::storageStruct &flashData);
void scanNetworksComplete(int numberOfNetworks);
bool updateOrAddNodeInfoList(Node &nodeInfo);
void removeDeviceFromNodeList(const char *deviceId);
bool updateExistingNodeInfo(Node nodeInfo);
bool isConnectedToWifi();
void resetMasterWifiNode();
bool isAccessPointNode();
void setAccessPointNode(bool isAccessPointNode);
void showNodeInfo();
bool isMasterNode();
void purgeDevicesFromNodeList(uint16_t ifNotSeenForNumberOfScans = 6);

#ifdef RUN_UNIT_TESTS
int32_t calculateAccessPointLevel(AccessPoints::AccessPointList *accessPointList,
    AccessPoints::AccessPointInfo *closestRecognizedAccessPoint,
    AccessPoints::AccessPointInfo *accessPointHomeWifi, uint32_t freeHeap);

#endif
}  // namespace Mesh
#endif
