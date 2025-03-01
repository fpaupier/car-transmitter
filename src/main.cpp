#include <esp_now.h>
#include <WiFi.h>
#include <Arduino.h>
#include <Adafruit_SH110X.h>
#include <HardwareSerial.h>
#include <Wire.h>
#include <ESPNowCam.h>


#include "secrets.h"

// Joystick pins
#define VRX_PIN   34
#define VRY_PIN   35
#define SW_PIN    32
#define DEADZONE  50
#define NUM_CALIBRATIONS 20
#define MIN_RANGE (-255)
#define MAX_RANGE 255

// Display pin
#define SDA_PIN 21
#define SCL_PIN 22


#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif


#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 128 // OLED display height, in pixels
#define OLED_RESET (-1)     // can set an oled reset pin if desired

Adafruit_SH1107 display = Adafruit_SH1107(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET, 1000000, 100000);


// ESPNowCam instance
ESPNowCam radio;

// Frame buffer
uint8_t *fb;
uint8_t *bwBuffer; // Buffer for black and white converted image

// Display globals
int32_t dw, dh;
unsigned long lastFrameTime = 0;
float fps = 0;


// Function to convert JPEG to 1-bit black and white for OLED
void jpgToBW(uint8_t *jpgData, uint32_t length) {
    // This is a simplified placeholder - in a real implementation,
    // you would need to decode the JPEG and convert to 1-bit BW
    // For now, we'll just fill the display with a test pattern

    display.clearDisplay();

    // Draw a test pattern or placeholder
    display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SH110X_WHITE);
    display.setCursor(10, 10);
    display.print("Frame: ");
    display.print(length);
    display.setCursor(10, 30);
    display.print("FPS: ");
    display.print(fps, 1);

    // In a real implementation, you would:
    // 1. Decode the JPEG (using TJpgDec or similar library)
    // 2. Resize to 128x128
    // 3. Convert to black and white using a threshold
    // 4. Draw to the display buffer

    display.display();
}

void printFPS() {
    unsigned long currentTime = millis();
    if (lastFrameTime > 0) {
        fps = 1000.0 / (currentTime - lastFrameTime);
    }
    lastFrameTime = currentTime;

    Serial.print("FPS: ");
    Serial.println(fps);
}


void onDataReady(uint32_t length) {
    // Process the received JPEG data and display on OLED
    jpgToBW(fb, length);
    printFPS();
}


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
    Serial.printf("X: %4d (Raw X: %4d)| Y: %4d (Raw Y: %4d)| BTN: %d\n", joystickData.x, rawX, joystickData.y, rawY,
                  joystickData.button);
}


void testdrawline() {
    int16_t i;

    display.clearDisplay(); // Clear display buffer

    for (i = 0; i < display.width(); i += 4) {
        display.drawLine(0, 0, i, display.height() - 1, SH110X_WHITE);
        display.display(); // Update screen with each newly-drawn line
        delay(1);
    }
    for (i = 0; i < display.height(); i += 4) {
        display.drawLine(0, 0, display.width() - 1, i, SH110X_WHITE);
        display.display();
        delay(1);
    }
    delay(250);

    display.clearDisplay();

    for (i = 0; i < display.width(); i += 4) {
        display.drawLine(0, display.height() - 1, i, 0, SH110X_WHITE);
        display.display();
        delay(1);
    }
    for (i = display.height() - 1; i >= 0; i -= 4) {
        display.drawLine(0, display.height() - 1, display.width() - 1, i, SH110X_WHITE);
        display.display();
        delay(1);
    }
    delay(250);

    display.clearDisplay();

    for (i = display.width() - 1; i >= 0; i -= 4) {
        display.drawLine(display.width() - 1, display.height() - 1, i, 0, SH110X_WHITE);
        display.display();
        delay(1);
    }
    for (i = display.height() - 1; i >= 0; i -= 4) {
        display.drawLine(display.width() - 1, display.height() - 1, 0, i, SH110X_WHITE);
        display.display();
        delay(1);
    }
    delay(250);

    display.clearDisplay();

    for (i = 0; i < display.height(); i += 4) {
        display.drawLine(display.width() - 1, 0, 0, i, SH110X_WHITE);
        display.display();
        delay(1);
    }
    for (i = 0; i < display.width(); i += 4) {
        display.drawLine(display.width() - 1, 0, i, display.height() - 1, SH110X_WHITE);
        display.display();
        delay(1);
    }

    delay(2000); // Pause for 2 seconds
}

void testdrawrect(void) {
    display.clearDisplay();

    for (int16_t i = 0; i < display.height() / 2; i += 2) {
        display.drawRect(i, i, display.width() - 2 * i, display.height() - 2 * i, SH110X_WHITE);
        display.display(); // Update screen with each newly-drawn rectangle
        delay(1);
    }

    delay(2000);
}

void testfillrect(void) {
    display.clearDisplay();

    for (int16_t i = 0; i < display.height() / 2; i += 3) {
        // The INVERSE color is used so rectangles alternate white/black
        display.fillRect(i, i, display.width() - i * 2, display.height() - i * 2, SH110X_INVERSE);
        display.display(); // Update screen with each newly-drawn rectangle
        delay(1);
    }

    delay(2000);
}


void setup() {
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(SW_PIN, INPUT_PULLUP);

    // Initialize display
    Wire.begin(SDA_PIN, SCL_PIN);
    display.begin(0x3C, true); // Address may be 0x3D default

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 0);
    display.println("ESPNowCam OLED");
    display.println("Initializing...");
    display.display();
    delay(1000);

    dw = SCREEN_WIDTH;
    dh = SCREEN_HEIGHT;

    // Check for PSRAM
    if (psramFound()) {
        size_t psram_size = esp_spiram_get_size() / 1048576;
        Serial.printf("PSRAM size: %dMb\r\n", psram_size);

        // Allocate buffer in PSRAM
        fb = (uint8_t *) ps_malloc(5000 * sizeof(uint8_t));
        bwBuffer = (uint8_t *) ps_malloc(SCREEN_WIDTH * SCREEN_HEIGHT / 8); // 1-bit per pixel
    } else {
        // Fallback to regular memory if PSRAM not available
        Serial.println("PSRAM not found, using regular memory");
        fb = (uint8_t *) malloc(5000 * sizeof(uint8_t));
        bwBuffer = (uint8_t *) malloc(SCREEN_WIDTH * SCREEN_HEIGHT / 8);
    }


    if (!fb || !bwBuffer) {
        Serial.println("Memory allocation failed!");
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("Memory error!");
        display.display();
        for (;;);
    }

    // Initialize ESPNowCam
    radio.setRecvBuffer(fb);
    radio.setRecvCallback(onDataReady);

    if (radio.init()) {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("ESPNow Init Success");
        display.println("Waiting for data...");
        display.display();
        Serial.println("ESPNow initialized successfully");
    } else {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("ESPNow Init Failed");
        display.display();
        Serial.println("ESPNow initialization failed");
    }

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
