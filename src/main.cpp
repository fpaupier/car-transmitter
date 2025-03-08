#include <esp_now.h>
#include <WiFi.h>
#include <Arduino.h>
#include <TFT_eSPI.h>
#include "secrets.h"

// Joystick pins
#define VRX_PIN   32
#define VRY_PIN   33
#define SW_PIN    27
#define DEADZONE  50
#define NUM_CALIBRATIONS 20
#define MIN_RANGE (-255)
#define MAX_RANGE 255

// LED and display
#define BACKLIGHT_PIN 4
#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

// Color theme
#define GREEN     0x5E0A
#define BLUE      0x04DF
#define DARK      0x0200
#define TEXT_COLOR      0xBFFA
#define WARNING_COLOR   0xF9E0
#define CRITICAL_COLOR  0xF800

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
unsigned long lastDisplayUpdate = 0;
unsigned long lastSignalUpdate = 0;
int signalStrength = 3; // 0-5 scale
int carBattery = 76;    // Placeholder
int remoteBattery = 85; // Placeholder
int latency = 24;       // Placeholder in ms
int speed = 0;          // Calculated from joystick values
String mode = "RACE";   // Current mode

// Initialize display
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite headerSprite = TFT_eSprite(&tft);
TFT_eSprite joystickSprite = TFT_eSprite(&tft);
TFT_eSprite footerSprite = TFT_eSprite(&tft);

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
    connected = (status == ESP_NOW_SEND_SUCCESS);

    // Update signal strength based on success/failure
    if (millis() - lastSignalUpdate > 1000) { // Update once per second
        if (connected) {
            if (signalStrength < 5) signalStrength++;
        } else {
            if (signalStrength > 0) signalStrength--;
        }
        lastSignalUpdate = millis();
    }
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
    int mappedY = -mapJoystickToRange(rawY, yMin, yMax, yCenter, MIN_RANGE, MAX_RANGE);

    joystickData.x = applyDeadzone(mappedX, DEADZONE);
    joystickData.y = applyDeadzone(mappedY, DEADZONE);
    joystickData.button = !digitalRead(SW_PIN);

    // Calculate simulated speed based on joystick Y position
    speed = map(abs(joystickData.y), 0, 255, 0, 35);
}

// Draw boot animation
void drawBootScreen() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(GREEN);

    for (int i = 0; i < 10; i++) {
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(10, 30);
        tft.setTextSize(1);
        tft.println("NAZGHUL INDUSTRIES.");
        tft.setCursor(10, 50);
        tft.println("REMOTE CONTROL SYSTEM");
        tft.setCursor(10, 70);
        tft.println("INITIALIZING...");
        tft.setCursor(10, 90);
        tft.print("PROGRESS: [");
        for (int j = 0; j < i; j++) {
            tft.print("=");
        }
        for (int j = i; j < 10; j++) {
            tft.print(" ");
        }
        tft.println("]");
        delay(100);
    }

    tft.fillScreen(TFT_BLACK);
    tft.setCursor(20, 60);
    tft.setTextSize(2);
    tft.println("SYSTEM READY");
    delay(500);
}

// Initialize display and sprites
void setupDisplay() {
    // Initialize TFT
    tft.init();
    tft.setRotation(1); // Landscape
    tft.fillScreen(TFT_BLACK);

    // Enable backlight
    pinMode(BACKLIGHT_PIN, OUTPUT);
    digitalWrite(BACKLIGHT_PIN, HIGH);

    // Create sprites
    headerSprite.createSprite(240, 25);
    joystickSprite.createSprite(240, 85);
    footerSprite.createSprite(240, 25);

    // Draw initial screen
    drawBootScreen();
    delay(2000);
}


