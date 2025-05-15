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
  // Method to set all project details including ID
  void setCurrentProjectDetails(const String& id, const String& name, uint32_t color);

  // Public helper methods for LVGL events
  void processScreenTap();
  void processScreenLongPress();

private:
  unsigned long startTime;
  int duration;                  // Total duration in minutes
  unsigned long elapsedTime;     // Elapsed time in seconds
  
  // Store project details for the current session
  String currentProjectId;       
  String currentProjectName;     
  uint32_t currentLedColor;      

  // LVGL UI Object Pointers
  lv_obj_t *projectNameLabel;
  lv_obj_t *timeDisplayLabel;
  lv_obj_t *timerProgressBar; 
};