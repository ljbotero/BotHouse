#ifndef CONFIGURATION_H
#define CONFIGURATION_H
#include <IPAddress.h>

// *************************************************** //
#define MIN_HEAP_TO_BE_AP  10000

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

const String CONFIDENTIAL_STRING = "**********";
const uint16_t BROADCAST_PORT = 7372;
const uint16_t SSDP_PORT = 1900;
const int32_t MINIMAL_SIGNAL_STRENGHT = -68;

const uint32_t MAX_MILLIS_TO_WAIT_TO_CONNECT_TO_AP = 45000;

////////////////////////////////////////

const uint8_t MAX_LENGTH_WIFI_NAME = 50;
const uint8_t MAX_LENGTH_WIFI_PASSWORD = 30;
const uint8_t MAX_LENGTH_DEVICE_NAME = 30;
const uint8_t MAX_LENGTH_DEVICE_TYPE_ID = 16;
const uint8_t MAX_LENGTH_HUB_NAMESPACE = 15;
const uint8_t MAX_LENGTH_HUB_API = 40;
const uint8_t MAX_LENGTH_HUB_TOKEN = 40;
//const uint8_t MAX_LENGTH_AMAZON_REFRESH_TOKEN = 400;
const uint8_t MAX_LENGTH_AMAZON_USER_ID = 45;
const uint8_t MAX_LENGTH_AMAZON_EMAIL = 50;

const IPAddress accessPointIP(192, 168, 4, 1);
const IPAddress accessPointGateway(192, 168, 4, 1);
const IPAddress accessPointSubnet(255, 255, 255, 0);

////////////////////////////////////////
// TWEAKS

#define DEBUG_MODE "verbose"
//#define FORCE_MASTER_NODE
//#define RUN_UNIT_TESTS
//#define DISABLE_OTA
//#define DISABLE_UPNP
//#define DISABLE_AP
#define DISABLE_ALEXA_SKILL

#endif
