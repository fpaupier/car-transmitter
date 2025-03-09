#include "display.h"

Display::Display() :
    tft(),
    headerSprite(&tft),
    joystickSprite(&tft),
    footerSprite(&tft),
    lastUpdateTime(0)
{
    // Constructor body - no need to assign sprites here anymore
}


void Display::begin() {
    // Initialize TFT
    tft.init();
    tft.setRotation(1); // Landscape
    tft.fillScreen(TFT_BLACK);

    // Create sprites
    headerSprite.createSprite(DISPLAY_WIDTH, HEADER_HEIGHT);
    joystickSprite.createSprite(DISPLAY_WIDTH, JOYSTICK_AREA_HEIGHT);
    footerSprite.createSprite(DISPLAY_WIDTH, FOOTER_HEIGHT);

    // Draw initial screen
    drawBootScreen();
    delay(2000);
}

void Display::drawBootScreen() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(COLOR_GREEN);

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

void Display::update(const struct_message &joystickData, int signalStrength, int speed, const String &mode) {
    if (millis() - lastUpdateTime > DISPLAY_UPDATE_INTERVAL) {
        drawHeader(signalStrength);
        drawJoystickVisual(joystickData, speed);
        drawFooter(mode);
        lastUpdateTime = millis();
    }
}

void Display::drawHeader(int signalStrength) {
    headerSprite.fillSprite(COLOR_DARK);
    headerSprite.setTextColor(COLOR_TEXT);

    // Draw signal strength
    headerSprite.setCursor(5, 8);
    headerSprite.setTextSize(1);
    headerSprite.print("SIGNAL:");

    for (int i = 0; i < MAX_SIGNAL_STRENGTH; i++) {
        if (i < signalStrength) {
            headerSprite.fillRect(SIGNAL_BAR_X_START + (i * SIGNAL_BAR_SPACING),
                                  15 - (i * 2),
                                  SIGNAL_BAR_WIDTH,
                                  3 + (i * 2),
                                  COLOR_GREEN);
        } else {
            headerSprite.drawRect(SIGNAL_BAR_X_START + (i * SIGNAL_BAR_SPACING),
                                  15 - (i * 2),
                                  SIGNAL_BAR_WIDTH,
                                  3 + (i * 2),
                                  COLOR_GREEN);
        }
    }

    // Draw separator line
    headerSprite.drawLine(0, HEADER_HEIGHT - 1, DISPLAY_WIDTH, HEADER_HEIGHT - 1, COLOR_GREEN);

    // Push to screen
    headerSprite.pushSprite(0, 0);
}

