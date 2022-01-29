#pragma once
#include <ESP8266WiFi.h>
#define HAS_BOOT_TIME


#ifdef HAS_BOOT_TIME
extern const char* ntp_server;
#endif

//screen refresh
extern const int screenUpdateFrequencyMs;

//Id of the device for Prometheus
extern const char * deviceId;

//Wifi information
extern const char* ssid;
extern const char* password;
extern const uint16_t port;

// Uncomment the line below to configure a static IP address.
//#define staticip
