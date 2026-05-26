#include <Wire.h>

void setup()
{
    Serial.begin(9600);
    Wire.begin();
    delay(100);
    Serial.println(F("\nI2C Scanner"));
}

void loop()
{
    byte error, address;
    int nDevices = 0;

    Serial.println(F("Scanning..."));

    for (address = 1; address < 127; address++)
    {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0)
        {
            Serial.print(F("I2C device found at address 0x"));
            if (address < 16)
                Serial.print('0');
            Serial.print(address, HEX);
            Serial.println(F(" !"));
            nDevices++;
        }
    }

    if (nDevices == 0)
        Serial.println(F("No I2C devices found\n"));
    else
        Serial.println(F("done\n"));

    delay(5000);
}