void Display::drawJoystickVisual(const struct_message &joystickData, int speed) {
    joystickSprite.fillSprite(TFT_BLACK);

    // Calculate joystick position
    int joyX = JOYSTICK_CENTER_X + (joystickData.x / JOYSTICK_SCALE);
    int joyY = JOYSTICK_CENTER_Y - (joystickData.y / JOYSTICK_SCALE);

    // Draw crosshair background
    joystickSprite.drawLine(JOYSTICK_CENTER_X, 10, JOYSTICK_CENTER_X, 74, COLOR_DARK);
    joystickSprite.drawLine(70, JOYSTICK_CENTER_Y, 170, JOYSTICK_CENTER_Y, COLOR_DARK);

    // Draw direction indicators based on joystick position
    // Forward
    if (joystickData.y > JOYSTICK_DEADZONE) {
        joystickSprite.fillTriangle(120, 10, 115, 20, 125, 20, COLOR_GREEN);
    } else {
        joystickSprite.drawTriangle(120, 10, 115, 20, 125, 20, COLOR_DARK);
    }

    // Backward
    if (joystickData.y < -JOYSTICK_DEADZONE) {
        joystickSprite.fillTriangle(120, 74, 115, 64, 125, 64, COLOR_GREEN);
    } else {
        joystickSprite.drawTriangle(120, 74, 115, 64, 125, 64, COLOR_DARK);
    }

    // Left
    if (joystickData.x < -JOYSTICK_DEADZONE) {
        joystickSprite.fillTriangle(70, 42, 80, 37, 80, 47, COLOR_GREEN);
    } else {
        joystickSprite.drawTriangle(70, 42, 80, 37, 80, 47, COLOR_DARK);
    }

    // Right
    if (joystickData.x > JOYSTICK_DEADZONE) {
        joystickSprite.fillTriangle(170, 42, 160, 37, 160, 47, COLOR_GREEN);
    } else {
        joystickSprite.drawTriangle(170, 42, 160, 37, 160, 47, COLOR_DARK);
    }

    // Draw diagonal indicators
    // Forward-Left
    if (joystickData.x < -JOYSTICK_DEADZONE && joystickData.y > JOYSTICK_DEADZONE) {
        joystickSprite.fillTriangle(90, 20, 95, 15, 100, 25, COLOR_GREEN);
    } else {
        joystickSprite.drawTriangle(90, 20, 95, 15, 100, 25, COLOR_DARK);
    }

    // Forward-Right
    if (joystickData.x > JOYSTICK_DEADZONE && joystickData.y > JOYSTICK_DEADZONE) {
        joystickSprite.fillTriangle(150, 20, 145, 15, 140, 25, COLOR_GREEN);
    } else {
        joystickSprite.drawTriangle(150, 20, 145, 15, 140, 25, COLOR_DARK);
    }

    // Backward-Left
    if (joystickData.x < -JOYSTICK_DEADZONE && joystickData.y < -JOYSTICK_DEADZONE) {
        joystickSprite.fillTriangle(90, 64, 95, 69, 100, 59, COLOR_GREEN);
    } else {
        joystickSprite.drawTriangle(90, 64, 95, 69, 100, 59, COLOR_DARK);
    }

    // Backward-Right
    if (joystickData.x > JOYSTICK_DEADZONE && joystickData.y < -JOYSTICK_DEADZONE) {
        joystickSprite.fillTriangle(150, 64, 145, 69, 140, 59, COLOR_GREEN);
    } else {
        joystickSprite.drawTriangle(150, 64, 145, 69, 140, 59, COLOR_DARK);
    }

    // Draw center point
    joystickSprite.drawCircle(JOYSTICK_CENTER_X, JOYSTICK_CENTER_Y, JOYSTICK_CIRCLE_RADIUS, COLOR_BLUE);
    joystickSprite.drawCircle(JOYSTICK_CENTER_X, JOYSTICK_CENTER_Y, JOYSTICK_DOT_RADIUS, COLOR_BLUE);

    // Draw joystick position
    joystickSprite.fillCircle(joyX, joyY, JOYSTICK_POINTER_SIZE, COLOR_GREEN);
    joystickSprite.drawCircle(joyX, joyY, JOYSTICK_POINTER_SIZE, COLOR_TEXT);

    // Draw speed indicator
    joystickSprite.setTextColor(COLOR_TEXT);
    joystickSprite.setCursor(180, 35);
    joystickSprite.setTextSize(1);
    joystickSprite.print("SPEED:");
    joystickSprite.setCursor(180, 50);
    joystickSprite.setTextSize(2);
    joystickSprite.print(speed);
    joystickSprite.setTextSize(1);
    joystickSprite.print(" %");

    // Push to screen
    joystickSprite.pushSprite(0, HEADER_HEIGHT);
}

void Display::drawFooter(const String &mode) {
    footerSprite.fillSprite(COLOR_DARK);
    footerSprite.setTextColor(COLOR_TEXT);

    // Draw separator line
    footerSprite.drawLine(0, 0, DISPLAY_WIDTH, 0, COLOR_GREEN);

    // Draw mode
    footerSprite.setCursor(10, 8);
    footerSprite.setTextSize(1);
    footerSprite.print("MODE: ");
    footerSprite.setTextColor(COLOR_GREEN);
    footerSprite.print(mode);

    // Push to screen
    footerSprite.pushSprite(0, HEADER_HEIGHT + JOYSTICK_AREA_HEIGHT);
}
