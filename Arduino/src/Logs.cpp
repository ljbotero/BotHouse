#include "Logs.h"
#ifdef ENABLE_REMOTE_LOGGING
#include <ArduinoWebsockets.h>
#endif
#include <ESP8266WiFi.h>
#include "Config.h"
#include "Events.h"
#include "MessageGenerator.h"
#include "Utils.h"
#include "WebServer.h"

namespace Logs {

static bool _disableSerialLog = false;

static const Logs::caller me = Logs::caller::Logs;

static char serialPrintLeadingSpaces[10] = "\0";
// static char lastMessage[100] = "\0";
static char SERIAL_PRINT_NEST_SPACES[2] = {' ', ' '};
static const auto MAX_LENGTH_SINGLE_LINE = 256;
// static const auto MIN_TIME_DISTANCE_BETWEEN_SAME_MESSAGES = 8000;
static bool loggingPaused = false;
static bool isNewLine = false;
// auto currentLogHistoryQueueLength = 0;
// unsigned long lastTimeGetHistoryQueue = 0;
// static unsigned long lastMessageTimestamp = 0;

#ifdef ENABLE_REMOTE_LOGGING
static websockets::WebsocketsServer webSocketServer;
static websockets::WebsocketsClient webSocketClient;
bool webSocketsAvailable = false;
#endif

void pauseLogging(bool paused) {
  loggingPaused = paused;
}

void ICACHE_FLASH_ATTR logEspInfo() {
  Logs::serialPrintln(me, PSTR("ESP Full Version: "), String(ESP.getFullVersion()).c_str());
  Logs::serialPrintln(me, PSTR("MAC Address:          "), String(WiFi.macAddress()).c_str());
  Logs::serialPrintln(me, PSTR("ChipId:               "), String(ESP.getChipId()).c_str());
  Logs::serialPrintln(me, PSTR("Sdk Version:          "), String(ESP.getSdkVersion()).c_str());
  Logs::serialPrintln(me, PSTR("Core Version:         "), String(ESP.getCoreVersion()).c_str());
  Logs::serialPrintln(me, PSTR("Boot Version:         "), String(ESP.getBootVersion()).c_str());
  Logs::serialPrintln(
      me, PSTR("Flash Chip Vendor Id: "), String(ESP.getFlashChipVendorId()).c_str());
  Logs::serialPrintln(me, PSTR("Flash Chip Id:        "), String(ESP.getFlashChipId()).c_str());
  Logs::serialPrint(me, PSTR("Reset Reason: "), String(ESP.getResetReason()).c_str());
  Logs::serialPrintln(me, String(ESP.getResetInfo()).c_str());
}

// bool isMessageSameAsRecentLast(const String &message, bool append) {
//   if (!append && message == lastMessage &&
//       Utils::getNormailzedTime() - lastMessageTimestamp <
//       MIN_TIME_DISTANCE_BETWEEN_SAME_MESSAGES) {
//     lastMessageTimestamp = Utils::getNormailzedTime();
//     return true;
//   }
//   lastMessageTimestamp = Utils::getNormailzedTime();
//   lastMessage = message;
//   return false;
// }

void concatCharArray(
    char *buffer, bool append, const size_t bufferSize, const char *message = NULL) {
  if (message == nullptr || buffer == nullptr) {
    return;
  }
  if (append) {
    strncat_P(buffer, (PGM_P)message, bufferSize - strlen_P((PGM_P)buffer) - 1);
  } else {
    strncpy_P(buffer, (PGM_P)message, bufferSize - 1);
  }
}

void concatCharArrays(char buffer[], bool append, const size_t bufferSize, const char message1[],
    const char message2[] = "\0", const char message3[] = "\0", const char message4[] = "\0",
    const char message5[] = "\0", const char message6[] = "\0", const char message7[] = "\0") {
  concatCharArray(buffer, append, bufferSize, message1);
  concatCharArray(buffer, true, bufferSize, message2);
  concatCharArray(buffer, true, bufferSize, message3);
  concatCharArray(buffer, true, bufferSize, message4);
  concatCharArray(buffer, true, bufferSize, message5);
  concatCharArray(buffer, true, bufferSize, message6);
  concatCharArray(buffer, true, bufferSize, message7);
}

void disableSerialLog(bool disableSerialLogger) {
  _disableSerialLog = disableSerialLogger;
}

#ifdef ENABLE_REMOTE_LOGGING
char singleLineMsg[MAX_LENGTH_SINGLE_LINE] = "\0";

void onEventsCallback(websockets::WebsocketsEvent event, String data) {
  if (event == websockets::WebsocketsEvent::ConnectionOpened) {
    serialPrintln(me, PSTR("webSocketServer:Connection opened"));
    // webSocketsAvailable = true;
  } else if (event == websockets::WebsocketsEvent::ConnectionClosed) {
    serialPrintln(me, PSTR("webSocketServer:Connection closed"));
    webSocketsAvailable = false;
  } else if (event == websockets::WebsocketsEvent::GotPing) {
    Serial.println(PSTR("Got a Ping!"));
  } else if (event == websockets::WebsocketsEvent::GotPong) {
    Serial.println(PSTR("Got a Pong!"));
  }
}
#endif

void sendMessage(char message[], bool serialPrintIsNewLine) {
#ifdef ENABLE_REMOTE_LOGGING
  if (strlen(message) == 0) {
    return;
  }
  auto bufferSize = MAX_LENGTH_SINGLE_LINE - strlen(singleLineMsg);
  strncat_P(singleLineMsg, (PGM_P)message, bufferSize);
  if (serialPrintIsNewLine) {
    if (!WiFi.isConnected()) {
      webSocketsAvailable = false;
    }
    if (webSocketsAvailable) {
      webSocketClient.send(singleLineMsg);
    }
    singleLineMsg[0] = '\0';
  }
#endif
}

void serialPrintBase(char newMessage[], caller callingNamespace, const char message1[],
    const char message2[], const char message3[], bool newLine) {
  if (newLine) {
    concatCharArrays(newMessage, false, MAX_LENGTH_SINGLE_LINE, serialPrintLeadingSpaces, PSTR("("),
        callerNames[callingNamespace].c_str(), PSTR(") "), message1, message2, message3);
  } else {
    concatCharArrays(newMessage, true, MAX_LENGTH_SINGLE_LINE, message1, message2, message3);
  }
}

void serialPrint(
    caller callingNamespace, const char message1[], const char message2[], const char message3[]) {
  char newMessage[MAX_LENGTH_SINGLE_LINE] = "\0";
  serialPrintBase(newMessage, callingNamespace, message1, message2, message3, isNewLine);
#ifdef DEBUG_MODE
  if (!_disableSerialLog) {  // && !isMessageSameAsRecentLast(newMessage, true)) {
    Serial.print(newMessage);
  }
#endif
  if (!loggingPaused) {
    sendMessage(newMessage, isNewLine);
  }
  isNewLine = false;
}

#ifdef DEBUG_MODE
void appendMemory(char buffer[]) {
  concatCharArrays(buffer, true, MAX_LENGTH_SINGLE_LINE, PSTR(" ("),
      String(ESP.getFreeHeap()).c_str(), PSTR("  Free Heap)"));
}
#endif

/// Prints a new character at the end
void serialPrintln(
    caller callingNamespace, const char message1[], const char message2[], const char message3[]) {
  char newMessage[MAX_LENGTH_SINGLE_LINE] = "\0";
  serialPrintBase(newMessage, callingNamespace, message1, message2, message3, isNewLine);
#ifdef DEBUG_MODE
  if (!_disableSerialLog) {
    appendMemory(newMessage);
    Serial.println(newMessage);
  }
#endif
  if (!loggingPaused) {
    sendMessage(newMessage, isNewLine);
  }
  isNewLine = true;
}

void serialPrintlnStart(
    caller callingNamespace, const char message1[], const char message2[], const char message3[]) {
  serialPrintln(callingNamespace, message1, message2, message3);
  concatCharArrays(serialPrintLeadingSpaces, false, 10, SERIAL_PRINT_NEST_SPACES);
}

void serialPrintlnEnd(
    caller callingNamespace, const char message1[], const char message2[], const char message3[]) {
  int newLen = strlen(serialPrintLeadingSpaces) - strlen(SERIAL_PRINT_NEST_SPACES);
  if (newLen <= 0) {
    newLen = 0;
  }
  serialPrintLeadingSpaces[newLen] = '\0';

  char finalMessage[MAX_LENGTH_SINGLE_LINE] = "\0";
  concatCharArrays(finalMessage, false, MAX_LENGTH_SINGLE_LINE, message1, message2, message3);
  if (strlen(finalMessage) > 0) {
    serialPrintln(callingNamespace, finalMessage);
  }
}

void handle() {
  // webSocketClient.onEvent(onEventsCallback);
  // Serial.println("New websocket client connected");
#ifdef ENABLE_REMOTE_LOGGING
  if (!webSocketsAvailable) {
    if (webSocketServer.poll()) {
      webSocketClient = webSocketServer.accept();
      webSocketClient.onEvent(onEventsCallback);
      webSocketsAvailable = true;
    }
  }
#endif
}

void ICACHE_FLASH_ATTR setup() {
#ifdef ENABLE_REMOTE_LOGGING
  webSocketServer.listen(90);
  serialPrintln(me, PSTR("Websocket Logs server "), String(webSocketServer.available()).c_str());
#endif
}

}  // namespace Logs

