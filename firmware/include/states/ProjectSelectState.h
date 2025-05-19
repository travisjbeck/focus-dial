#pragma once

#include "State.h"
#include "managers/ProjectManager.h" // To access projects
#include "Controllers.h"             // To control display/LEDs/input
#include <vector>
#include <lvgl.h> // Added for LVGL types

// Forward declarations if needed
class StateMachine;
// class InputController; // Already included via Controllers.h indirectly or directly if DisplayController etc. don't pull it in

class ProjectSelectState : public State
{
public:
  ProjectSelectState(StateMachine &sm, DisplayController &display, LEDController &leds, InputController &input, ProjectManager &pm);

  void enter() override;
  void update() override;
  void exit() override;

  // Public helper methods for LVGL event callbacks
  void processRollerValueChange(lv_obj_t* roller_obj);
  void processScreenTap();
  void processScreenLongPress(); // Added for long press back
  // void processCancelPress(); // No more cancel button, back via timeout or future gesture

private:
  // Controller references
  StateMachine &stateMachine;
  DisplayController &displayController;
  LEDController &ledController;
  InputController &inputController;
  ProjectManager &projectManager;

  // State Variables
  int selectedProjectIndex;
  // int numProjectsToShow; // No longer needed as roller handles visibility
  ProjectList projectsWithNone;
  bool needsInitialRender;
  unsigned long lastActivityTime;

  // LVGL UI Object pointers
  lv_obj_t *titleLabel; // Added for the title
  lv_obj_t *roller;
  // lv_obj_t *confirmButton; // Removed
  // lv_obj_t *cancelButton;  // Removed

  // Helper methods
  void loadProjects();
  void renderDisplay(); // May become obsolete or change role
  void updateLedColor();
  void handleInput();

  // Static event handlers (can be declared here or defined as static in .cpp before use)
  // For simplicity if they are only used by this class and defined in its .cpp, explicit declaration here might not be needed.
  // However, it can be good practice for clarity if they were more complex or needed by other .cpp files via this header.

  static const unsigned long PROJECT_SELECT_TIMEOUT_MS = 30000; // Timeout for this state (30 seconds)
};