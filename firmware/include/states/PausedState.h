#ifndef PAUSED_STATE_H
#define PAUSED_STATE_H

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
  void setPause(const String& projectId, int totalDurationMinutes, unsigned long elapsedSeconds, uint32_t ledColor, const String& projectName);

  // Public helper methods for LVGL events (if needed for this state's UI)
  void processScreenTap(); // To resume
  void processScreenLongPress(); // To cancel

private:
  String activeProjectId;
  int originalDurationMinutes;
  unsigned long pausedElapsedTimeSeconds;
  uint32_t activeLedColor;
  String activeProjectName;

  // LVGL UI Object Pointers (if PausedState has its own distinct UI)
  lv_obj_t *pausedLabel;
  lv_obj_t *timeDisplayLabel;
  lv_obj_t *projectNameLabel;
  // For simplicity, let's make the whole screen tappable for resume, and a specific area/gesture for stop,
  // or use two distinct areas. For now, one tap to resume, long press to stop.
  // lv_obj_t* resumeButtonLabel; 
  // lv_obj_t* stopButtonLabel;

  static void screen_tap_event_handler(lv_event_t* e);
  static void screen_long_press_event_handler(lv_event_t* e);

  // Changed back to private as they are called by static handlers
  void processResume();
  void processStop();
};

#endif // PAUSED_STATE_H