// unsigned long lastTimeReceivedBroadcastLogsRequest = 0;
// const auto MAX_LOG_HISTORY_QUEUE = 50;
// const auto MAX_AGE_HISTORY_QUEUE = 15000;
// const auto MAX_AGE_BROADCAST_LOG = 10000;
// const auto HEAP_LIMIT_FOR_LOGS = 18000;
// LogHistory *logHistoryQueue = NULL;
// LogHistory *logHistoryQueueEnd = NULL;
// LogHistory *getLogHistoryQueue() {
//   lastTimeGetHistoryQueue = millis();
//   return logHistoryQueue;
// }
// void broadcastLog(const String &message, bool append) {
//   if (Utils::getNormailzedTime() < lastTimeReceivedBroadcastLogsRequest) {
//     lastTimeReceivedBroadcastLogsRequest = 0;
//   }
//   if (lastTimeReceivedBroadcastLogsRequest == 0 ||
//       Utils::getNormailzedTime() - lastTimeReceivedBroadcastLogsRequest > MAX_AGE_BROADCAST_LOG)
//       {
//     return;
//   }
//   String json = MessageGenerator::generateLogMessage(message, append);
//   Network::broadcastEverywhere(json);
// }
// void deleteHistoryQueue() {
//   if (logHistoryQueue == nullptr) {
//     return;
//   }
//   Logs::serialPrintln(me, PSTR("deleteHistoryQueue"));
//   LogHistory *toDelete = logHistoryQueue;
//   while (toDelete != nullptr) {
//     LogHistory *next = toDelete->next;
//     delete toDelete;
//     toDelete = next;
//   }
//   logHistoryQueue = NULL;
//   logHistoryQueueEnd = NULL;
//   currentLogHistoryQueueLength = 0;
// }

