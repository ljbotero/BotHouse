#include "../src/Config.h"
#ifdef RUN_UNIT_TESTS
#include "../src/AccessPoints.h"
#include "../src/Mesh.h"

test(Mesh_calculateAccessPointLevel, NotEnoughHeap) {
  AccessPoints::AccessPointList *accessPointList = NULL;
  AccessPoints::AccessPointInfo *strongestRecognizedAccessPoint = NULL;
  uint32_t freeHeap = MIN_HEAP_TO_BE_AP - 1;
  int32_t actual = Mesh::calculateAccessPointLevel(
      accessPointList, strongestRecognizedAccessPoint, freeHeap);
  int32_t expected = 0;
  assertEqual(actual, expected);
}

//*******************************************************
//                       [me-1]
//*******************************************************
test(Mesh_calculateAccessPointLevel, NoAPs) {
  AccessPoints::AccessPointList *accessPointList = NULL;
  AccessPoints::AccessPointInfo *strongestRecognizedAccessPoint = NULL;
  uint32_t freeHeap = MIN_HEAP_TO_BE_AP;
  int32_t actual = Mesh::calculateAccessPointLevel(
      accessPointList, strongestRecognizedAccessPoint, freeHeap);
  int32_t expected = -1;
  assertEqual(actual, expected);
}

//*******************************************************
//                  [me-2]---->[ap-1]
//*******************************************************
test(Mesh_calculateAccessPointLevel, SingleAP) {

  AccessPoints::AccessPointInfo *strongestRecognizedAccessPoint = new AccessPoints::AccessPointInfo;
  strongestRecognizedAccessPoint->apLevel = -1;
  strongestRecognizedAccessPoint->RSSI = MINIMAL_SIGNAL_STRENGHT - 1;
  strcpy(strongestRecognizedAccessPoint->SSID, String("ap-1").c_str());

  AccessPoints::AccessPointList *accessPointList = new AccessPoints::AccessPointList;
  accessPointList->next = NULL;
  accessPointList->ap = strongestRecognizedAccessPoint;

  uint32_t freeHeap = MIN_HEAP_TO_BE_AP;
  int32_t actual = Mesh::calculateAccessPointLevel(
      accessPointList, strongestRecognizedAccessPoint, freeHeap);
  int32_t expected = -2;
  assertEqual(actual, expected);
}

//*******************************************************
//                   [me2]---->[ap1]---->...
//*******************************************************
test(Mesh_calculateAccessPointLevel, SingleAPpositiveLevels) {

  AccessPoints::AccessPointInfo *strongestRecognizedAccessPoint = new AccessPoints::AccessPointInfo;
  strongestRecognizedAccessPoint->apLevel = 1;
  strongestRecognizedAccessPoint->RSSI = MINIMAL_SIGNAL_STRENGHT - 1;
  strcpy(strongestRecognizedAccessPoint->SSID, String("ap1").c_str());

  AccessPoints::AccessPointList *accessPointList = new AccessPoints::AccessPointList;
  accessPointList->next = NULL;
  accessPointList->ap = strongestRecognizedAccessPoint;

  uint32_t freeHeap = MIN_HEAP_TO_BE_AP;
  int32_t actual = Mesh::calculateAccessPointLevel(
      accessPointList, strongestRecognizedAccessPoint, freeHeap);
  int32_t expected = 2;
  assertEqual(actual, expected);
}

//*******************************************************
//                   [ap-1]------------->[ap-2]
//                              [me0]--->[ap-2]
//*******************************************************
test(Mesh_calculateAccessPointLevel, InTheMiddleOfAPs) {

  AccessPoints::AccessPointInfo *ap1 = new AccessPoints::AccessPointInfo;
  ap1->apLevel = 1;
  strcpy(ap1->SSID, String("ap1").c_str());

  AccessPoints::AccessPointInfo *ap2 = new AccessPoints::AccessPointInfo;
  ap2->apLevel = 2;
  strcpy(ap2->SSID, String("ap2").c_str());

  AccessPoints::AccessPointList *accessPointList = new AccessPoints::AccessPointList;
  accessPointList->next = NULL;
  accessPointList->ap = ap1;

  accessPointList->next = new AccessPoints::AccessPointList;
  accessPointList->next->next = NULL;
  accessPointList->next->ap = ap2;

  AccessPoints::AccessPointInfo *strongestRecognizedAccessPoint = ap1;

  uint32_t freeHeap = MIN_HEAP_TO_BE_AP;
  int32_t actual = Mesh::calculateAccessPointLevel(
      accessPointList, strongestRecognizedAccessPoint, freeHeap);
  int32_t expected = 0;
  assertEqual(actual, expected);
}

//*******************************************************
//                          [ap1]------->[wifi]
//                   [me0]->[ap1]
//*******************************************************

test(Mesh_isAccessPointPotentialNode) {
  String arg = "";
  assertFalse(Mesh::isAccessPointAPotentialNode(arg.c_str()));
  arg = "abc";
  assertFalse(Mesh::isAccessPointAPotentialNode(arg.c_str()));
  arg = "_";
  assertFalse(Mesh::isAccessPointAPotentialNode(arg.c_str()));
  arg = "___ ";
  assertFalse(Mesh::isAccessPointAPotentialNode(arg.c_str()));

  arg = "___1";
  assertTrue(Mesh::isAccessPointAPotentialNode(arg.c_str()));
  arg = "___-2";
  assertTrue(Mesh::isAccessPointAPotentialNode(arg.c_str()));
  arg = "aaab b b_-1000";
  assertTrue(Mesh::isAccessPointAPotentialNode(arg.c_str()));
  arg = "aaab b b_1000";
  assertTrue(Mesh::isAccessPointAPotentialNode(arg.c_str()));
}

//*******************************************************
//                                [me]-->[wifi]
//                   [ap1]-------------->[wifi]
//*******************************************************

//*******************************************************
//       [me(X+1)]---->[ap1]---->[apX]      [apY]
//_______________________________________________________
//            apX: Next closest AP
//            apY: Furhest AP
//*******************************************************

//*******************************************************
//                   [ap-1]------------->[ap-2]
//                   [ap-1]<---[me0]
//*******************************************************

//*******************************************************
//                             [ap1]---->[wifi]
//                   [me2]-------------->[wifi]
//*******************************************************

#endif