// Draw header with signal, battery, latency
void drawHeader() {
    headerSprite.fillSprite(DARK);
    headerSprite.setTextColor(TEXT_COLOR);

    // Draw signal strength
    headerSprite.setCursor(5, 8);
    headerSprite.setTextSize(1);
    headerSprite.print("SIGNAL:");

    for (int i = 0; i < 5; i++) {
        if (i < signalStrength) {
            headerSprite.fillRect(65 + (i * 8), 15 - (i * 2), 6, 3 + (i * 2), GREEN);
        } else {
            headerSprite.drawRect(65 + (i * 8), 15 - (i * 2), 6, 3 + (i * 2), GREEN);
        }
    }

    // Draw remote battery
    headerSprite.setCursor(120, 8);
    headerSprite.print("BATT:");

    uint16_t battColor = remoteBattery > 50 ? GREEN :
                         (remoteBattery > 20 ? WARNING_COLOR : CRITICAL_COLOR);

    headerSprite.setTextColor(battColor);
    headerSprite.print(remoteBattery);
    headerSprite.print("%");

    // Draw latency
    headerSprite.setTextColor(TEXT_COLOR);
    headerSprite.setCursor(180, 8);
    headerSprite.print("LAT:");
    headerSprite.print(latency);
    headerSprite.print("ms");

    // Draw separator line
    headerSprite.drawLine(0, 24, 240, 24, GREEN);

    // Push to screen
    headerSprite.pushSprite(0, 0);
}

// Draw joystick visualization
void drawJoystickVisual() {
    joystickSprite.fillSprite(TFT_BLACK);

    // Calculate joystick position (centered at 120,42)
    int joyX = 120 + (joystickData.x / 8);
    int joyY = 42 - (joystickData.y / 8);

    // Draw crosshair background
    joystickSprite.drawLine(120, 10, 120, 74, DARK);
    joystickSprite.drawLine(70, 42, 170, 42, DARK);

// Draw direction indicators based on joystick position
    if (joystickData.y > DEADZONE) { // Forward (inverted from original)
        joystickSprite.fillTriangle(120, 10, 115, 20, 125, 20, GREEN);
    } else {
        joystickSprite.drawTriangle(120, 10, 115, 20, 125, 20, DARK);
    }

    if (joystickData.y < -DEADZONE) { // Backward (inverted from original)
        joystickSprite.fillTriangle(120, 74, 115, 64, 125, 64, GREEN);
    } else {
        joystickSprite.drawTriangle(120, 74, 115, 64, 125, 64, DARK);
    }

    if (joystickData.x < -DEADZONE) { // Left
        joystickSprite.fillTriangle(70, 42, 80, 37, 80, 47, GREEN);
    } else {
        joystickSprite.drawTriangle(70, 42, 80, 37, 80, 47, DARK);
    }

    if (joystickData.x > DEADZONE) { // Right
        joystickSprite.fillTriangle(170, 42, 160, 37, 160, 47, GREEN);
    } else {
        joystickSprite.drawTriangle(170, 42, 160, 37, 160, 47, DARK);
    }

// Draw diagonal indicators
    if (joystickData.x < -DEADZONE && joystickData.y > DEADZONE) { // Forward-Left (inverted Y)
        joystickSprite.fillTriangle(90, 20, 95, 15, 100, 25, GREEN);
    } else {
        joystickSprite.drawTriangle(90, 20, 95, 15, 100, 25, DARK);
    }

    if (joystickData.x > DEADZONE && joystickData.y > DEADZONE) { // Forward-Right (inverted Y)
        joystickSprite.fillTriangle(150, 20, 145, 15, 140, 25, GREEN);
    } else {
        joystickSprite.drawTriangle(150, 20, 145, 15, 140, 25, DARK);
    }

    if (joystickData.x < -DEADZONE && joystickData.y < -DEADZONE) { // Backward-Left (inverted Y)
        joystickSprite.fillTriangle(90, 64, 95, 69, 100, 59, GREEN);
    } else {
        joystickSprite.drawTriangle(90, 64, 95, 69, 100, 59, DARK);
    }

    if (joystickData.x > DEADZONE && joystickData.y < -DEADZONE) { // Backward-Right (inverted Y)
        joystickSprite.fillTriangle(150, 64, 145, 69, 140, 59, GREEN);
    } else {
        joystickSprite.drawTriangle(150, 64, 145, 69, 140, 59, DARK);
    }

    // Draw center point
    joystickSprite.drawCircle(120, 42, 15, BLUE);
    joystickSprite.drawCircle(120, 42, 5, BLUE);

    // Draw joystick position
    int joySize = 8;
    joystickSprite.fillCircle(joyX, joyY, joySize, GREEN);
    joystickSprite.drawCircle(joyX, joyY, joySize, TEXT_COLOR);

    // Draw speed indicator
    joystickSprite.setTextColor(TEXT_COLOR);
    joystickSprite.setCursor(180, 35);
    joystickSprite.setTextSize(1);
    joystickSprite.print("SPEED:");
    joystickSprite.setCursor(180, 50);
    joystickSprite.setTextSize(2);
    joystickSprite.print(speed);
    joystickSprite.setTextSize(1);
    joystickSprite.print(" km/h");

    // Push to screen
    joystickSprite.pushSprite(0, 25);
}

