#ifndef BTC_SWITCH_DMOWNS_CONFIG
#define BTC_SWITCH_DMOWNS_CONFIG
#include <FS.h>
#include <Arduino.h>
#include <WiFiManager.h>
#include <WiFiClientSecure.h>
#ifdef ESP32
  #include <SPIFFS.h>
#endif
#include <ArduinoJson.h>

struct LnbitsDeviceConfig{
    char lnbitsServer[23];
    char endPoint[12];
    char deviceId[22];
};

namespace {
    int webPortalTimeout = 120;
    //Config pin button web portal
    #define TRIGGER_PIN 0
    char lnbitsServer[23];
    char endPoint[12] = "/api/v1/ws/";
    char deviceId[22];

    //flag for saving data
    bool shouldSaveConfig = false;
    int configPortalTimePeriod = 3000;
    unsigned long time_now = 0; 
}

namespace Config {
    void init();
    //callback notifying us of the need to save config
    void saveConfigCallback();
    bool lnbitsIsConfigured();
    LnbitsDeviceConfig getLnbitsDeviceConfig();
    void waitToStartConfigPortal();
}

#endif