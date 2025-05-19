#pragma once

#include "Config.h"
#include <Arduino.h> // For millis()

// Base class
class State
{
public:
  virtual ~State() {}

  virtual void enter() {
    entryTime = millis();
  };
  virtual void update() = 0;
  virtual void exit() = 0;

  unsigned long getEntryTime() const { return entryTime; } // Public getter
  static const unsigned long TAP_DEBOUNCE_MS = 300; // Made public

protected:
  unsigned long entryTime; // Timestamp of when the state was entered
  // static const unsigned long TAP_DEBOUNCE_MS = 300; // Moved to public
};