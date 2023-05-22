#include "config.h"
#include "lnbits_device.h"

void setup()
{
    Serial.begin(115200);
    Config::init();
    LnbitsDevice::init();
}

void loop()
{
    LnbitsDevice::loop();
}
