#pragma once

#include <Arduino.h>
#include <OneButton.h>
#include <RotaryEncoder.h>
#include <functional>

class InputController
{
public:
    InputController(uint8_t encoderPinA, uint8_t encoderPinB);
    void begin();
    void update();

    void onEncoderRotateHandler(std::function<void(int delta)> handler);
    int getEncoderPosition();

    void releaseHandlers();

private:
    RotaryEncoder encoder;

    uint8_t encoderPinA;
    uint8_t encoderPinB;

    std::function<void(int delta)> encoderRotateHandler = nullptr;

    int lastPosition;

    void onEncoderRotate(int delta);

    static void handleEncoderInterrupt();
};

extern InputController inputController;
