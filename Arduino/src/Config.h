#ifndef CONFIGURATION_H
#define CONFIGURATION_H
#include <ArduinoJson.h>
#include <IPAddress.h>
#include <TypeConversionFunctions.h>

// *************************************************** //
#define MIN_HEAP_TO_BE_AP 10000

// wifi
#define SERVER_PORT 80
#define RANDOM_VARIANCE_MILLIS_FROM 1000
#define RANDOM_VARIANCE_MILLIS_UPTO 15000

#define HUB_REFRESH_FREQUENCY_SECS 30 * 60
#define MAX_HTTP_RESPONSE_SIZE 512

#define MESH_CHANNEL 9
#define MESH_SSID_NAME "BotHouse"
#define MESH_MAX_CLIENTS 8

#define MDNS_NAME "bot"

//#define DEBUG_ESP_MDNS_RESPONDER
//#define DEBUG_ESP_MDNS
static const auto MILLIS_TO_RUN_SAFE_MODE_FOR = 5 * 60 * 1000;

static const char CONFIDENTIAL_STRING[] PROGMEM = "**********";
static const auto BROADCAST_PORT = 7372;
static const auto SSDP_PORT = 1900;
static const auto MINIMAL_SIGNAL_STRENGHT = -67;

static const auto MAX_MILLIS_TO_WAIT_TO_CONNECT_TO_AP = 45000;
static const auto LOWEST_MEMORY_POSSIBLE_BEFORE_REBOOT = 1024 * 6;

////////////////////////////////////////

static const auto MAX_LENGTH_VALUES = 150;
static const auto MAX_LENGTH_UUID = 50;
static const auto MAX_LENGTH_ACTION = 15;
static const auto MAX_LENGTH_COMMAND_NAME = 25;
static const auto MAX_LENGTH_DEVICE_ID = 8;
static const auto MAX_LENGTH_DEVICE_NAME = 30;
static const auto MAX_LENGTH_DEVICE_TYPE_ID = 16;
static const auto MAX_LENGTH_EVENT_NAME = 10;
static const auto MAX_LENGTH_IP = 15;
static const auto MAX_LENGTH_SSID = 32;
static const auto MAX_LENGTH_HUB_NAMESPACE = 15;
static const auto MAX_LENGTH_HUB_API = 40;
static const auto MAX_LENGTH_HUB_TOKEN = 40;
static const auto MAX_LENGTH_WIFI_NAME = 50;
static const auto MAX_LENGTH_WIFI_PASSWORD = 30;

static const IPAddress accessPointIP(192, 168, 4, 1);
static const IPAddress accessPointGateway(192, 168, 4, 1);
static const IPAddress accessPointSubnet(255, 255, 255, 0);

#if defined(ARDUINO_ARCH_ESP32)
static const String chipId = uint64ToString(ESP.getEfuseMac());
#else
static const String chipId = uint64ToString(ESP.getChipId());
#endif

////////////////////////////////////////
// TWEAKS

//#define DEBUG_MODE "verbose"
//#define DISABLE_SAFE_MODE
//#define FORCE_MASTER_NODE
//#define DISABLE_WEBSERVER
//#define RUN_UNIT_TESTS
//#define DISABLE_OTA
//#define DISABLE_UPNP
//#define DISABLE_MDNS
//#define DISABLE_HUBS
#define DISABLE_AP
//#define DISABLE_MESH
//#define ENABLE_NAT_ROUTER
#define ENABLE_REMOTE_LOGGING

#endif
