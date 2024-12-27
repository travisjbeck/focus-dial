#pragma once

#include <Adafruit_NeoPixel.h>

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

    // Animation handling methods
    void handleFillAndDecay();
    void handleSpinner();
    void handleBreath();

    // Reset animation state
    void stopCurrentAnimation();

    // Helper to scale color by brightness
    uint32_t scaleColor(uint32_t color, uint8_t brightness);
};