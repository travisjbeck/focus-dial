#pragma once

#include "State.h"
#include <Arduino.h> // For String
#include <lvgl.h>    // For LVGL types if we display project name/color here too

class PausedState : public State
{
public:
  PausedState();
  void enter() override;
  void update() override;
  void exit() override;

  // Updated to accept color and project name
  void setPause(int duration, unsigned long elapsedTime, uint32_t ledColor, const String& projectName);

  // Public helper methods for LVGL events (if needed for this state's UI)
  void processScreenTap(); // To resume
  void processScreenLongPress(); // To cancel

private:
  int duration;
  unsigned long pauseEnter; // Time when pause state was entered
  unsigned long elapsedTimeAtPause; // Elapsed time when pause began
  
  // Store color and name to pass back to TimerState or display
  uint32_t pausedLedColor;
  String pausedProjectName;

  // LVGL UI Object Pointers (if PausedState has its own distinct UI)
  lv_obj_t *pausedTimeLabel;
  lv_obj_t *pausedProjectNameLabel;
  lv_obj_t *instructionLabel; // e.g., "Tap to Resume / Long Press to Cancel"
};