// bool logHistoryEnqueue(const String &message, boolean append) {
//   return true;
//   // LogData data;
//   // data.deviceName = Devices::getDeviceName();
//   // data.timestamp = Utils::getNormailzedTime();
//   // data.message = message;
//   // return enqueue(data, append);
// }
// bool enqueue(LogData &data, bool append) {
//   if (logHistoryQueue != nullptr && data.timestamp < logHistoryQueue->data.timestamp) {
//     data.timestamp = Utils::getNormailzedTime();
//   }
//   if (millis() < lastTimeGetHistoryQueue) {
//     lastTimeGetHistoryQueue = 0;
//   }
//   if (millis() - lastTimeGetHistoryQueue > MAX_AGE_HISTORY_QUEUE && lastTimeGetHistoryQueue > 0)
//   {
//     deleteHistoryQueue();
//     return false;
//   }
//   lastTimeReceivedBroadcastLogsRequest = 0;
//   if (logHistoryQueue == nullptr) {
//     logHistoryQueue = new LogHistory;
//     logHistoryQueue->next = NULL;
//     logHistoryQueue->prev = NULL;
//     logHistoryQueueEnd = logHistoryQueue;
//     logHistoryQueue->data.timestamp = data.timestamp;
//     logHistoryQueue->data.deviceName = data.deviceName;
//     logHistoryQueue->data.message = data.message;
//     currentLogHistoryQueueLength++;
//   } else if (append) {
//     logHistoryQueue->data.message += data.message;
//   } else {
//     LogHistory *previousTip = logHistoryQueue;
//     logHistoryQueue = new LogHistory;
//     logHistoryQueue->next = previousTip;
//     logHistoryQueue->prev = NULL;
//     logHistoryQueue->data.timestamp = data.timestamp;
//     logHistoryQueue->data.deviceName = data.deviceName;
//     logHistoryQueue->data.message = data.message;
//     previousTip->prev = logHistoryQueue;
//     currentLogHistoryQueueLength++;
//   }

//   while ((currentLogHistoryQueueLength > MAX_LOG_HISTORY_QUEUE ||
//              ESP.getFreeHeap() < HEAP_LIMIT_FOR_LOGS) &&
//          logHistoryQueueEnd != nullptr) {
//   //while (currentLogHistoryQueueLength > MAX_LOG_HISTORY_QUEUE && logHistoryQueueEnd != nullptr)
//   {
//     LogHistory *toDelete = logHistoryQueueEnd;
//     logHistoryQueueEnd = logHistoryQueueEnd->prev;
//     delete toDelete;
//     if (logHistoryQueueEnd != nullptr) {
//       logHistoryQueueEnd->next = NULL;
//     } else {
//       logHistoryQueue = NULL;
//     }
//     currentLogHistoryQueueLength--;
//   }
//   return true;
// }

// void receivedBroadcastLogsRequest() {
//   if (lastTimeReceivedBroadcastLogsRequest == 0 ||
//       Utils::getNormailzedTime() - lastTimeReceivedBroadcastLogsRequest > MAX_AGE_BROADCAST_LOG)
//       {
//     serialPrintln(me, F("receivedBroadcastLogsRequest"));
//   }
//   lastTimeReceivedBroadcastLogsRequest = Utils::getNormailzedTime();
// }
