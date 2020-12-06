#include "Logs.h"
#include "Config.h"
#include "Events.h"
#include "MessageGenerator.h"
#include "Utils.h"
#include "WebServer.h"

namespace Logs {

const Logs::caller me = Logs::caller::Logs;

const String callerNames[] = {"AccessPoints", "Main", "Config", "Devices", "Events", "Files",
    "HubsIntegration", "Logs", "Mesh", "MessageGenerator", "MessageProcessor", "MDns", "Network",
    "UniversalPlugAndPlay", "OTAupdates", "Storage", "Utils", "WebServer"};

const auto MAX_LOG_HISTORY_QUEUE = 30;
const auto MAX_AGE_HISTORY_QUEUE = 60000;
const auto MAX_AGE_BROADCAST_LOG = 30000;
const auto HEAP_LIMIT_FOR_LOGS = 10000;

String serialPrintLeadingSpaces = "";
String lastMessage = "";
const uint16_t SERIAL_PRINT_NEST_SPACES = 2;
const uint32_t MIN_TIME_DISTANCE_BETWEEN_SAME_MESSAGES = 8000;
bool serialPrintIsNewLine = false;
bool loggingPaused = false;
LogHistory *logHistoryQueue = NULL;
LogHistory *logHistoryQueueEnd = NULL;
auto currentLogHistoryQueueLength = 0;
unsigned long lastTimeGetHistoryQueue = 0;
unsigned long lastMessageTimestamp = 0;
unsigned long lastTimeReceivedBroadcastLogsRequest = 0;

void pauseLogging(bool paused) {
  loggingPaused = paused;
}

void receivedBroadcastLogsRequest() {
  if (lastTimeReceivedBroadcastLogsRequest == 0 ||
      Utils::getNormailzedTime() - lastTimeReceivedBroadcastLogsRequest > MAX_AGE_BROADCAST_LOG) {
    serialPrintln(me, F("receivedBroadcastLogsRequest"));
  }
  lastTimeReceivedBroadcastLogsRequest = Utils::getNormailzedTime();
}

LogHistory *getLogHistoryQueue() {
  lastTimeGetHistoryQueue = millis();
  return logHistoryQueue;
}

void deleteHistoryQueue() {
  if (logHistoryQueue == nullptr) {
    return;
  }
  Logs::serialPrintln(me, F("deleteHistoryQueue"));
  LogHistory *toDelete = logHistoryQueue;
  while (toDelete != nullptr) {
    LogHistory *next = toDelete->next;
    delete toDelete;
    toDelete = next;
  }
  logHistoryQueue = NULL;
  logHistoryQueueEnd = NULL;
  currentLogHistoryQueueLength = 0;
}

bool enqueue(LogData &data, bool append) {
  if (logHistoryQueue != nullptr && data.timestamp < logHistoryQueue->data.timestamp) {
    data.timestamp = Utils::getNormailzedTime();
  }
  if (millis() < lastTimeGetHistoryQueue) {
    lastTimeGetHistoryQueue = 0;
  }
  if (millis() - lastTimeGetHistoryQueue > MAX_AGE_HISTORY_QUEUE && lastTimeGetHistoryQueue > 0) {
    deleteHistoryQueue();
    return false;
  }
  lastTimeReceivedBroadcastLogsRequest = 0;
  if (logHistoryQueue == nullptr) {
    logHistoryQueue = new LogHistory;
    logHistoryQueue->next = NULL;
    logHistoryQueue->prev = NULL;
    logHistoryQueueEnd = logHistoryQueue;
    logHistoryQueue->data.timestamp = data.timestamp;
    logHistoryQueue->data.deviceName = data.deviceName;
    logHistoryQueue->data.message = data.message;
    currentLogHistoryQueueLength++;
  } else if (append) {
    logHistoryQueue->data.message += data.message;
  } else {
    LogHistory *previousTip = logHistoryQueue;
    logHistoryQueue = new LogHistory;
    logHistoryQueue->next = previousTip;
    logHistoryQueue->prev = NULL;
    logHistoryQueue->data.timestamp = data.timestamp;
    logHistoryQueue->data.deviceName = data.deviceName;
    logHistoryQueue->data.message = data.message;
    previousTip->prev = logHistoryQueue;
    currentLogHistoryQueueLength++;
  }

  while ((currentLogHistoryQueueLength > MAX_LOG_HISTORY_QUEUE ||
             ESP.getFreeHeap() < HEAP_LIMIT_FOR_LOGS) &&
         logHistoryQueueEnd != nullptr) {
    LogHistory *toDelete = logHistoryQueueEnd;
    logHistoryQueueEnd = logHistoryQueueEnd->prev;
    delete toDelete;
    if (logHistoryQueueEnd != nullptr) {
      logHistoryQueueEnd->next = NULL;
    } else {
      logHistoryQueue = NULL;
    }
    currentLogHistoryQueueLength--;
  }
  return true;
}

bool logHistoryEnqueue(const String &message, boolean append) {
  LogData data;
  data.deviceName = Devices::getDeviceName();
  data.timestamp = Utils::getNormailzedTime();
  data.message = message;
  return enqueue(data, append);
}

void broadcastLog(const String &message, bool append) {
  if (Utils::getNormailzedTime() < lastTimeReceivedBroadcastLogsRequest) {
    lastTimeReceivedBroadcastLogsRequest = 0;
  }
  if (lastTimeReceivedBroadcastLogsRequest == 0 ||
      Utils::getNormailzedTime() - lastTimeReceivedBroadcastLogsRequest > MAX_AGE_BROADCAST_LOG) {
    return;
  }
  String json = MessageGenerator::generateLogMessage(message, append);
  Network::broadcastMessage(json);
}

bool isMessageSameAsRecentLast(const String &message, bool append) {
  if (!append && message == lastMessage &&
      Utils::getNormailzedTime() - lastMessageTimestamp < MIN_TIME_DISTANCE_BETWEEN_SAME_MESSAGES) {
    lastMessageTimestamp = Utils::getNormailzedTime();
    return true;
  }
  lastMessageTimestamp = Utils::getNormailzedTime();
  lastMessage = message;
  return false;
}

void serialPrint(caller callingNamespace, const String &message1, const String &message2,
    const String &message3) {
#ifdef DEBUG_MODE
  String newMessage = message1 + message2 + message3;
  if (serialPrintIsNewLine) {
    newMessage = serialPrintLeadingSpaces + "(" + callerNames[callingNamespace] + ") " + message1 +
                 message2 + message3;
  }
  if (!isMessageSameAsRecentLast(newMessage, true)) {
    Serial.print(newMessage);
  }
  if (!loggingPaused) {
    loggingPaused = true;
    logHistoryEnqueue(newMessage, !serialPrintIsNewLine);
    // broadcastLog(newMessage, serialPrintIsNewLine);
    loggingPaused = false;
  }
  serialPrintIsNewLine = false;
#endif
}

void serialPrintln(caller callingNamespace, const String &message1, const String &message2,
    const String &message3) {
#ifdef DEBUG_MODE
  String newMessage = message1 + message2 + message3;
  if (serialPrintIsNewLine) {
    newMessage = serialPrintLeadingSpaces + "(" + callerNames[callingNamespace] + ") " + message1 +
                 message2 + message3;
  }
  if (!isMessageSameAsRecentLast(newMessage, false)) {
    Serial.println(newMessage);
  }
  if (!loggingPaused) {
    loggingPaused = true;
    logHistoryEnqueue(newMessage, !serialPrintIsNewLine);
    // broadcastLog(newMessage, serialPrintIsNewLine);
    loggingPaused = false;
  }
  serialPrintIsNewLine = true;
#endif
}

void serialPrintlnStart(caller callingNamespace, const String &message1, const String &message2,
    const String &message3) {
  serialPrintln(callingNamespace, message1, message2, message3);
  for (uint16_t count = 0; count < SERIAL_PRINT_NEST_SPACES; count++) {
    serialPrintLeadingSpaces = serialPrintLeadingSpaces + " ";
  }
}

void serialPrintlnEnd(caller callingNamespace, const String &message1, const String &message2,
    const String &message3) {
  int finalLength = serialPrintLeadingSpaces.length() - SERIAL_PRINT_NEST_SPACES;
  if (finalLength <= 0) {
    serialPrintLeadingSpaces = "";
  } else {
    serialPrintLeadingSpaces = serialPrintLeadingSpaces.substring(0, finalLength);
  }

  String finalMessage = message1 + message2 + message3;
  if (!finalMessage.isEmpty()) {
    serialPrintln(callingNamespace, finalMessage);
  } else if (!serialPrintIsNewLine) {
    serialPrintln(callingNamespace, finalMessage);
  }
}

}  // namespace Logs