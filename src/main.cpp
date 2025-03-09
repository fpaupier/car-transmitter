#include <Arduino.h>
#include "joystick.h"
#include "display.h"
#include "coms.h"
#include "config.h"

Joystick joystick;
Display display;
Communication communication;
String mode = DEFAULT_MODE;

void setup() {
    Serial.begin(115200);

    // Initialize display
    display.begin();

    // Initialize joystick
    joystick.begin();

    // Initialize communication
    if (!communication.begin()) {
        Serial.println("Communication initialization failed");
        // Could display error message here
    }

    Serial.println("Controller initialized");
}

void loop() {
    // Read joystick input
    joystick.read();

    // Send data to receiver
    communication.send(joystick.getData());

    // Update display
    display.update(
            joystick.getData(),
            communication.getSignalStrength(),
            joystick.getSpeed(),
            mode
    );
}
