#include <FS.h>
#include <LittleFS.h>

#include <WiFiManager.h>
#include <WiFiClientSecure.h>

#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <Hash.h>

int webPortalTimeout = 120;
// Config pin button web portal
#define TRIGGER_PIN 0
String fullLnbitsServerUrl;
String lnbitsServer;
String endPoint = "/api/v1/ws/";
String deviceId;

// flag for saving data
bool shouldSaveConfig = false;
const unsigned long configPortalTimePeriod = 3000;

struct LnbitsDeviceConfig
{
    String lnbitsServer;
    String endPoint;
    String deviceId;
};

void initConfig();
// callback notifying us of the need to save config
void saveConfigCallback();
bool lnbitsIsConfigured();
LnbitsDeviceConfig getLnbitsDeviceConfig();
void waitToStartConfigPortal();

WebSocketsClient webSocket;
bool canBeInitialized;
String payloadStr = "";
bool paid;
String getValue(String data, char separator, int index);
void webSocketEvent(WStype_t type, uint8_t *payload, size_t length);

bool fileExists(const char *path)
{
    File file = LittleFS.open(path, "r");
    if (!file)
    {
        return false;
    }
    else
    {
        return true;
    }
}

void deleteFile(const char *path)
{
    Serial.printf("Deleting file: %s\n", path);
    if (LittleFS.remove(path))
    {
        Serial.println("File deleted");
    }
    else
    {
        Serial.println("Delete failed");
    }
}

void waitToStartConfigPortal()
{
    Serial.println("Starting waiting for config hotspot");
    unsigned long currentTime = millis();
    unsigned long startTime = millis();
    while (currentTime - startTime < configPortalTimePeriod)
    {
        currentTime = millis();
        delay(100);
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
    return (deviceId != NULL);
}

// LnbitsDeviceConfig getLnbitsDeviceConfig()
// {
//     LnbitsDeviceConfig config;
//     config.lnbitsServer = lnbitsServer;
//     config.deviceId = deviceId;
//     config.endPoint = endPoint;
//     return config;
// }

String getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = {0, -1};
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++)
    {
        if (data.charAt(i) == separator || i == maxIndex)
        {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i + 1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{
    switch (type)
    {
    case WStype_DISCONNECTED:
        Serial.printf("[WSc] Disconnected!\n");
        break;
    case WStype_CONNECTED:
    {
        Serial.printf("[WSc] Connected to url: %s\n", payload);

        // send message to server when Connected
        webSocket.sendTXT("Connected");
    }
    break;
    case WStype_TEXT:
        Serial.println("Message recived");
        payloadStr = (char *)payload;
        paid = true;
    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
        break;
    }
}

void setup()
{
    Serial.begin(115200);
    delay(2000);
    // clean FS, for testing
    // LittleFS.format();

    // read configuration from FS json
    Serial.println("mounting FS...");
    if (LittleFS.begin())
    {
        Serial.println("mounted file system");
        if (fileExists("/config.json"))
        {
            // file exists, reading and loading
            Serial.println("reading config file");
            File configFile = LittleFS.open("/config.json", "r");
            if (configFile)
            {
                Serial.println("opened config file");
                size_t size = configFile.size();
                // Allocate a buffer to store contents of the file.
                std::unique_ptr<char[]> buf(new char[size]);

                configFile.readBytes(buf.get(), size);
                DynamicJsonDocument json(1024);
                auto deserializeError = deserializeJson(json, buf.get());
                serializeJson(json, Serial);
                if (!deserializeError)
                {
                    Serial.println("\nparsed json");
                    fullLnbitsServerUrl = json["fullLnbitsServerUrl"].as<String>();
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

    // WiFiManager
    // Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;

    // The extra parameters to be configured (can be either global or just in the setup)
    // After connecting, parameter.getValue() will get you the configured value
    // id/name placeholder/prompt default length
    char fullLnbitsServerUrlBuff[200];
    fullLnbitsServerUrl.toCharArray(fullLnbitsServerUrlBuff, 200);
    WiFiManagerParameter custom_lnbits_server("lnbitsserver", "Lnbits Server", fullLnbitsServerUrlBuff, 200);
    // add all your parameters here
    wifiManager.addParameter(&custom_lnbits_server);

    // set config save notify callback
    wifiManager.setSaveConfigCallback(saveConfigCallback);

    wifiManager.setConnectTimeout(60);

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
    fullLnbitsServerUrl = custom_lnbits_server.getValue();
    Serial.println(fullLnbitsServerUrl);
    lnbitsServer = fullLnbitsServerUrl.substring(5, fullLnbitsServerUrl.length() - 33);
    deviceId = fullLnbitsServerUrl.substring(fullLnbitsServerUrl.length() - 22);

    // save the custom parameters to FS
    if (shouldSaveConfig)
    {
        deleteFile("/config.json");
        Serial.println("saving config");
        DynamicJsonDocument json(1024);

        json["fullLnbitsServerUrl"] = fullLnbitsServerUrl;
        File configFile = LittleFS.open("/config.json", "w");
        if (!configFile)
        {
            Serial.println("failed to open config file for writing");
        }
        serializeJson(json, Serial);
        serializeJson(json, configFile);

        delay(2000);
        configFile.close();
        // end save
    }

    Serial.println("local ip");
    Serial.println(WiFi.localIP());
    waitToStartConfigPortal();

    canBeInitialized = lnbitsIsConfigured();
    if (canBeInitialized)
    {
        Serial.println("Init websocket");
        Serial.println(lnbitsServer);
        Serial.println(endPoint);
        Serial.println(deviceId);
        String test = String(endPoint) + String(deviceId);
        Serial.println(test);
        webSocket.beginSSL(lnbitsServer.c_str(), 443, test.c_str());
        webSocket.onEvent(webSocketEvent);
        webSocket.setReconnectInterval(1000);
        Serial.println("Init websocket complete");
    }
}

void loop()
{
    payloadStr = "";
    delay(200);
    canBeInitialized = lnbitsIsConfigured();
    if (canBeInitialized)
    {
        while (paid == false)
        {   
            webSocket.loop();
            if (paid)
            {
                Serial.println("Paid");
                pinMode(getValue(payloadStr, '-', 0).toInt(), OUTPUT);
                digitalWrite(getValue(payloadStr, '-', 0).toInt(), HIGH);
                delay(getValue(payloadStr, '-', 1).toInt());
                digitalWrite(getValue(payloadStr, '-', 0).toInt(), LOW);
                paid = false;
            }
        }
    }
}
