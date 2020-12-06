#ifndef WebServer_h
#define WebServer_h

#include <ESP8266WebServer.h>
#include "Config.h"

namespace WebServer {

ESP8266WebServer &getServer();
void setup();
void stop();
void handle();

}  // namespace Server
#endif
