//#define DEBUG 1
#define SERIAL_BPS 115200
#define VERSION "005"

/* settings:
 *  Generic ESP8266
 *  Flash mode QIO
 *  Flash size 1MB (no SPIFFS)
 *  */
 
// Connect RST to Vcc with a 12K resistor and
// to GND with a switch, prefereably through a
// 100nF capacitor. Program will drop an MQTT
// message and then sleep forever (as D16 and
// RST are not connected).

#include "config.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

#define RTC_OK 0
#define RTC_NOWIFI 1
#define RTC_NOMQTT 2
#define RTC_UPDATED 3
#define RTC_NOPUBLISH 4
#define RTC_OTAFAIL 5

char msgBuf [128];
uint32_t rtc;

ADC_MODE(ADC_VCC);

#ifdef mqtt_ssl
WiFiClientSecure wiFiClient;
#endif
#ifndef mqtt_ssl
WiFiClient wiFiClient;
#endif
PubSubClient pubSubClient(wiFiClient);

template <typename Generic> void debugMsgLn (Generic g) {
#ifdef DEBUG
  Serial.println(g);
#endif
}

template <typename Generic> void debugMsg (Generic g) {
#ifdef DEBUG
  Serial.print(g);
#endif
}

void callback(char* topic, byte* payload, unsigned int length) {
}

int WifiGetRssiAsQuality(int rssi)
{
  int quality = 0;

  if (rssi <= -100) {
    quality = 0;
  } else if (rssi >= -50) {
    quality = 100;
  } else {
    quality = 2 * (rssi + 100);
  }
  return quality;
}

void setup() {

  char miniBuf [10];

  Serial.begin (SERIAL_BPS);
  debugMsgLn(F(""));
  debugMsg(F("PoBox "));
  debugMsgLn(VERSION);

  // Now start the WiFi https://www.bakke.online/index.php/2017/05/22/reducing-wifi-power-consumption-on-esp8266-part-3/
  debugMsgLn(F("Initialising Wifi..."));
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepWake();
  delay( 1 );
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.config( ip, gateway, subnet, dns );

  // connect to the WiFi
  if (WiFi.status() != WL_CONNECTED) { // should always be true
    debugMsgLn(F("WiFi.begin"));
    WiFi.begin(homeSsid, homePassword);
  }
  int c = 100; // give it 10 seconds
  while (c-- && WiFi.status() != WL_CONNECTED) delay (100);

  // gracefully handle failure, record, and fall asleep
  if (WiFi.status() != WL_CONNECTED) {
    //WiFi.mode(WIFI_OFF);
    rtc = RTC_NOWIFI;                  // No wifi connection
    debugMsgLn(F("WiFi connection Failed!"));
    return;                   // quit setup, so it will enter loop and promptly fall asleep
  }
  debugMsg(F("Connected to: "));
  debugMsg(WiFi.SSID());
  debugMsg(F(", IP address: "));
  debugMsgLn(WiFi.localIP());

  // connect to the MQTT broker
  pubSubClient.setServer(mqtt_server, mqtt_port);
  pubSubClient.setCallback(callback);
  if (!pubSubClient.connect (mqtt_clientId, mqtt_username, mqtt_password)) {
    //WiFi.mode(WIFI_OFF);
    rtc = RTC_NOMQTT;                  // Connect to WiFi, but not to broker
    debugMsgLn(F("MQTT connection Failed!"));
    return;                   // quit setup, so it will enter loop and promptly fall asleep

  }
  debugMsgLn(F("MQTT connected"));

  // Report Vcc to MQTT and open JSON.
  strcpy (msgBuf, "{\"vcc\":");
  itoa(ESP.getVcc(), miniBuf, 10);
  strcat (msgBuf, miniBuf);

  // report flap status
  strcat (msgBuf, ",\"flap\":true");

  // report last status in case one or more were missed
  strcat (msgBuf, ",\"prev\":");
  ESP.rtcUserMemoryRead(0, &rtc, sizeof(rtc));
  itoa((int)rtc, miniBuf, 10);
  strcat (msgBuf, miniBuf);

  // Report RSSI to MQTT.
  strcat (msgBuf, ",\"RSSI\":\"");
  itoa((int)WifiGetRssiAsQuality(WiFi.RSSI()), miniBuf, 10);
  strcat (msgBuf, miniBuf);
  strcat (msgBuf, "\"");
  
  // Report version to MQTT.
  strcat (msgBuf, ",\"version\":\"");
  strcat (msgBuf, VERSION);
  strcat (msgBuf, "\"");

  // End JSON string.
  strcat (msgBuf, "}");

  // publish the JSON string
  rtc = RTC_OK; // assume all OK
  if (!pubSubClient.publish (mqtt_topic, msgBuf)) {; // do not retain
    rtc = RTC_NOPUBLISH; // missed a publish
  }

  // disconnect from MQTT
  delay (10);
  pubSubClient.disconnect();
}

void loop() {

  // try http ota
  if (WiFi.status() == WL_CONNECTED) {
    debugMsgLn(F("Check OTA..."));
    // now try OTA
    switch (ESPhttpUpdate.update(HTTP_OTA_ADDRESS, HTTP_OTA_PORT, HTTP_OTA_PATH, String(__FILE__).substring(String(__FILE__).lastIndexOf('/') + 1) + "." + VERSION)) {
      case HTTP_UPDATE_FAILED:
        debugMsgLn("HTTP update failed: Error " + ESPhttpUpdate.getLastErrorString() + "\n");
        rtc = RTC_OTAFAIL;
        break;
      case HTTP_UPDATE_NO_UPDATES:
        debugMsgLn(F("No updates"));
        break;
      case HTTP_UPDATE_OK:
        debugMsgLn(F("Update OK"));
        rtc = RTC_UPDATED;
        break;
    }
  }

  // save last status
  ESP.rtcUserMemoryWrite(0, &rtc, sizeof(rtc));

  delay (10);
  WiFi.mode(WIFI_OFF);
  delay (10);
  debugMsgLn(F("Zzz..."));
  ESP.deepSleep(0, WAKE_RF_DEFAULT); // one minutes, bus since GPIO16 is not connected to RST, nothing happens.
  delay (10000);
}

