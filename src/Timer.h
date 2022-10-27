#include <Arduino.h>

boolean runEveryShort(unsigned long interval)

{
    static unsigned long previousMillis = 0;

    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= interval)
    {
        previousMillis = currentMillis;

        return true;
    }

    return false;
}

boolean runEveryLong(unsigned long interval)

{
    static unsigned long previousMillis = 0;

    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= interval)
    {
        previousMillis = currentMillis;

        return true;
    }

    return false;
}