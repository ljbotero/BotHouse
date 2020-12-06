#ifndef AccessPoints_h
#define AccessPoints_h

#include "Config.h"
#include "Storage.h"

namespace AccessPoints {

struct AccessPointInfo {
  String SSID = "";
  int32_t RSSI;
  uint8_t *BSSID = NULL;
  int wifiChannel = -1;
  int networkIndex = -1;
  int32_t apLevel = -1;
  bool isRecognized;
  bool isOpen;
};

struct AccessPointList {
  AccessPointInfo *ap;
  AccessPointList *next;
};

struct AccessPointConnectionInfo {
  String SSID = "";
  uint32_t lastFailedConnection = 0;
  uint8_t connectionAttempts = 0;
  AccessPointConnectionInfo *next;
};

AccessPointList *getAccessPointsList();
void setAccessPointsList(AccessPointList *accessPointsList);
AccessPointInfo *getAccessPointHomeWifi();
AccessPointInfo *getStrongestAccessPoint(AccessPointInfo *excludeAP = NULL,
    int32_t RSSIGreatherThan = 0, int32_t apLevelLowerThan = 0,
    uint8_t connectionAttemptsLessThan = 0, uint32_t notFailedConnectingSince = 0);
AccessPointInfo *getAccessPointAtLevel(int32_t apLevel, AccessPointList *accessPointsList);
AccessPointInfo *getAccessPointWithHighestLevel(AccessPointList *accessPointsList);
AccessPointConnectionInfo *getAccessPointInfo(const String &ssid);
void deleteNodeAccessPoints(AccessPointList *accessPoints);
void scanNetworksAnalysis(Storage::storageStruct &flashData, int numberOfNetworks);
void showWifiScanInfo(bool showHeader, bool isRecognized);

#ifdef RUN_UNIT_TESTS
#endif
}  // namespace AccessPoints
#endif
