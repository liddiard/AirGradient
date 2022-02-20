#include "user.h"

//screen refresh
const int screenUpdateFrequencyMs = 4000;

//push metrics to serverss
const int sendMetricsFrequencyMs = 30000;
const String metricsServerUrl = "http://hw.airgradient.com/";

//Id of the device for Prometheus
const char * deviceId = "";

//Wifi information
const char* ssid = "";
const char* password = "";
const uint16_t port = 80;

#ifdef staticip
IPAddress static_ip(192, 168, 42, 20);
IPAddress gateway(192, 168, 42, 1);
IPAddress subnet(255, 255, 255, 0);
#endif

const char* ntp_server = "pool.ntp.org";