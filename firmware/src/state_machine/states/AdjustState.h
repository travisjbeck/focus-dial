#pragma once

#include "../include/State.h"

class AdjustState : public State
{
public:
  AdjustState();
  virtual ~AdjustState();
  
  const char* getStateName() const override { return "AdjustState"; }

protected:
  void onEnter() override;
  void onUpdate() override;
  void onExit() override;
  const char* getLogTag() const override { return "AdjustState"; }

private:
  int currentDuration; // In seconds
  unsigned long lastEncoderTime;
  int lastEncoderPosition; // Track encoder position for this state
  int gradientOffset; // For LED rotation effect
  
  static const unsigned long ADJUST_TIMEOUT_MS = 10000; // 10 seconds timeout
};