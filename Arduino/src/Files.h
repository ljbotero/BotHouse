#ifndef Files_h
#define Files_h

namespace Files {
#include <ESP8266WebServer.h>

void streamFile(const String& path, const String& contentType, ESP8266WebServer &server);
void fileUpload(ESP8266WebServer &server);
void fileDelete(const String& fname, ESP8266WebServer &server);
void getFiles(ESP8266WebServer &server);
bool exists(const String& path);
void setup();

}  // namespace Files
#endif