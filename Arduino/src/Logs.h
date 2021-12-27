#ifndef Logs_h
#define Logs_h
#include "Config.h"

namespace Logs {

enum caller {
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
};

static const String callerNames[] = {FPSTR("AccessPoints"), FPSTR("Main"), FPSTR("Config"),
    FPSTR("Devices"), FPSTR("Events"), FPSTR("Files"), FPSTR("HubsIntegration"), FPSTR("Logs"),
    FPSTR("Mesh"), FPSTR("MessageGenerator"), FPSTR("MessageProcessor"), FPSTR("MDns"),
    FPSTR("Network"), FPSTR("UniversalPlugAndPlay"), FPSTR("OTAupdates"), FPSTR("Storage"),
    FPSTR("Utils"), FPSTR("WebServer")};


void serialPrintln(caller callingNamespace, const char message1[], const char message2[] = "\0",
    const char message3[] = "\0");
void serialPrintlnStart(caller callingNamespace, const char message1[],
    const char message2[] = "\0", const char message3[] = "\0");
void serialPrintlnEnd(caller callingNamespace, const char message1[] = "\0",
    const char message2[] = "\0", const char message3[] = "\0");
void serialPrint(caller callingNamespace, const char message1[], const char message2[] = "\0",
    const char message3[] = "\0");
void pauseLogging(bool paused);
void disableSerialLog(bool disableSerialLog);
void logEspInfo();
void handle();
void setup();

}  // namespace Logs
#endif

// void receivedBroadcastLogsRequest();

// bool enqueue(LogData &data, bool append);
// void deleteHistoryQueue();

// struct LogData {
//   unsigned long timestamp;
//   String deviceName;
//   String message;
// };

// struct LogHistory {
//   LogData data;
//   LogHistory *next = NULL;
//   LogHistory *prev = NULL;
// };

// LogHistory *getLogHistoryQueue();