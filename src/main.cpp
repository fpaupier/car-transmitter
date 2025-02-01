#include <esp_now.h>
#include <WiFi.h>
#include "secrets.h"

// Joystick pins
#define VRX_PIN  34
#define VRY_PIN  35
#define SW_PIN   32

// Data structure for joystick values
typedef struct struct_message {
    int x;
    int y;
    bool button;
} struct_message;

// Data structure
struct_message joystickData;

void setup() {
    Serial.begin(115200);
    pinMode(SW_PIN, INPUT_PULLUP);

    // Init ESP-NOW
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

    // Register peer
    esp_now_peer_info_t peerInfo;
    memcpy(peerInfo.peer_addr, RECEIVER_MAC_ADDRESS, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add peer");
        return;
    }
}

void loop() {
    // Read joystick values
    joystickData.x = analogRead(VRX_PIN) - 2048;  // Center at 0
    joystickData.y = analogRead(VRY_PIN) - 2048;
    joystickData.button = digitalRead(SW_PIN);

    // Send data via ESP-NOW
    esp_err_t result = esp_now_send(RECEIVER_MAC_ADDRESS, (uint8_t *) &joystickData, sizeof(joystickData));

    delay(50);  // Adjust for responsiveness
}
