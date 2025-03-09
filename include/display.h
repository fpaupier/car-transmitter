#pragma once

#include <TFT_eSPI.h>
#include "config.h"
#include "types.h"

class Display {
public:
    Display();

    void begin();

    void drawBootScreen();

    void update(const struct_message &joystickData, int signalStrength, int speed, const String &mode);

private:
    TFT_eSPI tft;
    TFT_eSprite headerSprite;
    TFT_eSprite joystickSprite;
    TFT_eSprite footerSprite;
    unsigned long lastUpdateTime;

    void drawHeader(int signalStrength);

    void drawJoystickVisual(const struct_message &joystickData, int speed);

    void drawFooter(const String &mode);
};