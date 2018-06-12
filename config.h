#include <ESP8266WiFi.h>
// Update these with values suitable for your MQTT server and account

// Station
#define homeSsid      "your wifi ssid"
#define homePassword  "your wifi password"
IPAddress ip( 172, 19, 3, 69 );
IPAddress gateway( 172, 19, 3, 254 );
IPAddress subnet( 255, 255, 255, 0 );
IPAddress dns( 172, 19, 3, 251 );

// MQTT broker
#define mqtt_ssl
#define mqtt_server   "your mqtt broker"
#define mqtt_port     8883
#define mqtt_username "username"
#define mqtt_password "password"
#define mqtt_clientId "random id"
#define mqtt_topic    "stat/mailbox/trigger"

// OTA update
#define HTTP_OTA_ADDRESS      F("hostname of your OTA webserver") // Address of OTA update server
#define HTTP_OTA_PATH         F("path of your OTA script")        // Path to update firmware
#define HTTP_OTA_PORT         8123                                // Port of update server

