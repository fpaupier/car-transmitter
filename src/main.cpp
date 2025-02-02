#include <esp_now.h>
#include <WiFi.h>
#include <Arduino.h>
#include <HardwareSerial.h>  // Explicit serial declaration

#include "secrets.h"

// Joystick pins
#define VRX_PIN  34
#define VRY_PIN  35
#define SW_PIN   32
#define DEADZONE 50


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
    Serial.print("\r\nLast Packet Send Status:\t");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
    connected = (status == ESP_NOW_SEND_SUCCESS);
}

void calibrateJoystick() {
    delay(1000);
    xCenter = analogRead(VRX_PIN);
    yCenter = analogRead(VRY_PIN);
}

int applyDeadzone(int value, int deadzone) {
    return abs(value) < deadzone ? 0 : value;
}

void readJoystick() {
    int rawX = analogRead(VRX_PIN) - xCenter;
    int rawY = analogRead(VRY_PIN) - yCenter;

    // Apply deadzone and map to -255~255 range
    joystickData.x = applyDeadzone(map(rawX, -2048, 2048, -255, 255), DEADZONE);
    joystickData.y = applyDeadzone(map(rawY, -2048, 2048, -255, 255), DEADZONE);
    joystickData.button = !digitalRead(SW_PIN);
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

