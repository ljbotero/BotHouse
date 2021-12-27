#ifndef UniversalPlugAndPlay_h
#define UniversalPlugAndPlay_h
#include <ESP8266WiFi.h>

#ifndef DISABLE_UPNP
#define SSDP_MULTICAST_ADDR 239, 255, 255, 250
#define SSDP_INTERVAL 1200
#define SSDP_UUID_SIZE 42
#define SSDP_SCHEMA_URL_SIZE 64
#define SSDP_DEVICE_TYPE_SIZE 64
#define SSDP_FRIENDLY_NAME_SIZE 64
#define SSDP_SERIAL_NUMBER_SIZE 37
#define SSDP_PRESENTATION_URL_SIZE 128
#define SSDP_MODEL_NAME_SIZE 64
#define SSDP_MODEL_URL_SIZE 128
#define SSDP_MODEL_VERSION_SIZE 32
#define SSDP_MANUFACTURER_SIZE 64
#define SSDP_MANUFACTURER_URL_SIZE 128
#define SDP_SERVICE_TYPE_SIZE 32
#define SDP_SERVICE_ID_SIZE 32
#define SDP_SCPDURL_SIZE 64
#define SDP_CONTROL_URL_SIZE 64
#define SDP_EVENT_SUB_URL_SIZE 64
#endif

namespace UniversalPlugAndPlay {
char *getUUID(const char *deviceId, uint8_t deviceIndex);
//#ifndef ARDUINO_ESP8266_GENERIC
void setup();
void handle();
void stop();
void handleDescription(const String &ssidDeviceType);
void handleEvent(const String &ssidDeviceType);

static const char DEVICE_TYPE_LOCALBOT[] PROGMEM =
    "urn:schemas-upnp-org:device:bothouse";  //"upnp:rootdevice"
static const char SSDP_DESCRIPTION_PATH[] PROGMEM = "/description.xml";
#ifndef DISABLE_UPNP
static const char ST_SSDP_DEVICE_TYPE[] PROGMEM = "ST: urn:schemas-upnp-org:device:bothouse";
//static const char SSDP_DEVICE_NAME[] PROGMEM = "Bot House Mesh";
static const char SSDP_MODEL_NAME[] PROGMEM = "Bot House 2020";
static const char SSDP_MANUFACTURER[] PROGMEM = "Bot House 2020";
static const char SSDP_URL[] PROGMEM = "http://bot.local";
static const char SSDP_ROOT[] PROGMEM = "urn:schemas-upnp-org:device-1-0";

static const char ST_SSDP_ALL[] PROGMEM = "ST: ssdp:all";

// http://upnp.org/specs/arch/UPnP-arch-DeviceArchitecture-v1.1.pdf
static const char _ssdp_response_template[] PROGMEM =
    "HTTP/1.1 200 OK\r\n"
    "EXT:\r\n"
    "CACHE-CONTROL: max-age=%u\r\n"  // SSDP_INTERVAL
    "SERVER: Arduino/1.0 UPNP/1.1 %s/%s\r\n"  // _modelName, _modelNumber
    "USN: %s\r\n"  // _uuid
    "ST: %s\r\n"  // "NT" or "ST", _deviceType
    "LOCATION: http://%s:%u/%s\r\n"  // WiFi.localIP(), _port, _schemaURL
    "\r\n";

static const char _ssdp_schema_template[] PROGMEM =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/xml\r\n"
    "Connection: close\r\n"
    "Access-Control-Allow-Origin: *\r\n\r\n"
    "<?xml version=\"1.0\"?><root xmlns=\"%s\">"
    "<specVersion><major>1</major><minor>0</minor></specVersion>"
    "<URLBase>http://%s:%u/</URLBase>"  // WiFi.localIP(), _port
    "<device>"
    "<deviceType>%s</deviceType>"
    "<deviceId>%s</deviceId>"
    "<deviceIndex>%s</deviceIndex>"
    "<friendlyName>%s</friendlyName>"
    "<presentationURL>%s</presentationURL>"
    "<serialNumber>%s</serialNumber>"
    "<modelName>%s</modelName>"
    "<modelNumber>%s</modelNumber>"
    "<modelURL>%s</modelURL>"
    "<manufacturer>%s</manufacturer>"
    "<manufacturerURL>%s</manufacturerURL>"
    "<UDN>%s</UDN>"
    "<serviceList>"
    "<service>"
    "<serviceType>urn:schemas-upnp-org:service:%s:1</serviceType>"
    "<serviceId>urn:upnp-org:serviceId:%s:1</serviceId>"
    "<SCPDURL>/upnp/%s.xml</SCPDURL>"
    "<controlURL>/upnp-control/%s.xml</controlURL>"
    "<eventSubURL>/upnp-eventing/%s.xml</eventSubURL>"
    "</service>"
    "</serviceList>"
    "</device>"
    "</root>\r\n"
    "\r\n";
#endif
}  // namespace UniversalPlugAndPlay
#endif

