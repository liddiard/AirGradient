/**
 * This sketch connects an AirGradient DIY sensor to a WiFi network, and runs a
 * tiny HTTP server to serve air quality metrics to Prometheus.
 */

#include "main.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>

#include <Wire.h>
#include "SSD1306Wire.h"
#include "Configuration/user.h"
#include "Metrics/MetricGatherer.h"
#include "Sensors/Particle/PMSXSensor.h"
#include "Sensors/Temperature/SHTXSensor.h"
#include "Sensors/CO2/SensairS8Sensor.h"
#include "Sensors/Time/BootTimeSensor.h"
#include "AQI/AQICalculator.h"
#include "Prometheus/PrometheusServer.h"

using namespace AirGradient_Internal;

// Config ----------------------------------------------------------------------

// For housekeeping.
uint8_t counter = 0;

// Config End ------------------------------------------------------------------

SSD1306Wire display(0x3c, SDA, SCL);

auto metrics = std::make_shared<MetricGatherer>(-2);
auto aqiCalculator = std::make_shared<AQICalculator>(metrics);
auto server = std::make_unique<PrometheusServer>(port, deviceId, metrics, aqiCalculator);
Ticker updateScreenTicker;
Ticker sendMetricsTicker;


void setup() {
    Serial.begin(9600);
    
    metrics->addSensor(std::make_unique<PMSXSensor>())
            .addSensor(std::make_unique<SHTXSensor>())
            .addSensor(std::make_unique<SensairS8Sensor>())
            .addSensor(std::make_unique<BootTimeSensor>(ntp_server));

    // Init Display.
    display.init();
    display.flipScreenVertically();
    showTextRectangle("Init", String(EspClass::getChipId(), HEX), true);

    // Set static IP address if configured.
#ifdef staticip
    WiFi.config(static_ip, gateway, subnet);
#endif

    // Set WiFi mode to client (without this it may try to act as an AP).
    WiFi.mode(WIFI_STA);

    // Configure Hostname
    if ((deviceId != NULL) && (deviceId[0] == '\0')) {
        Serial.printf("No Device ID is Defined, Defaulting to board defaults");
    } else {
        wifi_station_set_hostname(deviceId);
        WiFi.setHostname(deviceId);
    }

    // Setup and wait for WiFi.
    WiFi.begin(ssid, password);
    Serial.println("");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        showTextRectangle("Trying to", "connect...", true);
        Serial.print(".");
    }

    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("MAC address: ");
    Serial.println(WiFi.macAddress());
    Serial.print("Hostname: ");
    Serial.println(WiFi.hostname());

    metrics->begin();
    aqiCalculator->begin();

    server->begin();

    showTextRectangle("Listening To", WiFi.localIP().toString() + ":" + String(port), true);
    updateScreenTicker.attach_ms_scheduled(screenUpdateFrequencyMs, updateScreen);
    sendMetricsTicker.attach_ms_scheduled(sendMetricsFrequencyMs, sendMetrics);
}

void loop() {
    server->handleRequests();
}

// DISPLAY
void showTextRectangle(const String &ln1, const String &ln2, boolean small) {
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    if (small) {
        display.setFont(ArialMT_Plain_16);
    } else {
        display.setFont(ArialMT_Plain_24);
    }
    display.drawString(32, 16, ln1);
    display.drawString(32, 36, ln2);
    display.display();
}

void updateScreen() {
    auto data = metrics->getData();
    auto sensorType = metrics->getMeasurements();
    // Take a measurement at a fixed interval.
    switch (counter) {

        case 0:
            if (!(sensorType & Measurement::Particle)) {
                showTextRectangle("PM2", String(data.PARTICLE_DATA.PM_2_5), false);
                break;
            }

        case 1:
            if (!(sensorType & Measurement::CO2)) {
                showTextRectangle("CO2", String(data.GAS_DATA.CO2), false);
                break;
            }

        case 2:
            if (!(sensorType & Measurement::Temperature)) {
                showTextRectangle("TMP", String(data.TMP, 1) + "C", false);
                break;
            }

        case 3:
            if (!(sensorType & Measurement::Humidity)) {
                showTextRectangle("HUM", String(data.HUM, 1) + "%", false);
                break;
            }
    }

    counter = ++counter % 4;
}

// push sensor data to a webserver
void sendMetrics() {
    auto data = metrics->getData();
    String payload = 
        "{\"wifi\":" + String(WiFi.RSSI())               + "," +
        "\"pm02\":"  + String(data.PARTICLE_DATA.PM_2_5) + "," +
        "\"rco2\":"  + String(data.GAS_DATA.CO2)         + "," +
        "\"atmp\":"  + String(data.TMP)                  + "," +
        "\"rhum\":"  + String(data.HUM)                  + "}";

    // send payload
    Serial.println(payload);
    String POSTURL = metricsServerUrl + "sensors/airgradient:" + String(ESP.getChipId(), HEX) + "/measures";
    Serial.println(POSTURL);
    WiFiClient client;
    HTTPClient http;
    http.begin(client, POSTURL);
    http.addHeader("content-type", "application/json");
    int httpCode = http.POST(payload);
    String response = http.getString();
    Serial.println(httpCode);
    Serial.println(response);
    http.end();
}