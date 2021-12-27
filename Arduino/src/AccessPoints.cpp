#include "AccessPoints.h"
#include <ESP8266WiFi.h>
#include "Config.h"
#include "Logs.h"
#include "Mesh.h"
#include "Storage.h"
#include "Utils.h"

namespace AccessPoints {

static const Logs::caller me = Logs::caller::AccessPoints;
static AccessPointList *_accessPointsList = NULL;
static AccessPointInfo *_accessPointHomeWifi = NULL;

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

AccessPointConnectionInfo *getAccessPointInfo(const char *SSID) {
  if (strlen(SSID) == 0) {
    return NULL;
  }
  AccessPointConnectionInfo *currInfo = _rootInfo;
  while (currInfo != nullptr && !String(SSID).equalsIgnoreCase(String(currInfo->SSID))) {
    currInfo = currInfo->next;
  }
  if (currInfo == nullptr) {
    Logs::serialPrintln(
        me, PSTR("getAccessPointInfo:new:AccessPointConnectionInfo:'"), SSID, PSTR("'"));
    currInfo = new AccessPointConnectionInfo;
    Utils::sstrncpy(currInfo->SSID, SSID, MAX_LENGTH_SSID);
    currInfo->connectionAttempts = 0;
    currInfo->lastFailedConnection = 0;
    currInfo->next = _rootInfo;
    _rootInfo = currInfo;
  }
  return currInfo;
}

void ICACHE_FLASH_ATTR setAccessPointInfo(Storage::storageStruct &flashData, AccessPointInfo *info,
    int networkIndex, bool isRecognized, const String &saltedMeshName) {
  Utils::sstrncpy(info->SSID, WiFi.SSID(networkIndex).c_str(), MAX_LENGTH_SSID);
  info->apLevel = 0;
  if (String(info->SSID).startsWith(saltedMeshName)) {
    String appLevelStr = String(info->SSID).substring(saltedMeshName.length());
    if (!appLevelStr.isEmpty()) {
      info->apLevel = appLevelStr.toInt();
    }
  }
  info->wifiChannel = WiFi.channel(networkIndex);
  info->networkIndex = networkIndex;
  info->RSSI = WiFi.RSSI(networkIndex);
  info->isRecognized = isRecognized;
  info->isOpen = WiFi.encryptionType(networkIndex) == wl_enc_type::ENC_TYPE_NONE;
  // uint8_t *newBSSID = WiFi.BSSID(networkIndex);
  // if (newBSSID != nullptr) {
  //   if (info->BSSID == nullptr) {
  //     uint8_t _bssidArray[6]{0};
  //     info->BSSID = _bssidArray;
  //   }
  //   for (int i = 0; i < 6; i++) {
  //     info->BSSID[i] = newBSSID[i];
  //   }
  // } else {
  //   info->BSSID = NULL;
  // }
}

ICACHE_FLASH_ATTR AccessPointInfo *getStrongestAccessPoint(AccessPointInfo *excludeAP,
    int32_t RSSIGreatherThan, int32_t apLevelLowerThan, uint8_t connectionAttemptsLessThan,
    uint32_t notFailedConnectingSince) {
  AccessPointInfo *strongestAP = NULL;
  AccessPointList *currNode = getAccessPointsList();
  // Logs::serialPrint(me, PSTR("getStrongestAccessPoint: "));
  // Logs::serialPrint(me, PSTR("excludeAP="), excludeAP == nullptr ? "" : excludeAP->SSID);
  // Logs::serialPrint(me, PSTR(", RSSIGreatherThan="), String(RSSIGreatherThan).c_str());
  // Logs::serialPrint(me, PSTR(", apLevelLowerThan="), String(apLevelLowerThan).c_str());
  // Logs::serialPrint(me, PSTR(", connectionAttemptsLessThan="),
  // String(connectionAttemptsLessThan).c_str()); Logs::serialPrintln(me, PSTR(",
  // notFailedConnectingSince="), String(notFailedConnectingSince).c_str());

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
    Logs::serialPrint(me, PSTR("getStrongestAccessPoint = "), strongestAP->SSID);
    Logs::serialPrintln(me, PSTR(" "), String(strongestAP->RSSI).c_str(), PSTR("db"));
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

bool isAccessPointInRange(const char *SSID) {
  AccessPointList *currNode = getAccessPointsList();
  while (currNode != nullptr) {
    if (strncmp(currNode->ap->SSID, SSID, MAX_LENGTH_SSID) == 0) {
      return true;
    }
    currNode = currNode->next;
  }
  return false;
}

ICACHE_FLASH_ATTR AccessPointInfo *getAccessPointWithHighestLevel(
    AccessPointList *accessPointsList) {
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

void ICACHE_FLASH_ATTR deleteAccessPoint(AccessPointInfo *toDelete) {
  if (toDelete == nullptr) {
    return;
  }
  // if (toDelete->BSSID != nullptr) {
  //   Logs::serialPrintlnStart(me, PSTR("deleteAccessPoint:BSSID"));
  //   delete toDelete->BSSID;
  // }
  delete toDelete;
}

void ICACHE_FLASH_ATTR deleteNodeAccessPoints(AccessPointList *accessPoints) {
  AccessPointList *toDelete = accessPoints;
  while (toDelete != nullptr) {
    deleteAccessPoint(toDelete->ap);
    AccessPointList *next = toDelete->next;
    delete toDelete;
    toDelete = next;
  }
}

void ICACHE_FLASH_ATTR insertIntoAccessPointsList(AccessPointInfo *ap) {
  if (ap == nullptr) {
    return;
  }
  // Check if SSID has previously been scanned
  AccessPointList *currNode = getAccessPointsList();
  AccessPointList *lastNode = NULL;
  while (currNode != nullptr) {
    lastNode = currNode;
    currNode = currNode->next;
  }

  if (getAccessPointsList() == nullptr) {
    AccessPointList *accessPointList = new AccessPointList;
    accessPointList->next = NULL;
    accessPointList->ap = ap;
    setAccessPointsList(accessPointList);
    return;
  }
  // Add new node
  AccessPointList *newNode = new AccessPointList;
  newNode->ap = ap;
  newNode->next = NULL;
  lastNode->next = newNode;
}

void ICACHE_FLASH_ATTR scanNetworksAnalysis(
    Storage::storageStruct &flashData, int numberOfNetworks) {
  Logs::serialPrintlnStart(me, PSTR("scanNetworksAnalysis"));
  // Find router Wifi first
  String saltedMeshName((char *)0);
  Mesh::getSaltedMeshName(saltedMeshName, flashData);
  for (int networkIndex = 0; networkIndex < numberOfNetworks; networkIndex++) {
    String currentSSID = WiFi.SSID(networkIndex);
    if (currentSSID.isEmpty()) {
      Logs::serialPrintln(me, PSTR("No name found for SSID - Skipping"));
      continue;
    }
    if (isAccessPointInRange(currentSSID.c_str())) {
      // Prevent adding AP twice
      continue;
    }
    if (currentSSID.equalsIgnoreCase(flashData.wifiName)) {
      AccessPointInfo *accessPointHomeWifi = new AccessPointInfo;
      setAccessPointInfo(flashData, accessPointHomeWifi, networkIndex, true, saltedMeshName);
#ifdef FORCE_MASTER_NODE
      accessPointHomeWifi->RSSI = -5;
#endif
      insertIntoAccessPointsList(accessPointHomeWifi);
      setAccessPointHomeWifi(accessPointHomeWifi);
    } else if (currentSSID.startsWith(saltedMeshName)) {
      AccessPointInfo *APNode = new AccessPointInfo;
      setAccessPointInfo(flashData, APNode, networkIndex, true, saltedMeshName);
#ifdef FORCE_MASTER_NODE
      APNode->RSSI = APNode->RSSI - 20;
#endif
      insertIntoAccessPointsList(APNode);
#ifdef ENABLE_NAT_ROUTER
    } else if (currentSSID.startsWith(MESH_SSID_NAME)) {
      AccessPointInfo *APNode = new AccessPointInfo;
      setAccessPointInfo(flashData, APNode, networkIndex, true, MESH_SSID_NAME);
      insertIntoAccessPointsList(APNode);
#endif
    } else {
      AccessPointInfo *wifiAp = new AccessPointInfo;
      setAccessPointInfo(flashData, wifiAp, networkIndex, false, saltedMeshName);
      insertIntoAccessPointsList(wifiAp);
    }
  }
  Logs::serialPrintlnEnd(me);
}

void ICACHE_FLASH_ATTR showWifiScanInfo(bool showHeader, bool isRecognized) {
  if (showHeader) {
    Logs::serialPrintln(me, PSTR("# AP TYPE      RSSI   LEVEL   SSID"));
  }
  AccessPointInfo *strongestAccessPoint = getStrongestAccessPoint(NULL, 0, 0);
  AccessPointList *currNode = getAccessPointsList();
  while (currNode != nullptr) {
    if (currNode->ap->isRecognized == isRecognized) {
      if (currNode->ap->isRecognized) {
        Logs::serialPrint(me, PSTR("# Recognized    "));
      } else {
        Logs::serialPrint(me, PSTR("# Unrecognized  "));
      }
      Logs::serialPrint(me, String(currNode->ap->RSSI).c_str(), PSTR("       "));
      Logs::serialPrint(me, String(currNode->ap->apLevel).c_str(), PSTR("   "));
      Logs::serialPrint(me, currNode->ap->SSID);
      if (WiFi.SSID() == String(currNode->ap->SSID)) {
        Logs::serialPrint(me, PSTR(" ("), WiFi.localIP().toString().c_str(), PSTR(")"));
      }
      if (strongestAccessPoint == currNode->ap) {
        Logs::serialPrint(me, PSTR(" (strongest AP)"));
      }
      Logs::serialPrintln(me, PSTR(""));
    }
    currNode = currNode->next;
  }
}

}  // namespace AccessPoints