#pragma once

#include "config.h"
#include "types.h"

class Joystick {
public:
    Joystick();

    void begin();

    void calibrate();

    void read();

    struct_message getData() const;

    int getSpeed() const;

private:
    struct_message data;
    int xCenter;
    int yCenter;
    int xMin;
    int xMax;
    int yMin;
    int yMax;
    int speed;

    int applyDeadzone(int value, int deadzone);

    int mapJoystickToRange(int value, int valueMin, int valueMax, int valueCenter, int outMin, int outMax);
};
