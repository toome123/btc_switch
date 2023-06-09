#include "config.h"


namespace {
    WebSocketsClient webSocket;
    bool canBeInitialized = 0;
    String payloadStr = "";
    bool paid;
}


namespace LnbitsDevice{
    void init();
    void loop();
    String getValue(String data, char separator, int index);
    void webSocketEvent(WStype_t type, uint8_t *payload, size_t length);
}