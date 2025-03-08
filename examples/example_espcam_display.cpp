//
// Created by Francois Paupier on 08/03/2025.
// Rquired conf:
//  [env:lilygo-t-display]
//      platform = espressif32
//      board = lilygo-t-display
//      framework = arduino
//      lib_deps =
//          hpsaturn/EspNowCam@^0.1.12
//          bodmer/TFT_eSPI@^2.5.0
//          bodmer/TJpg_Decoder@^1.1.0
//      monitor_speed = 115200


/*
 *
 * ## TFT-eSPI Library Modifications

The sketch we will be writing for the controller will use the TFT-eSPI Library by Bodmer. You can install this library
using the Library Manager in the Arduino IDE. You might have already installed it if you followed my instructions in the
article about using Round LCD Modules, but you may need to update it, as it needs to be at least version 2.4.79 to work
with the TTGO displays.

After installing it, you will need to modify a file in the library to work with the TTGO T-Display. Here is how you do
this:

1. Navigate to the TFT_eSPI folder in your libraries folder (which usually lives under your Arduino folder).
2. Look for User_Setup_Select.h and open it with a text editor.
3. Comment out line 30, which reads `#include <User_Setup.h>`
4. Uncomment line 61, which reads `#include <User_Setups/Setup25_TTGO_T_Display.h>`
5. Save the file.

Once you do this, the library will work with the TTGO module. Source: DroneBot workshop https://dronebotworkshop.com/mecanum/

 */


#include <Arduino.h>
#include <ESPNowCam.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <TJpg_Decoder.h>

ESPNowCam radio;
TFT_eSPI tft = TFT_eSPI();

uint8_t fb[15000]; // Static buffer in internal RAM
uint32_t dw, dh;
int16_t xpos = 0;
int16_t ypos = 0;
float scale_factor = 1.0;
bool first_frame = true;

static uint32_t frame_count = 0;
static uint_fast64_t last_time = 0;

// JPEG rendering callback required by TJpg_Decoder
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
    // Apply scaling and centering
    int16_t scaled_x = xpos + (x * scale_factor);
    int16_t scaled_y = ypos + (y * scale_factor);
    int16_t scaled_w = w * scale_factor;
    int16_t scaled_h = h * scale_factor;

    // Check boundaries
    if (scaled_y >= tft.height() || scaled_x >= tft.width()) return false;

    // Push image to display with scaling
    tft.pushImage(scaled_x, scaled_y, scaled_w, scaled_h, bitmap);

    return true;
}

// Calculate optimal scaling and positioning for the image
void calculateScaling(uint16_t img_width, uint16_t img_height) {
    float scale_w = (float) dw / img_width;
    float scale_h = (float) dh / img_height;

    // Use the smaller scaling factor to maintain aspect ratio
    scale_factor = min(scale_w, scale_h);

    // Calculate position to center the image
    xpos = (dw - (img_width * scale_factor)) / 2;
    ypos = (dh - (img_height * scale_factor)) / 2;

    // Apply scaling to TJpg_Decoder
    TJpgDec.setJpgScale(1); // We'll handle scaling in the callback
}

// Callback when data is received via ESPNowCam
void onDataReady(uint32_t length) {
    // Get image dimensions from JPEG header (only for first frame)
    if (first_frame) {
        uint16_t w = 0, h = 0;
        TJpgDec.getJpgSize(&w, &h, fb, length);
        if (w > 0 && h > 0) {
            calculateScaling(w, h);
            first_frame = false;
            Serial.printf("Image dimensions: %dx%d, Scale: %.2f\n", w, h, scale_factor);
        }
    }

    // Clear screen before drawing new frame
    tft.fillScreen(TFT_BLACK);

    // Draw the JPEG directly to the screen
    TJpgDec.drawJpg(0, 0, fb, length);

    // FPS calculation
    frame_count++;
    if (millis() - last_time >= 1000) {
        Serial.printf("FPS: %d | JPG Size: %d bytes\n", frame_count, length);
        frame_count = 0;
        last_time = millis();
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("ESPNowCam Receiver Starting...");

    // Initialize TFT display with correct settings for T-Display
    tft.init();
    tft.setRotation(1); // Landscape mode for T-Display (240x135)
    tft.fillScreen(TFT_BLACK);

    // Enable backlight for T-Display (PIN 4)
    pinMode(4, OUTPUT);
    digitalWrite(4, HIGH);

    dw = tft.width();  // 240 pixels in landscape mode
    dh = tft.height(); // 135 pixels in landscape mode

    Serial.printf("Display dimensions: %dx%d\n", dw, dh);

    // Disable WiFi scanning to prevent interference
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    // Initialize JPEG decoder with appropriate settings
    TJpgDec.setSwapBytes(true); // Critical for correct color rendering
    TJpgDec.setCallback(tft_output);

    // Set ESPNowCam receiver buffer and callback
    radio.setRecvBuffer(fb);
    radio.setRecvCallback(onDataReady);

    if (radio.init()) {
        Serial.println("ESPNowCam initialized successfully!");
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.drawCentreString("ESPNow Ready", dw / 2, dh / 2, 2);
        delay(1000); // Show message briefly
        tft.fillScreen(TFT_BLACK); // Clear screen before receiving frames
    } else {
        Serial.println("ESPNowCam initialization failed!");
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.drawCentreString("ESPNow Failed", dw / 2, dh / 2, 2);
        while (1) delay(100); // Halt if initialization fails
    }
}

void loop() {
    // Nothing to do here; ESP-NOW reception handled via callback
}
