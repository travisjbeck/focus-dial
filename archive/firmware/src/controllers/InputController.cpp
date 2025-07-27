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

/* // Button deprecated - Phase 1
void InputController::handleButtonInterrupt()
{
  if (instancePtr)
  {
    instancePtr->button.tick();
  }
}
*/

InputController::InputController(uint8_t encoderPinA, uint8_t encoderPinB) // Modified for no button
    : encoder(encoderPinA, encoderPinB, RotaryEncoder::LatchMode::TWO03),
      lastPosition(0),
      encoderPinA(encoderPinA),
      encoderPinB(encoderPinB)
{

  // Attach click, double-click, and long-press handlers using OneButton library
  // button.attachClick([](void *scope)
  //                    { static_cast<InputController *>(scope)->onButtonClick(); }, this); // Button deprecated
  // button.attachDoubleClick([](void *scope)
  //                          { static_cast<InputController *>(scope)->onButtonDoubleClick(); }, this); // Button deprecated
  // button.attachLongPressStart([](void *scope)
  //                             { static_cast<InputController *>(scope)->onButtonLongPress(); }, this); // Button deprecated

  instancePtr = this; // Set the global instance pointer to this instance
}

void InputController::begin()
{
  // button.setDebounceMs(20); // Button deprecated
  // button.setClickMs(150); // Button deprecated
  // button.setPressMs(400); // Button deprecated
  lastPosition = encoder.getPosition();

  // pinMode(buttonPin, INPUT_PULLUP); // Button deprecated
  pinMode(encoderPinA, INPUT_PULLUP);
  pinMode(encoderPinB, INPUT_PULLUP);

  // Set up interrupts for encoder handling
  attachInterrupt(digitalPinToInterrupt(encoderPinA), handleEncoderInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(encoderPinB), handleEncoderInterrupt, CHANGE);

  // Set up interrupt for button handling
  // attachInterrupt(digitalPinToInterrupt(buttonPin), handleButtonInterrupt, CHANGE); // Button deprecated
}

void InputController::update()
{
  // button.tick(); // Button deprecated
  encoder.tick(); // Still needed if not relying purely on ISR, or for certain encoder types/modes.
                  // Per plan, encoder is ISR-driven but keeping tick() call here is fine as it is often harmless or beneficial.

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
/* // Button deprecated - Phase 1
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
*/

void InputController::onEncoderRotateHandler(std::function<void(int delta)> handler)
{
  encoderRotateHandler = handler;
}

// Method to release all handlers
void InputController::releaseHandlers()
{
  // pressHandler = nullptr; // Button deprecated
  // doublePressHandler = nullptr; // Button deprecated
  // longPressHandler = nullptr; // Button deprecated
  encoderRotateHandler = nullptr;

  // button.reset(); // Button deprecated
  lastPosition = encoder.getPosition(); // Reset encoder position tracking
}

// Internal event handlers that call the registered state handlers
/* // Button deprecated - Phase 1
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
*/

void InputController::onEncoderRotate(int delta)
{
  if (encoderRotateHandler != nullptr)
  {
    encoderRotateHandler(-delta); // Pass delta to the handler
  }
}

int InputController::getEncoderPosition() // Added getter for encoder position
{
  return encoder.getPosition();
}

// Global instance initialization is typically done in main.cpp or similar
// InputController inputController(ENCODER_A_PIN, ENCODER_B_PIN); // Example, actual pins from Config.h
