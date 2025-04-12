#pragma once

#include "State.h"

class TimerState : public State
{
public:
  TimerState();

  void enter() override;
  void update() override;
  void exit() override;

  void setTimer(int duration, unsigned long elapsedTime);

private:
  unsigned long startTime;
  int duration;              // Total duration in minutes
  unsigned long elapsedTime; // Elapsed time in seconds
  uint32_t currentLedColor;  // Store the color for this timer session
};