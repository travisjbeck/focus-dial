#pragma once

// Pin configuration for ESP32-S3-Touch-AMOLED-1.75

// Display pins
#define LCD_SDIO0 4
#define LCD_SDIO1 5
#define LCD_SDIO2 6
#define LCD_SDIO3 7
#define LCD_SCLK 38
#define LCD_CS 12
#define LCD_RESET 39
#define LCD_WIDTH 466
#define LCD_HEIGHT 466

// Touch pins
#define IIC_SDA 15
#define IIC_SCL 14
#define TP_INT 11
#define TP_RESET 40

// Rotary Encoder pins (on breakout header)
#define ENCODER_A 17
#define ENCODER_B 18
#define ENCODER_BUTTON 21  // Optional push button on encoder

// NeoPixel LED Ring (24 LEDs on 3.3V power)
#define NEOPIXEL_PIN 16
#define NEOPIXEL_COUNT 24