#pragma once
#include <Arduino.h>

// Pin definitions
#define VRX_PIN             32
#define VRY_PIN             33
#define SW_PIN              27

// Joystick configuration
#define JOYSTICK_DEADZONE   50
#define NUM_CALIBRATIONS    20
#define JOYSTICK_MIN_RANGE  (-255)
#define JOYSTICK_MAX_RANGE  255
#define JOYSTICK_RAW_MIN    0
#define JOYSTICK_RAW_MAX    4095

// Display update intervals (ms)
#define DISPLAY_UPDATE_INTERVAL  50
#define SIGNAL_UPDATE_INTERVAL   1000
#define SEND_INTERVAL            20

// Color theme
#define COLOR_GREEN         0x5E0A
#define COLOR_BLUE          0x04DF
#define COLOR_DARK          0x0200
#define COLOR_TEXT          0xBFFA
#define COLOR_CRITICAL      0xF800

// Display dimensions
#define DISPLAY_WIDTH       240
#define DISPLAY_HEIGHT      135
#define HEADER_HEIGHT       25
#define JOYSTICK_AREA_HEIGHT 85
#define FOOTER_HEIGHT       25

// Joystick visualization
#define JOYSTICK_CENTER_X   120
#define JOYSTICK_CENTER_Y   42
#define JOYSTICK_SCALE      8
#define JOYSTICK_CIRCLE_RADIUS 15
#define JOYSTICK_DOT_RADIUS 5
#define JOYSTICK_POINTER_SIZE 8

// Signal strength
#define MAX_SIGNAL_STRENGTH 5
#define SIGNAL_BAR_WIDTH    6
#define SIGNAL_BAR_SPACING  8
#define SIGNAL_BAR_X_START  65

// Default mode
#define DEFAULT_MODE        "RACE"
