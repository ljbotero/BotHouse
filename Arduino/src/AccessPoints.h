#ifndef AccessPoints_h
#define AccessPoints_h

#include "Config.h"
#include "Storage.h"

namespace AccessPoints {

struct AccessPointInfo {
  char SSID[MAX_LENGTH_SSID] =  "\0";
  int32_t RSSI;
  uint8_t BSSID[6];
  int32_t wifiChannel = -1;
  int networkIndex = -1;
  int32_t apLevel = -1;
  bool isRecognized;
  bool isHomeWifi;
  bool isOpen;
};

struct AccessPointList {
  AccessPointInfo *ap;
  AccessPointList *next;
};

struct AccessPointConnectionInfo {
  uint8_t BSSID[6];
  unsigned long lastFailedConnection = 0;
  uint8_t connectionAttempts = 0;
  AccessPointConnectionInfo *next;
};

AccessPointList *getAccessPointsList();
AccessPointInfo *getStrongestAccessPoint(AccessPointInfo *excludeAP = NULL,
    int32_t RSSIGreatherThan = 0, int32_t apLevelLowerThan = 0,
    uint8_t connectionAttemptsLessThan = 0, uint32_t notFailedConnectingSince = 0);
AccessPointInfo *getAccessPointAtLevel(int32_t apLevel, AccessPointList *accessPointsList);
AccessPointInfo *getAccessPointWithHighestLevel(AccessPointList *accessPointsList);
AccessPointConnectionInfo *getAccessPointInfo(const uint8_t *BSSID);
void deleteNodeAccessPoints(AccessPointList *accessPoints);
void scanNetworksAnalysis(Storage::storageStruct &flashData, int numberOfNetworks);
void showWifiScanInfo(bool showHeader, bool isRecognized);
bool isAccessPointInRange(const uint8_t *BSSID);
void setAccessPointsList(AccessPointList *accessPointsList);

#ifdef RUN_UNIT_TESTS
#endif
}  // namespace AccessPoints
#endif
