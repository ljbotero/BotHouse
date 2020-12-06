#ifndef Logs_h
#define Logs_h
#include <ArduinoJson.h>

namespace Logs {

typedef enum {
  AccessPoints,
  Main,
  Config,
  Devices,
  Events,
  Files,
  Hub,
  Logs,
  Mesh,
  MessageGenerator,
  MessageProcessor,
  MDns,
  Network,
  UniversalPlugAndPlay,
  OTAupdates,
  Storage,
  Utils,
  WebServer
} caller;

struct LogData {
  unsigned long timestamp;
  String deviceName;
  String message;
};

struct LogHistory {
  LogData data;
  LogHistory *next = NULL;
  LogHistory *prev = NULL;
};

LogHistory *getLogHistoryQueue();
void serialPrintln(caller callingNamespace, const String &message1, const String &message2 = "",
    const String &message3 = "");
void serialPrintlnStart(caller callingNamespace, const String &message1,
    const String &message2 = "", const String &message3 = "");
void serialPrintlnEnd(caller callingNamespace, const String &message1 = "",
    const String &message2 = "", const String &message3 = "");
void serialPrint(caller callingNamespace, const String &message1, const String &message2 = "",
    const String &message3 = "");
void receivedBroadcastLogsRequest();
bool enqueue(LogData &data, bool append);
void pauseLogging(bool paused);
void deleteHistoryQueue();

}  // namespace Logs
#endif
