#include "joystick.h"

Joystick::Joystick() :
        xCenter(JOYSTICK_RAW_MAX / 2),
        yCenter(JOYSTICK_RAW_MAX / 2),
        xMin(JOYSTICK_RAW_MIN),
        xMax(JOYSTICK_RAW_MAX),
        yMin(JOYSTICK_RAW_MIN),
        yMax(JOYSTICK_RAW_MAX),
        speed(0) {
    data.x = 0;
    data.y = 0;
    data.button = false;
}

void Joystick::begin() {
    pinMode(SW_PIN, INPUT_PULLUP);
    calibrate();
}

void Joystick::calibrate() {
    Serial.println("Starting joystick calibration...");
    int sum_x = 0;
    int sum_y = 0;
    for (int i = 0; i < NUM_CALIBRATIONS; ++i) {
        sum_x += analogRead(VRX_PIN);
        sum_y += analogRead(VRY_PIN);
        delay(10);
    }
    xCenter = sum_x / NUM_CALIBRATIONS;
    yCenter = sum_y / NUM_CALIBRATIONS;

    Serial.println("Calibration complete.");
    Serial.print("xCenter: ");
    Serial.println(xCenter);
    Serial.print("yCenter: ");
    Serial.println(yCenter);
}

int Joystick::applyDeadzone(int value, int deadzone) {
    return abs(value) < deadzone ? 0 : value;
}

int Joystick::mapJoystickToRange(int value, int valueMin, int valueMax, int valueCenter, int outMin, int outMax) {
    if (value < valueCenter) {
        return map(value, valueMin, valueCenter, outMin, 0);
    } else {
        return map(value, valueCenter, valueMax, 0, outMax);
    }
}

void Joystick::read() {
    int rawX = analogRead(VRX_PIN);
    int rawY = analogRead(VRY_PIN);

    int mappedX = mapJoystickToRange(rawX, xMin, xMax, xCenter, JOYSTICK_MIN_RANGE, JOYSTICK_MAX_RANGE);
    int mappedY = -mapJoystickToRange(rawY, yMin, yMax, yCenter, JOYSTICK_MIN_RANGE, JOYSTICK_MAX_RANGE);

    data.x = applyDeadzone(mappedX, JOYSTICK_DEADZONE);
    data.y = applyDeadzone(mappedY, JOYSTICK_DEADZONE);
    data.button = !digitalRead(SW_PIN);

    // Calculate simulated speed based on joystick Y position
    speed = map(abs(data.y), 0, JOYSTICK_MAX_RANGE, 0, 100);
}

struct_message Joystick::getData() const {
    return data;
}

int Joystick::getSpeed() const {
    return speed;
}
