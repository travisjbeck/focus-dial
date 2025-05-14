#pragma once

#include <Arduino.h>

// --- Hardware ---
// #define OLED_WIDTH 128 // Removed, specific to old display
// #define OLED_HEIGHT 64 // Removed, specific to old display
// #define OLED_ADDR 0x3C // Removed, specific to old display

#define LED_PIN 1 // XIAO D0
#define NUM_LEDS 16
#define NEOPIXEL_TYPE NEO_GRB + NEO_KHZ800
#define LED_BRIGHTNESS 100

#define ENCODER_A_PIN 43 // XIAO D6
#define ENCODER_B_PIN 8  // XIAO D9
// #define BUTTON_PIN 26 // Deprecated, replaced by touch

// --- LED Colors (prefixed with FD_ to avoid library conflicts) --
#define FD_BLACK 0x000000
#define FD_RED 0xFF0000
#define FD_GREEN 0x00FF00
#define FD_YELLOW 0xFFFF00
#define FD_BLUE 0x0000FF
#define FD_MAGENTA 0xFF00FF
#define FD_CYAN 0x00FFFF
#define FD_WHITE 0xFFFFFF
#define FD_ORANGE 0xFFA500
#define FD_PURPLE 0x800080
#define FD_PINK 0xFFC0CB
#define FD_TEAL 0x008080
#define FD_LIME 0x00FF00 // Same as GREEN, can be removed if redundant
#define FD_BROWN 0xA52A2A
#define FD_NAVY 0x000080
#define FD_MAROON 0x800000
#define FD_OLIVE 0x808000
#define FD_SILVER 0xC0C0C0
#define FD_GOLD 0xFFD700
#define FD_INDIGO 0x4B0082
#define FD_VIOLET 0xEE82EE
#define FD_TURQUOISE 0x40E0D0
#define FD_SKYBLUE 0x87CEEB
#define FD_SALMON 0xFA8072
#define FD_PLUM 0xDDA0DD
#define FD_KHAKI 0xF0E68C

// Specific Timer Colors
#define FD_COLOR_AMBER 0xFFBF00 // Specific amber color for some states

// --- Defaults ---
#define DEFAULT_TIMER 25 // min - Default to 25 minutes if no value in NVS
#define MIN_TIMER 5      // min - Minimum timer
#define MAX_TIMER 240    // min - Maximum timer (4 hours)

#define SPLASH_DURATION 2 // sec - 2 seconds splash state
#define CHANGE_TIMEOUT 60 // sec - 5 seconds adjust timeout
#define SLEEP_TIMOUT 10   // min - 5 minutes to transition to sleep
#define PAUSE_TIMEOUT 10  // min - 10 minutes to cancel the timer if stayed paused