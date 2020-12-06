#include "AccessPoints.h"
#include <ESP8266WiFi.h>
#include "Config.h"
#include "Logs.h"
#include "Mesh.h"
#include "Storage.h"
#include "Utils.h"

namespace AccessPoints {

const Logs::caller me = Logs::caller::AccessPoints;
AccessPointList *_accessPointsList = NULL;
AccessPointInfo *_accessPointHomeWifi = NULL;

AccessPointList *getAccessPointsList() {
  return _accessPointsList;
}

void setAccessPointsList(AccessPointList *accessPointsList) {
  _accessPointsList = accessPointsList;
}

AccessPointInfo *getAccessPointHomeWifi() {
  return _accessPointHomeWifi;
}

void setAccessPointHomeWifi(AccessPointInfo *accessPointHomeWifi) {
  _accessPointHomeWifi = accessPointHomeWifi;
}

AccessPointConnectionInfo *_rootInfo = NULL;

AccessPointConnectionInfo *getAccessPointInfo(const String &ssid) {
  if (ssid.isEmpty()) {
    return NULL;
  }
  AccessPointConnectionInfo *currInfo = _rootInfo;
  while (currInfo != nullptr && !ssid.equalsIgnoreCase(currInfo->SSID)) {
    currInfo = currInfo->next;
  }
  if (currInfo == nullptr) {
    Logs::serialPrintln(me, F("getAccessPointInfo:new:AccessPointConnectionInfo:'"), ssid, F("'"));
    currInfo = new AccessPointConnectionInfo;
    currInfo->SSID = ssid;
    currInfo->connectionAttempts = 0;
    currInfo->lastFailedConnection = 0;
    currInfo->next = _rootInfo;
    _rootInfo = currInfo;
  }
  return currInfo;
}

void setAccessPointInfo(Storage::storageStruct &flashData, AccessPointInfo *info, int networkIndex,
    bool isRecognized, const String &saltedMeshName) {
  info->SSID = WiFi.SSID(networkIndex);
  info->apLevel = 0;
  if (info->SSID.startsWith(saltedMeshName)) {
    String appLevelStr = info->SSID.substring(saltedMeshName.length());
    if (!appLevelStr.isEmpty()) {
      info->apLevel = appLevelStr.toInt();
    }
  }
  info->wifiChannel = WiFi.channel(networkIndex);
  info->networkIndex = networkIndex;
  info->RSSI = WiFi.RSSI(networkIndex);
  info->isRecognized = isRecognized;
  info->isOpen = WiFi.encryptionType(networkIndex) == wl_enc_type::ENC_TYPE_NONE;
  uint8_t *newBSSID = WiFi.BSSID(networkIndex);
  if (newBSSID != nullptr) {
    if (info->BSSID == nullptr) {
      uint8_t _bssidArray[6]{0};
      info->BSSID = _bssidArray;
    }
    for (int i = 0; i < 6; i++) {
      info->BSSID[i] = newBSSID[i];
    }
  } else {
    info->BSSID = NULL;
  }
}

AccessPointInfo *getStrongestAccessPoint(AccessPointInfo *excludeAP, int32_t RSSIGreatherThan,
    int32_t apLevelLowerThan, uint8_t connectionAttemptsLessThan,
    uint32_t notFailedConnectingSince) {
  AccessPointInfo *strongestAP = NULL;
  AccessPointList *currNode = getAccessPointsList();
  // Logs::serialPrint(me, F("getStrongestAccessPoint: "));
  // Logs::serialPrint(me, F("excludeAP="), excludeAP == nullptr ? "" : excludeAP->SSID);
  // Logs::serialPrint(me, F(", RSSIGreatherThan="), String(RSSIGreatherThan));
  // Logs::serialPrint(me, F(", apLevelLowerThan="), String(apLevelLowerThan));
  // Logs::serialPrint(me, F(", connectionAttemptsLessThan="), String(connectionAttemptsLessThan));
  // Logs::serialPrintln(me, F(", notFailedConnectingSince="), String(notFailedConnectingSince));

  while (currNode != nullptr) {
    if (!currNode->ap->isRecognized || excludeAP == currNode->ap) {
      currNode = currNode->next;
      continue;
    }
    bool hasStrongerSignal = (strongestAP == nullptr || currNode->ap->RSSI > strongestAP->RSSI) &&
                             (RSSIGreatherThan == 0 || currNode->ap->RSSI > RSSIGreatherThan);
    bool isHigherLevel = (apLevelLowerThan == 0 ||
                          (apLevelLowerThan > 0 && currNode->ap->apLevel < apLevelLowerThan) ||
                          (apLevelLowerThan < 0 && currNode->ap->apLevel > apLevelLowerThan));
    AccessPointConnectionInfo *wifiInfo = getAccessPointInfo(currNode->ap->SSID);
    bool notFailedRecently = true;
    if (wifiInfo != nullptr) {
      notFailedRecently = notFailedConnectingSince == 0 || connectionAttemptsLessThan == 0 ||
                          wifiInfo->lastFailedConnection <= notFailedConnectingSince ||
                          wifiInfo->connectionAttempts < connectionAttemptsLessThan;
    }
    if (hasStrongerSignal && isHigherLevel && notFailedRecently) {
      strongestAP = currNode->ap;
    }
    currNode = currNode->next;
  }
  if (strongestAP != nullptr) {
    Logs::serialPrint(me, F("getStrongestAccessPoint = "), strongestAP->SSID);
    Logs::serialPrintln(me, F(" "), String(strongestAP->RSSI), F("db"));
  }
  return strongestAP;
}

AccessPointInfo *getAccessPointAtLevel(int32_t apLevel, AccessPointList *accessPointsList) {
  AccessPointList *currNode = accessPointsList;
  while (currNode != nullptr) {
    if (currNode->ap->apLevel == apLevel) {
      return currNode->ap;
    }
    currNode = currNode->next;
  }
  return NULL;
}

AccessPointInfo *getAccessPointWithHighestLevel(AccessPointList *accessPointsList) {
  AccessPointInfo *accessPointWithHighestLevel = NULL;
  AccessPointList *currNode = accessPointsList;
  while (currNode != nullptr) {
    if (accessPointWithHighestLevel == nullptr ||
        (currNode->ap->apLevel > 0 &&
            currNode->ap->apLevel > accessPointWithHighestLevel->apLevel) ||
        (currNode->ap->apLevel < 0 &&
            currNode->ap->apLevel < accessPointWithHighestLevel->apLevel)) {
      accessPointWithHighestLevel = currNode->ap;
    }
    currNode = currNode->next;
  }
  return accessPointWithHighestLevel;
}

void deleteAccessPoint(AccessPointInfo *ap) {
  if (ap == nullptr) {
    return;
  }
  AccessPointList *accessPointsList = getAccessPointsList();
  if (ap == getAccessPointHomeWifi()) {
    setAccessPointHomeWifi(NULL);
  }
  if (accessPointsList != nullptr && ap == accessPointsList->ap) {
    setAccessPointsList(accessPointsList->next);
  }
  delete ap;
}

void deleteNodeAccessPoints(AccessPointList *accessPoints) {
  AccessPointList *toDelete = accessPoints;
  while (toDelete != nullptr) {
    AccessPointList *next = toDelete->next;
    deleteAccessPoint(toDelete->ap);
    delete toDelete;
    toDelete = next;
  }
}

void insertIntoAccessPointsList(AccessPointInfo *ap) {
  if (ap == nullptr) {
    return;
  }
  if (getAccessPointsList() == nullptr) {
    AccessPointList *accessPointList = new AccessPointList;
    accessPointList->next = NULL;
    accessPointList->ap = ap;
    setAccessPointsList(accessPointList);
    return;
  }
  // Check if SSID has previously been scanned
  AccessPointList *currNode = getAccessPointsList();
  while (currNode != nullptr) {
    if (currNode->ap->SSID == ap->SSID) {
      AccessPointInfo *toDelete = currNode->ap;
      currNode->ap = ap;
      deleteAccessPoint(toDelete);
      return;
    }
    currNode = currNode->next;
  }
  // Add new node
  AccessPointList *newNode = new AccessPointList;
  newNode->ap = ap;
  newNode->next = getAccessPointsList();
  setAccessPointsList(newNode);
}

void scanNetworksAnalysis(Storage::storageStruct &flashData, int numberOfNetworks) {
  // Find router Wifi first
  String saltedMeshName = Mesh::getSaltedMeshName(flashData);
  for (int networkIndex = 0; networkIndex < numberOfNetworks; networkIndex++) {
    String currentSSID = WiFi.SSID(networkIndex);
    if (currentSSID.isEmpty()) {
      Logs::serialPrintln(me, F("No name found for SSID - Skipping"));
      continue;
    }
    if (currentSSID.equalsIgnoreCase(flashData.wifiName)) {
      AccessPointInfo *accessPointHomeWifi = new AccessPointInfo;
      setAccessPointInfo(flashData, accessPointHomeWifi, networkIndex, true, saltedMeshName);
      //############################## TEST ONLY ###########################################
      // accessPointHomeWifi->RSSI = -85;
      insertIntoAccessPointsList(accessPointHomeWifi);
      setAccessPointHomeWifi(accessPointHomeWifi);
    } else if (currentSSID.startsWith(saltedMeshName)) {
      AccessPointInfo *APNode = new AccessPointInfo;
      setAccessPointInfo(flashData, APNode, networkIndex, true, saltedMeshName);
      insertIntoAccessPointsList(APNode);
    } else {
      AccessPointInfo *wifiAp = new AccessPointInfo;
      setAccessPointInfo(flashData, wifiAp, networkIndex, false, saltedMeshName);
      insertIntoAccessPointsList(wifiAp);
    }
  }
}

void showWifiScanInfo(bool showHeader, bool isRecognized) {
  if (showHeader) {
    Logs::serialPrintln(me, F("# AP TYPE      RSSI   LEVEL   SSID"));
  }
  AccessPointInfo *strongestAccessPoint = getStrongestAccessPoint(NULL, 0, 0);
  AccessPointList *currNode = getAccessPointsList();
  while (currNode != nullptr) {
    if (currNode->ap->isRecognized == isRecognized) {
      if (currNode->ap->isRecognized) {
        Logs::serialPrint(me, F("# Recognized    "));
      } else {
        Logs::serialPrint(me, F("# Unrecognized  "));
      }
      Logs::serialPrint(me, String(currNode->ap->RSSI), F("       "));
      Logs::serialPrint(me, String(currNode->ap->apLevel), F("   "));
      Logs::serialPrint(me, currNode->ap->SSID);
      if (WiFi.SSID() == currNode->ap->SSID) {
        Logs::serialPrint(me, F(" ("), WiFi.localIP().toString(), F(")"));
      }
      if (strongestAccessPoint == currNode->ap) {
        Logs::serialPrint(me, F(" (strongest AP)"));
      }
      Logs::serialPrintln(me, F(""));
    }
    currNode = currNode->next;
  }
}

}  // namespace AccessPoints