// Draw footer with mode and car battery
void drawFooter() {
    footerSprite.fillSprite(DARK);
    footerSprite.setTextColor(TEXT_COLOR);

    // Draw separator line
    footerSprite.drawLine(0, 0, 240, 0, GREEN);

    // Draw mode
    footerSprite.setCursor(10, 8);
    footerSprite.setTextSize(1);
    footerSprite.print("MODE: ");
    footerSprite.setTextColor(GREEN);
    footerSprite.print(mode);

    // Draw car battery
    footerSprite.setTextColor(TEXT_COLOR);
    footerSprite.setCursor(120, 8);
    footerSprite.print("CAR BATT: ");

    uint16_t carBattColor = carBattery > 50 ? GREEN :
                            (carBattery > 20 ? WARNING_COLOR : CRITICAL_COLOR);

    footerSprite.setTextColor(carBattColor);
    footerSprite.print(carBattery);
    footerSprite.print("%");

    // Push to screen
    footerSprite.pushSprite(0, 110);
}

// Update the display
void updateDisplay() {
    if (millis() - lastDisplayUpdate > 50) { // 20 FPS update rate
        drawHeader();
        drawJoystickVisual();
        drawFooter();
        lastDisplayUpdate = millis();
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(SW_PIN, INPUT_PULLUP);

    // Initialize display
    setupDisplay();

    // Joystick calibration
    calibrateJoystick();

    // Initialize WiFi
    WiFi.mode(WIFI_STA);

    // Initialize ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW init failed");
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(20, 60);
        tft.setTextColor(CRITICAL_COLOR);
        tft.setTextSize(1);
        tft.println("ESP-NOW INIT FAILED");

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
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(20, 60);
        tft.setTextColor(CRITICAL_COLOR);
        tft.setTextSize(1);
        tft.println("FAILED TO ADD PEER");

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

    Serial.println("Transmitter initialized");
}

void loop() {
    updateLED();
    readJoystick();
    updateDisplay();

    if (millis() - lastSendTime > 20) { // 50Hz refresh rate
        if (esp_now_send(RECEIVER_MAC_ADDRESS, (uint8_t *) &joystickData, sizeof(joystickData)) != ESP_OK) {
            Serial.println("Send Failed");
        }
        lastSendTime = millis();
    }

    // Simulate battery drain (for demo purposes)
    if (millis() % 60000 == 0) { // Every minute
        if (remoteBattery > 0) remoteBattery--;
        if (carBattery > 0) carBattery--;
    }

    // Simulate latency fluctuations (for demo purposes)
    if (millis() % 5000 == 0) { // Every 5 seconds
        latency = random(15, 35);
    }
}
