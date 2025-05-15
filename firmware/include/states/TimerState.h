#pragma once

#include "State.h"
#include <lvgl.h> // Added for LVGL types

class TimerState : public State
{
public:
  TimerState();

  void enter() override;
  void update() override;
  void exit() override;

  void setTimer(int duration, unsigned long elapsedTime);

  // Public helper methods for LVGL events
  void processScreenTap();
  void processScreenLongPress();

private:
  unsigned long startTime;
  int duration;                  // Total duration in minutes
  unsigned long elapsedTime;     // Elapsed time in seconds
  uint32_t currentLedColor;      // Store the color for this timer session
  String currentProjectName;     // Store the name for this session
  // String currentProjectColorHex; // Not strictly needed if currentLedColor is stored

  // LVGL UI Object Pointers
  lv_obj_t *projectNameLabel;
  lv_obj_t *timeDisplayLabel;
  lv_obj_t *timerProgressBar; // Optional, for visual progress
};