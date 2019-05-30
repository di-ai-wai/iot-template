/***********************************************************************************
 * Template for IOT-Devices
 * 
 * using MQTT for message transmission
 * OTA-Update possibility activated by an MQTT message
 * standard system messages also transmitted voa MQTT
 * every device uses its own esp-device id for identificatuon purpose
 * 
 * for passwords, IPs and sensor-gpios please use the seperate config-file
 * 
 * 
  Libraries :
    - ESP8266 core for Arduino :  https://github.com/esp8266/Arduino
    - ESP8266httpUpdate:          auto update esp8266 sketch
    - PubSubClient:               https://github.com/knolleary/pubsubclient

************************************************************************************
*/

#include <ESP8266WiFi.h>        // https://github.com/esp8266/Arduino
#include <ESP8266httpUpdate.h>  // ota update for esp8266
#include <PubSubClient.h>       // https://github.com/knolleary/pubsubclient/releases/tag/v2.6
#include "config.h"             // local config

// macros for debugging
#ifdef DEBUG
  #define         DEBUG_PRINT(x)    Serial.print(x)
  #define         DEBUG_PRINTLN(x)  Serial.println(x)
#else
  #define         DEBUG_PRINT(x)
  #define         DEBUG_PRINTLN(x)
#endif

#define           CONNECTED                 true
#define           NOT_CONNECTED             false

// Board properties
#define           FW_VERSION                "<version>"
#define           ALIAS                     "<iot-device>"
#define           LOCATION                  "<location>"
#define           CHIP                      "<chip-type>"

// MQTT
#define           MQTT_ON_PAYLOAD           "1"
#define           MQTT_OFF_PAYLOAD          "0"
#define           MQTT_ENDPOINT_SIZE        30

#define           MQTT_TOPIC_BASE           ""
#define           MQTT_TOPIC_REGISTRY       "registry"
#define           MQTT_TOPIC_SYSTEM_VERSION "system/version"
#define           MQTT_TOPIC_SYSTEM_ALIAS   "system/alias"
#define           MQTT_TOPIC_SYSTEM_CHIP    "system/chip"
#define           MQTT_TOPIC_SYSTEM_LOC     "system/location"
#define           MQTT_TOPIC_SYSTEM_UPDATE  "system/update"
#define           MQTT_TOPIC_SYSTEM_RESET   "system/reset"
#define           MQTT_TOPIC_SYSTEM_ONLINE  "system/online"

#define           MQTT_TOPIC_1              "topic/1"
#define           MQTT_TOPIC_2              "topic/2"

char              MQTT_CLIENT_ID[9]                               = {0};
char              MQTT_CHIP_VERSION[30]                           = {0};    
char              MQTT_ENDPOINT_REGISTRY[MQTT_ENDPOINT_SIZE]      = {0};
char              MQTT_ENDPOINT_SYS_VERSION[MQTT_ENDPOINT_SIZE]   = {0};
char              MQTT_ENDPOINT_SYS_ALIAS[MQTT_ENDPOINT_SIZE]     = {0};
char              MQTT_ENDPOINT_SYS_CHIP[MQTT_ENDPOINT_SIZE]      = {0};
char              MQTT_ENDPOINT_SYS_LOCATION[MQTT_ENDPOINT_SIZE]  = {0};
char              MQTT_ENDPOINT_SYS_UPDATE[MQTT_ENDPOINT_SIZE]    = {0};
char              MQTT_ENDPOINT_SYS_RESET[MQTT_ENDPOINT_SIZE]     = {0};
char              MQTT_ENDPOINT_SYS_ONLINE[MQTT_ENDPOINT_SIZE]    = {0};

char              MQTT_ENDPOINT_TOPIC_1[MQTT_ENDPOINT_SIZE]       = {0};
char              MQTT_ENDPOINT_TOPIC_2[MQTT_ENDPOINT_SIZE]       = {0};

char              MQTT_ENDPOINT_POWER[MQTT_ENDPOINT_SIZE]          = {0};


#ifdef TLS
WiFiClientSecure  wifiClient;
#else
WiFiClient        wifiClient;
#endif
PubSubClient      mqttClient(wifiClient);




// PREDEFINED FUNCTIONS
void reconnect();
void updateFW(char* fwUrl);
void reset();
void publishData(char* topic, char* payload);
void publishState(char* topic, int state);


///////////////////////////////////////////////////////////////////////////
//   MQTT with SSL/TLS
///////////////////////////////////////////////////////////////////////////
/*
  Function called to verify the fingerprint of the MQTT server certificate
 */
