#include <esp_now.h>
#include <WiFi.h>
#include <Arduino.h>
#include <HardwareSerial.h>

#include "secrets.h"

// Joystick pins
#define VRX_PIN   34
#define VRY_PIN   35
#define SW_PIN    32
#define DEADZONE  50
#define NUM_CALIBRATIONS 20
#define MIN_RANGE (-255)
#define MAX_RANGE 255


#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

// Data structure for joystick values
typedef struct struct_message {
    int x;
    int y;
    bool button;
} struct_message;

// Calibration values
int xCenter = 2048;
int yCenter = 2048;
int xMin = 0;
int xMax = 4095;
int yMin = 0;
int yMax = 4095;

struct_message joystickData;
bool connected = false;
unsigned long lastSendTime = 0;

esp_now_peer_info_t peerInfo;

// LED States
void updateLED() {
    static unsigned long lastBlink = 0;
    static bool ledState = false;

    if (connected) {
        digitalWrite(LED_BUILTIN, HIGH);
        return;
    }

    // Blink pattern when not connected
    if (millis() - lastBlink > 200) {
        ledState = !ledState;
        digitalWrite(LED_BUILTIN, ledState);
        lastBlink = millis();
    }
}

// Send callback
void sendCallback(const uint8_t *mac_addr, esp_now_send_status_t status) {
//    Serial.print("\r\nLast Packet Send Status:\t");
//    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
    connected = (status == ESP_NOW_SEND_SUCCESS);
}

void calibrateJoystick() {
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

int applyDeadzone(int value, int deadzone) {
    return abs(value) < deadzone ? 0 : value;
}

int mapJoystickToRange(int value, int valueMin, int valueMax, int valueCenter, int outMin, int outMax) {
    // Determine if the value is to the left or right of center
    if (value < valueCenter) {
        // Map from min to center to outMin to 0
        return map(value, valueMin, valueCenter, outMin, 0);
    } else {
        // Map from center to max to 0 to outMax
        return map(value, valueCenter, valueMax, 0, outMax);
    }
}

void readJoystick() {
    int rawX = analogRead(VRX_PIN);
    int rawY = analogRead(VRY_PIN);

    int mappedX = mapJoystickToRange(rawX, xMin, xMax, xCenter, MIN_RANGE, MAX_RANGE);
    int mappedY = mapJoystickToRange(rawY, yMin, yMax, yCenter, MIN_RANGE, MAX_RANGE);

    joystickData.x = applyDeadzone(mappedX, DEADZONE);
    joystickData.y = applyDeadzone(mappedY, DEADZONE);

    joystickData.button = !digitalRead(SW_PIN);
    Serial.printf("X: %4d (Raw X: %4d)| Y: %4d (Raw Y: %4d)| BTN: %d\n", joystickData.x, rawX, joystickData.y, rawY, joystickData.button);
}

void setup() {
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(SW_PIN, INPUT_PULLUP);

    // Joystick calibration
    calibrateJoystick();

    // Initialize WiFi
    WiFi.mode(WIFI_STA);

    // Initialize ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW init failed");
        int c = 0;
        while (c < 3) { // Rapid LED blink on failure
            c++;
            digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
            delay(100);
        }
        return;
    }

    esp_now_register_send_cb(sendCallback);

    // Register peer
    memcpy(peerInfo.peer_addr, RECEIVER_MAC_ADDRESS, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add peer");
        int counter = 0;
        while (counter < 3) { // Double blink pattern on peer error
            counter++;
            digitalWrite(LED_BUILTIN, HIGH);
            delay(100);
            digitalWrite(LED_BUILTIN, LOW);
            delay(100);
            digitalWrite(LED_BUILTIN, HIGH);
            delay(100);
            digitalWrite(LED_BUILTIN, LOW);
            delay(500);
        }
        return;
    }
    // Register callback once during setup
    Serial.println("Transmitter initialized");
}

void loop() {
    updateLED();
    readJoystick();
    if (millis() - lastSendTime > 20) { // 50Hz refresh rate
        if (esp_now_send(RECEIVER_MAC_ADDRESS, (uint8_t *) &joystickData, sizeof(joystickData)) != ESP_OK) {
            Serial.println("Send Failed");
        }
        lastSendTime = millis();
    }
}