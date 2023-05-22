#include "lnbits_device.h"

namespace LnbitsDevice
{
    void init()
    {
        canBeInitialized = Config::lnbitsIsConfigured();
        if (canBeInitialized)
        {
            
            LnbitsDeviceConfig config = Config::getLnbitsDeviceConfig();
            Serial.println(String(config.endPoint) + String(config.deviceId));
            Serial.println(config.deviceId);
            webSocket.beginSSL(config.lnbitsServer, 443, String(config.endPoint) + String(config.deviceId));
            webSocket.onEvent(webSocketEvent);
            webSocket.setReconnectInterval(1000);
        }
    }

    void loop()
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
        delay(2000);
    }

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
}