#ifdef TLS
void verifyFingerprint() {
  DEBUG_PRINT(F("INFO: Connecting to "));
  DEBUG_PRINTLN(MQTT_SERVER);

  if (!wifiClient.connect(MQTT_SERVER, atoi(MQTT_PORT))) {
    DEBUG_PRINTLN(F("ERROR: Connection failed. Halting execution"));
    reset();
  }

  if (wifiClient.verify(MQTT_FINGERPRINT, MQTT_SERVER)) {
    DEBUG_PRINTLN(F("INFO: Connection secure"));
  } else {
    DEBUG_PRINTLN(F("ERROR: Connection insecure! Halting execution"));
    reset();
  }
}
#endif

///////////////////////////////////////////////////////////////////////////
//   MQTT
///////////////////////////////////////////////////////////////////////////
/*
   Function called when a MQTT message arrived
   @param topic   The topic of the MQTT message
   @param _payload The payload of the MQTT message
   @param length  The length of the payload
*/
void callback(char* topic, byte* _payload, unsigned int length) {
  // handle the MQTT topic of the received message
  _payload[length] = '\0';
  char* payload = (char*) _payload;
  if (strcmp(topic, MQTT_ENDPOINT_TOPIC_2)==0) {
    if (strcmp(payload, MQTT_ON_PAYLOAD)==0) {
      DEBUG_PRINTLN(F("Info: topic 2"));
      // Do what you need
    }
  } else if (strcmp(topic, MQTT_ENDPOINT_SYS_RESET)==0) {
    if (strcmp(payload, MQTT_ON_PAYLOAD)==0) {
      publishState(MQTT_ENDPOINT_SYS_RESET, LOW);
      reset();
    }
  } else if (strcmp(topic, MQTT_ENDPOINT_SYS_UPDATE)==0) {
    if (length > 0) {
      updateFW(payload);
    }
  }
}

/*
  Function called to pulish payload data
*/
void publishData(char* topic, char* payload) {
  if (mqttClient.publish(topic, payload, true)) {
    DEBUG_PRINT(F("INFO: MQTT message publish succeeded. Topic: "));
    DEBUG_PRINT(topic);
    DEBUG_PRINT(F(" Payload: "));
    DEBUG_PRINTLN(payload);
  } else {
    DEBUG_PRINTLN(F("ERROR: MQTT message publish failed, either connection lost, or message too large"));
  }
}

/*
  Function called to publish a state
*/
void publishState(char* topic, int state) {
  publishData(topic, (char*) (state == 1 ? MQTT_ON_PAYLOAD : MQTT_OFF_PAYLOAD));
}

/*
  Function called to connect/reconnect to the MQTT broker
 */
void reconnect() {
  // test if the module has an IP address
  // if not, restart the module
  if (WiFi.status() != WL_CONNECTED) {
    DEBUG_PRINTLN(F("ERROR: The module isn't connected to the internet"));
    reset();
  }

  // try to connect to the MQTT broker
  while (!mqttClient.connected()) {
    if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS, MQTT_ENDPOINT_SYS_ONLINE, 0, 1, "0")) {
      DEBUG_PRINTLN(F("INFO: The client is successfully connected to the MQTT broker"));
    } else {
      DEBUG_PRINTLN(F("ERROR: The connection to the MQTT broker failed"));
      delay(1000);
    }
  }

  mqttClient.subscribe(MQTT_ENDPOINT_SYS_RESET);
  mqttClient.subscribe(MQTT_ENDPOINT_SYS_UPDATE);

  mqttClient.subscribe(MQTT_TOPIC_1);
}

///////////////////////////////////////////////////////////////////////////
//   SYSTEM
///////////////////////////////////////////////////////////////////////////
/*
 Function called to update chip firmware
 */
void updateFW(char* fwUrl) {
  DEBUG_PRINTLN(F("INFO: updating firmware ..."));
  t_httpUpdate_return ret = ESPhttpUpdate.update(fwUrl);
#ifdef DEBUG
  switch (ret) {
    case HTTP_UPDATE_FAILED:
      DEBUG_PRINT(F("ERROR: firmware update failed: "));
      DEBUG_PRINTLN(ESPhttpUpdate.getLastErrorString().c_str());
      break;
    case HTTP_UPDATE_NO_UPDATES:
      DEBUG_PRINTLN(F("ERROR: no new firmware"));
      break;
    case HTTP_UPDATE_OK:
      DEBUG_PRINTLN(F("Info: firmware update ok"));
      break;
  }
#endif
}

/*
  Function called to reset / restart the switch
 */
void reset() {
  DEBUG_PRINTLN(F("INFO: Reset..."));
  ESP.reset();
  delay(1000);
}