// static const char _ssdp_notify_template[] PROGMEM = "NOTIFY * HTTP/1.1\r\n"
//                                                     "HOST: 239.255.255.246:7900\r\n"
//                                                     "CONTENT-TYPE: text/xml\r\n"
//                                                     "CONTENT-LENGTH: %u"
//                                                     "NT: upnp:event\r\n"
//                                                     "NTS: upnp:propchange\r\n"
//                                                     "SID: uuid:subscription-UUID\r\n"
//                                                     "SEQ: %s\r\n\r\n"
//                                                     "<?xml version=\"1.0\"?>"
//                                                     "<e:propertyset
//                                                     xmlns:e=\"urn:schemas-upnp-org:event-1-0\">"
//                                                     "<e:property>"
//                                                     "<%s>new value</variableName>" //
//                                                     variableName
//                                                     "</e:property>"
//                                                     "</e:propertyset>"
//                                                     "\r\n";

// void eventNotify(const String &propertyName, const String &propertyValue, const IPAddress
// &senderIP,
//                  const uint16_t senderPort);

// static const char DEVICE_TYPE_WEMO[] PROGMEM = "urn:Belkin:device:controllee:1";
// static const char ST_SSDP_DEVICE_TYPE_WEMO[] PROGMEM = "ST: urn:Belkin:device:**";
// static const char SSDP_DEVICE_TYPE_WEMO[] PROGMEM = "urn:Belkin:device:**";
// static const char SSDP_DESCRIPTION_PATH_WEMO[] PROGMEM = "/wemo.xml";
// static const char UPNO_BASIC_EVENT_PATH_WEMO[] PROGMEM = "/upnp/control/basicevent1";
// static const char SSDP_ROOT_WEMO[] PROGMEM = "urn:Belkin:device-1-0";

// static const char _ssdp_response_template_wemo[] PROGMEM =
//     "HTTP/1.1 200 OK\r\n"
//     "CACHE-CONTROL: max-age=86400\r\n"
//     "EXT:\r\n"
//     "OPT: \"http://schemas.upnp.org/upnp/1/0/\"; ns=01\r\n"
//     "01-NLS: %s\r\n"
//     "USN: %s::urn:Belkin:device:**\r\n"
//     "ST: urn:Belkin:device:**\r\n"
//     "LOCATION: http://%s:%u/%s\r\n"
//     "\r\n";

// static const char _upnp_response_state_response_template_wemo[] PROGMEM =
//     "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
//     "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n"
//     "<s:Body>\r\n"
//     " <u:GetBinaryStateResponse xmlns:u=\"urn:Belkin:service:basicevent:1\">\r\n"
//     "  <BinaryState>%u</BinaryState>\r\n"
//     " </u:GetBinaryStateResponse>\r\n"
//     "</s:Body>\r\n"
//     "</s:Envelope>\r\n"
//     "\r\n";
// static const char _ssdp_schema_template_wemo[] PROGMEM =
//     "HTTP/1.1 200 OK\r\n"
//     "Content-Type: text/xml\r\n"
//     "Connection: close\r\n"
//     "Access-Control-Allow-Origin: *\r\n\r\n"
//     "<?xml version=\"1.0\"?><root xmlns=\"%s\">"
//     "<specVersion><major>1</major><minor>0</minor></specVersion>"
//     "<URLBase>http://%s:%u/</URLBase>"  // WiFi.localIP(), _port
//     "<device>"
//     "<deviceType>%s</deviceType>"
//     "<deviceId>%s</deviceId>"
//     "<deviceIndex>%s</deviceIndex>"
//     "<friendlyName>%s</friendlyName>"
//     "<presentationURL>%s</presentationURL>"
//     "<serialNumber>%s</serialNumber>"
//     "<modelName>%s</modelName>"
//     "<modelNumber>%s</modelNumber>"
//     "<modelURL>%s</modelURL>"
//     "<manufacturer>%s</manufacturer>"
//     "<manufacturerURL>%s</manufacturerURL>"
//     "<UDN>%s</UDN>"
//     "<serviceList>"
//     " <service>"
//     "  <serviceType>urn:Belkin:service:basicevent:1</serviceType>"
//     "  <serviceId>urn:Belkin:serviceId:basicevent1</serviceId>"
//     "  <controlURL>/%s/upnp/control/basicevent1</controlURL>"
//     "  <eventSubURL>/%s/upnp/event/basicevent1</eventSubURL>"
//     "  <SCPDURL>/%s/eventservice.xml</SCPDURL>"
//     " </service>"
//     "</serviceList>"
//     "</device>"
//     "</root>\r\n"
//     "\r\n";
