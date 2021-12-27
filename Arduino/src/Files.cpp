#include <ESP8266WebServer.h>
#include <FS.h>
#include "Events.h"
#include "Logs.h"
#include "Utils.h"

namespace Files {

static const Logs::caller me = Logs::caller::Files;
File fsUploadFile;

void ICACHE_FLASH_ATTR fileUpload(ESP8266WebServer& server) {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    if (filename.length() == 0) {
      server.send(500, F("text/plain"), F("500: Invalid file name"));
      return;
    }
    if (!filename.startsWith("/")) {
      filename = String(FPSTR("/")) + filename;
    }
    Logs::serialPrint(me, PSTR("handleFileUpload Name: "));
    Logs::serialPrintln(me, filename.c_str());
    fsUploadFile = SPIFFS.open(filename, "w");
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile) {
      fsUploadFile.close();
      Logs::serialPrint(me, PSTR("handleFileUpload Size: "));
      Logs::serialPrintln(me, String(upload.totalSize).c_str());
      server.sendHeader("Location", "/files", true);
      server.send(302, "text/plain", "");
    } else {
      fsUploadFile.close();
      server.send(500, F("text/plain"), F("500: couldn't create file"));
    }
  }
}

void ICACHE_FLASH_ATTR fileDelete(const String& fname, ESP8266WebServer& server) {
  Logs::serialPrintln(me, PSTR("fileDelete: "), fname.c_str());
  if (fname.length() == 0 || !SPIFFS.exists(fname)) {
    server.send(500, F("text/plain"), F("File does not exist"));
    return;
  }
  SPIFFS.remove(fname);
  server.send(200, F("text/plain"), F("Successfully removed file"));
}

void ICACHE_FLASH_ATTR getFiles(ESP8266WebServer& server) {
  Dir dir = SPIFFS.openDir("/");
  server.sendContent(F("<ul>"));
  while (dir.next()) {
    server.sendContent(F("<li>"));
    server.sendContent(dir.fileName());
    server.sendContent(F("</li>"));
  }
  server.sendContent(F("</ul>"));
}

void ICACHE_FLASH_ATTR streamFile(
    const String& path, const String& contentType, ESP8266WebServer& server) {
  File file = SPIFFS.open(path, "r");  // Open it
  if (file.size() > ESP.getFreeHeap()) {
    Logs::serialPrintln(me, PSTR("Free heap: "), String(ESP.getFreeHeap()).c_str());
    Logs::serialPrintln(me, PSTR("[ERROR]  Not enough heap memory to stream"));
    Utils::freeUpMemory();
  }
  server.sendHeader(F("Cache-Control"), F("public, max-age=3600, immutable"), true);
  server.sendHeader(F("Connection"), F("close"), false);
  size_t sent = server.streamFile(file, contentType);  // And send it to the client
  file.close();  
  Logs::serialPrintln(me, PSTR("Streamed "), String(sent).c_str(), PSTR("-byte file"));
}

bool exists(const String& path) {
  return SPIFFS.exists(path);
}

void ICACHE_FLASH_ATTR setup() {
  FSConfig cfg;
  cfg.setAutoFormat(false);
  SPIFFS.setConfig(cfg);
  SPIFFS.begin();
}

}  // namespace Files