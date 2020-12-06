#include <ESP8266WebServer.h>
#include <FS.h>
#include "Events.h"
#include "Logs.h"
#include "Utils.h"

namespace Files {

const Logs::caller me = Logs::caller::Files;
File fsUploadFile;

void fileUpload(ESP8266WebServer& server) {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    if (filename.length() == 0 || SPIFFS.exists(filename)) {
      server.send(500, F("text/plain"), F("500: couldn't create file"));
      return;
    }
    if (!filename.startsWith("/")) {
      filename = "/" + filename;
    }
    Logs::serialPrint(me, F("handleFileUpload Name: "));
    Logs::serialPrintln(me, filename);
    fsUploadFile = SPIFFS.open(filename, "w");
    filename = String();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile) {
      fsUploadFile.close();
      Logs::serialPrint(me, F("handleFileUpload Size: "));
      Logs::serialPrintln(me, String(upload.totalSize));
      server.sendHeader("Location", "/files", true);
      server.send ( 302, "text/plain", "");
    } else {
      server.send(500, F("text/plain"), F("500: couldn't create file"));
    }
  }
}

void fileDelete(const String& fname, ESP8266WebServer& server) {
  Logs::serialPrintln(me, F("fileDelete: "), fname);
  if (fname.length() == 0 || !SPIFFS.exists(fname)) {
    server.send(500, F("text/plain"), F("File does not exist"));
    return;
  }
  SPIFFS.remove(fname);
  server.send(200, F("text/plain"), F("Successfully removed file"));
}

void getFiles(ESP8266WebServer& server) {
  Dir dir = SPIFFS.openDir("/");
  server.sendContent(F("<ul>"));
  while (dir.next()) {
    server.sendContent(F("<li>"));
    server.sendContent(dir.fileName());
    server.sendContent(F("</li>"));
  }
  server.sendContent(F("</ul>"));
}

void streamFile(const String& path, const String& contentType, ESP8266WebServer& server) {
  File file = SPIFFS.open(path, "r");  // Open it
  if (file.size() > ESP.getFreeHeap()) {
    Logs::serialPrintln(me, F("Free heap: "), String(ESP.getFreeHeap()));
    Logs::serialPrintln(me, F("ERROR: Not enough heap memory to stream"));
    Utils::freeUpMemory();
  }
  size_t sent = server.streamFile(file, contentType);  // And send it to the client
  file.close();
  Logs::serialPrintln(me, F("Streamed "), String(sent), F("-byte file"));
}

bool exists(const String& path) {
  return SPIFFS.exists(path);
}

void setup() {
  FSConfig cfg;
  cfg.setAutoFormat(false);
  SPIFFS.setConfig(cfg);
  SPIFFS.begin();
}

}  // namespace Files