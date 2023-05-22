#include "config.h"

namespace Config
{
    void init()
    {
        // clean FS, for testing
        //SPIFFS.format();

        // read configuration from FS json
        Serial.println("mounting FS...");

        if (SPIFFS.begin())
        {
            Serial.println("mounted file system");
            if (SPIFFS.exists("/config.json"))
            {
                // file exists, reading and loading
                Serial.println("reading config file");
                File configFile = SPIFFS.open("/config.json", "r+");
                if (configFile)
                {
                    Serial.println("opened config file");
                    size_t size = configFile.size();
                    // Allocate a buffer to store contents of the file.
                    std::unique_ptr<char[]> buf(new char[size]);

                    configFile.readBytes(buf.get(), size);

#if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
                    DynamicJsonDocument json(1024);
                    auto deserializeError = deserializeJson(json, buf.get());
                    serializeJson(json, Serial);
                    if (!deserializeError)
                    {
#else
                    DynamicJsonBuffer jsonBuffer;
                    JsonObject &json = jsonBuffer.parseObject(buf.get());
                    json.printTo(Serial);
                    if (json.success())
                    {
#endif
                        Serial.println("\nparsed json");
                        strcpy(lnbitsServer, json["lnbitsServer"]);//TODO: fix this 
                        // strcpy(endPoint, json["endPoint"]);
                        strcpy(deviceId, json["deviceId"]);
                    }
                    else
                    {
                        Serial.println("failed to load json config");
                    }
                    configFile.close();
                }
            }
        }
        else
        {
            Serial.println("failed to mount FS");
        }
        // end read
        Serial.print("eminem before before test");
        Serial.print(lnbitsServer);
        Serial.print(deviceId);
        // The extra parameters to be configured (can be either global or just in the setup)
        // After connecting, parameter.getValue() will get you the configured value
        // id/name placeholder/prompt default length
        WiFiManagerParameter custom_lnbits_server("lnbitsserver", "Lnbits Server", lnbitsServer, 23);
        // WiFiManagerParameter custom_lnbits_server_endpoint("endpoint", "End Point", endPoint, 12);
        WiFiManagerParameter custom_lnbits_server_deviceId("deviceid", "Device Id", deviceId, 22);

        // WiFiManager
        // Local intialization. Once its business is done, there is no need to keep it around
        WiFiManager wifiManager;

        // set config save notify callback
        wifiManager.setSaveConfigCallback(saveConfigCallback);

        // add all your parameters here
        wifiManager.addParameter(&custom_lnbits_server);
        // wifiManager.addParameter(&custom_lnbits_server_endpoint);
        wifiManager.addParameter(&custom_lnbits_server_deviceId);

        WiFi.mode(WIFI_STA);
        pinMode(TRIGGER_PIN, INPUT_PULLUP);
        bool isConnected = wifiManager.autoConnect("Bitcoin Device", "satoshi123");
        if (isConnected)
        {
            Serial.println("Connected to WIFI");
        }
        else
        {
            Serial.println("Unable to connect to WIFI");
        }
        //TODO: fix this 
        Serial.print("eminem before test");
        Serial.print("lnbitsServer"); 
        Serial.print(lnbitsServer); 
        Serial.print("deviceId"); 
        Serial.print(deviceId); 
        strcpy(lnbitsServer, custom_lnbits_server.getValue());
        //TODO: fix this 
        // strcpy(endPoint, custom_lnbits_server_endpoint.getValue());
        strcpy(deviceId, custom_lnbits_server_deviceId.getValue());
        Serial.println("The values in the file are: ");
        Serial.println("\tLnbits Server : " + String(lnbitsServer));
        Serial.println("\tEnd Point : " + String(endPoint));
        Serial.println("\tDevice Id : " + String(deviceId));

        // save the custom parameters to FS
        if (shouldSaveConfig)
        {
            Serial.println("saving config");
#if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
            DynamicJsonDocument json(1024);
#else
            DynamicJsonBuffer jsonBuffer;
            JsonObject &json = jsonBuffer.createObject();
#endif
            json["lnbitsServer"] = lnbitsServer;
            // json["endPoint"] = endPoint;
            json["deviceId"] = deviceId;

            File configFile = SPIFFS.open("/config.json", "w+");
            if (!configFile)
            {
                Serial.println("failed to open config file for writing");
            }

#if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
            serializeJson(json, Serial);
            serializeJson(json, configFile);
#else
            json.printTo(Serial);
            json.printTo(configFile);
#endif
            configFile.close();
            // end save
        }

        Serial.println("local ip");
        Serial.println(WiFi.localIP());
        waitToStartConfigPortal();
    }

    void waitToStartConfigPortal()
    {
        Serial.println("Starting waiting for config hotspot");
        time_now = millis();
        while (millis() < time_now + configPortalTimePeriod)
        {

            if (digitalRead(TRIGGER_PIN) == LOW)
            {
                WiFiManager wifiManager;
                // reset settings - for testing
                // wifiManager.resetSettings();
                //  set configportal timeout
                wifiManager.setConfigPortalTimeout(webPortalTimeout);

                if (!wifiManager.startConfigPortal("Bitcoin Device", "satoshi123"))
                {
                    Serial.println("failed to connect and hit timeout");
                    delay(3000);
                    // reset and try again, or maybe put it to deep sleep
                    ESP.restart();
                    delay(5000);
                }
                // if you get here you have connected to the WiFi
                Serial.println("Connected to WIFI");
            }
        }
        Serial.println("Stoping waiting for config hotspot");
    }

    // callback notifying us of the need to save config
    void saveConfigCallback()
    {
        Serial.println("Should save config");
        shouldSaveConfig = true;
    }

    bool lnbitsIsConfigured()
    {
        return (*deviceId != '\0');
    }

    LnbitsDeviceConfig getLnbitsDeviceConfig()
    {
        LnbitsDeviceConfig config;
        strcpy(config.lnbitsServer, lnbitsServer);
        strcpy(config.endPoint, endPoint);
        strcpy(config.deviceId, deviceId);
        return config;
    }
}