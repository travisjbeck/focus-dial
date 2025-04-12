#pragma once

#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

class LEDController
{
public:
  LEDController(uint8_t ledPin, uint16_t numLeds, uint8_t brightness = 255);

  void begin();
  void update();

  void startFillAndDecay(uint32_t color, uint32_t totalDuration);
  void setSpinner(uint32_t color, int cycles);
  void setBreath(uint32_t color, int cycles, bool endFilled, uint32_t speed);
  void setSolid(uint32_t color);

  void turnOff();
  void printDebugInfo();

  // New preview mode methods
  void setPreviewMode(bool enabled);
  void setPreviewColor(const String &hexColor);
  void resetPreviewColor();
  bool isInPreviewMode() const;

  // Add color conversion utility
  static inline uint32_t hexColorToUint32(const String &hexColor)
  {
    if (!hexColor || hexColor.length() != 7 || hexColor[0] != '#')
    {
      Serial.printf("Invalid hex color format: %s. Defaulting to 0x000000.\n", hexColor.c_str());
      return 0; // Return black or a default color for invalid format
    }

    // Use c_str() to get pointer, then offset by 1
    long number = strtol(hexColor.c_str() + 1, NULL, 16);

    // NeoPixel uses 0x00RRGGBB format, ensure correct conversion if needed.
    // strtol directly parses the hex value correctly.
    return (uint32_t)number;
  }

private:
  Adafruit_NeoPixel leds;
  uint16_t numLeds;
  uint8_t brightness;
  int brightnessLevel;

  enum AnimationType
  {
    None,
    FillAndDecay,
    Spinner,
    Breath
  } currentAnimation;
  unsigned long lastUpdateTime;

  uint32_t animationColor;
  uint32_t animationDuration;
  uint32_t animationSpeed;

  int currentStep;
  int currentCycle;
  int pixelIndex;
  int animationCycles;
  bool endFilled;
  bool decayStarted;

  // Preview mode variables
  bool previewMode;
  uint32_t lastColor;
  AnimationType lastAnimation;

  // Animation handling methods
  void handleFillAndDecay();
  void handleSpinner();
  void handleBreath();

  // Reset animation state
  void stopCurrentAnimation();

  // Helper to scale color by brightness
  uint32_t scaleColor(uint32_t color, uint8_t brightness);

  // Save/restore state before/after preview
  void saveCurrentState();
  void restoreLastState();
};