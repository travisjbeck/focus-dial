#include "controllers/InputController.h"
#include <Arduino.h>

static InputController *instancePtr = nullptr; // Global pointer for the ISR

void InputController::handleEncoderInterrupt()
{
    if (instancePtr)
    {
        instancePtr->encoder.tick();
    }
}

void InputController::handleButtonInterrupt()
{
    if (instancePtr)
    {
        instancePtr->button.tick();
    }
}

InputController::InputController(uint8_t buttonPin, uint8_t encoderPinA, uint8_t encoderPinB)
    : button(buttonPin, true),
      encoder(encoderPinA, encoderPinB, RotaryEncoder::LatchMode::TWO03),
      lastPosition(0),
      buttonPin(buttonPin),
      encoderPinA(encoderPinA),
      encoderPinB(encoderPinB)
{

    // Attach click, double-click, and long-press handlers using OneButton library
    button.attachClick([](void *scope)
                       { static_cast<InputController *>(scope)->onButtonClick(); }, this);
    button.attachDoubleClick([](void *scope)
                             { static_cast<InputController *>(scope)->onButtonDoubleClick(); }, this);
    button.attachLongPressStart([](void *scope)
                                { static_cast<InputController *>(scope)->onButtonLongPress(); }, this);

    instancePtr = this; // Set the global instance pointer to this instance
}

void InputController::begin()
{
    button.setDebounceMs(20);
    button.setClickMs(150);
    button.setPressMs(400);
    lastPosition = encoder.getPosition();

    pinMode(buttonPin, INPUT_PULLUP);
    pinMode(encoderPinA, INPUT_PULLUP);
    pinMode(encoderPinB, INPUT_PULLUP);

    // Set up interrupts for encoder handling
    attachInterrupt(digitalPinToInterrupt(encoderPinA), handleEncoderInterrupt, CHANGE);
    attachInterrupt(digitalPinToInterrupt(encoderPinB), handleEncoderInterrupt, CHANGE);

    // Set up interrupt for button handling
    attachInterrupt(digitalPinToInterrupt(buttonPin), handleButtonInterrupt, CHANGE); // Interrupt on button state change
}

void InputController::update()
{
    button.tick();
    encoder.tick();

    // Check encoder position and calculate delta
    int currentPosition = encoder.getPosition();
    int delta = currentPosition - lastPosition;

    if (delta != 0)
    {
        onEncoderRotate(delta);
        lastPosition = currentPosition;
    }
}

// Register state-specific handlers
void InputController::onPressHandler(std::function<void()> handler)
{
    pressHandler = handler;
}

void InputController::onDoublePressHandler(std::function<void()> handler)
{
    doublePressHandler = handler;
}

void InputController::onLongPressHandler(std::function<void()> handler)
{
    longPressHandler = handler;
}

void InputController::onEncoderRotateHandler(std::function<void(int delta)> handler)
{
    encoderRotateHandler = handler;
}

// Method to release all handlers
void InputController::releaseHandlers()
{
    pressHandler = nullptr;
    doublePressHandler = nullptr;
    longPressHandler = nullptr;
    encoderRotateHandler = nullptr;

    button.reset();                       // Reset button state machine
    lastPosition = encoder.getPosition(); // Reset encoder position tracking
}

// Internal event handlers that call the registered state handlers
void InputController::onButtonClick()
{
    if (pressHandler != nullptr)
    {
        pressHandler();
    }
}

void InputController::onButtonDoubleClick()
{
    if (doublePressHandler != nullptr)
    {
        doublePressHandler();
    }
}

void InputController::onButtonLongPress()
{
    if (longPressHandler != nullptr)
    {
        longPressHandler();
    }
}

void InputController::onEncoderRotate(int delta)
{
    if (encoderRotateHandler != nullptr)
    {
        encoderRotateHandler(delta); // Pass delta to the handler
    }
}