/*
  Function called to connect to Wifi
 */
void connectWiFi() {
  // Connect to WiFi network
  DEBUG_PRINT(F("INFO: Connecting to "));
  DEBUG_PRINTLN(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
  }
  DEBUG_PRINTLN(F(""));
  DEBUG_PRINTLN(F("INFO: WiFi connected"));
}


///////////////////////////////////////////////////////////////////////////
//   Setup() and loop()
///////////////////////////////////////////////////////////////////////////
void setup() {
#ifdef DEBUG
  Serial.begin(115200);
#endif
  DEBUG_PRINTLN(F(""));
  DEBUG_PRINTLN(F(""));
  DEBUG_PRINTLN(F("Info: booted"));

  //YOUR code

  // connect to wifi
  connectWiFi();

  // get the Chip ID of the switch and use it as the MQTT client ID
  sprintf(MQTT_CLIENT_ID, "%06X", ESP.getChipId());
  DEBUG_PRINT(F("INFO: MQTT client ID/Hostname: "));
  DEBUG_PRINTLN(MQTT_CLIENT_ID);
  DEBUG_PRINT(F("INFO: CHIP: "));
  DEBUG_PRINTLN(ESP.getFullVersion());

  // create MQTT endpoints
  sprintf(MQTT_ENDPOINT_REGISTRY, "%s%s", MQTT_TOPIC_BASE, MQTT_TOPIC_REGISTRY);

  sprintf(MQTT_ENDPOINT_SYS_VERSION, "%s%s/%s", MQTT_TOPIC_BASE, MQTT_CLIENT_ID, MQTT_TOPIC_SYSTEM_VERSION);
  sprintf(MQTT_ENDPOINT_SYS_ALIAS, "%s%s/%s", MQTT_TOPIC_BASE, MQTT_CLIENT_ID, MQTT_TOPIC_SYSTEM_ALIAS);
  sprintf(MQTT_ENDPOINT_SYS_CHIP, "%s%s/%s", MQTT_TOPIC_BASE, MQTT_CLIENT_ID, MQTT_TOPIC_SYSTEM_CHIP);
  sprintf(MQTT_ENDPOINT_SYS_LOCATION, "%s%s/%s", MQTT_TOPIC_BASE, MQTT_CLIENT_ID, MQTT_TOPIC_SYSTEM_LOC);
  sprintf(MQTT_ENDPOINT_SYS_UPDATE, "%s%s/%s", MQTT_TOPIC_BASE, MQTT_CLIENT_ID, MQTT_TOPIC_SYSTEM_UPDATE);
  sprintf(MQTT_ENDPOINT_SYS_RESET, "%s%s/%s", MQTT_TOPIC_BASE, MQTT_CLIENT_ID, MQTT_TOPIC_SYSTEM_RESET);
  sprintf(MQTT_ENDPOINT_SYS_ONLINE, "%s%s/%s", MQTT_TOPIC_BASE, MQTT_CLIENT_ID, MQTT_TOPIC_SYSTEM_ONLINE);

  sprintf(MQTT_ENDPOINT_TOPIC_1, "%s%s/%s", MQTT_TOPIC_BASE, MQTT_CLIENT_ID, MQTT_TOPIC_1);
  sprintf(MQTT_ENDPOINT_TOPIC_2, "%s%s/%s", MQTT_TOPIC_BASE, MQTT_CLIENT_ID, MQTT_TOPIC_2);


#ifdef TLS
  // check the fingerprint of io.adafruit.com's SSL cert
  verifyFingerprint();
#endif

  // configure MQTT
  mqttClient.setServer(MQTT_SERVER, atoi(MQTT_PORT));
  mqttClient.setCallback(callback);

  // connect to the MQTT broker
  reconnect();

  // register device
  publishData(MQTT_ENDPOINT_REGISTRY, (char *)MQTT_CLIENT_ID);

  // publish running firmware version
  publishData(MQTT_ENDPOINT_SYS_VERSION, (char*) FW_VERSION);
  publishData(MQTT_ENDPOINT_SYS_ALIAS, (char*) ALIAS);
  publishData(MQTT_ENDPOINT_SYS_CHIP, (char *)CHIP);
  publishData(MQTT_ENDPOINT_SYS_LOCATION, (char*) LOCATION);
  publishState(MQTT_ENDPOINT_SYS_ONLINE, 1);

  // YOUR code

  DEBUG_PRINTLN(F("SETUP complete."));
}


void loop() {
  // keep the MQTT client connected to the broker
  if (!mqttClient.connected()) {
    reconnect();
  }
  mqttClient.loop();

  yield();

  // Your code
}
