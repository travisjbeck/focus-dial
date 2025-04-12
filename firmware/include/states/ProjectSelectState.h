#pragma once

#include "State.h"
#include "managers/ProjectManager.h" // To access projects
#include "Controllers.h"             // To control display/LEDs/input
#include <vector>

// Forward declarations if needed
class StateMachine;
class InputController;

class ProjectSelectState : public State
{
public:
  ProjectSelectState(StateMachine &sm, DisplayController &display, LEDController &leds, InputController &input, ProjectManager &pm);

  void enter() override;
  void update() override;
  void exit() override;

private:
  // Restore controller references needed by the state
  StateMachine &stateMachine;
  DisplayController &displayController;
  LEDController &ledController;
  InputController &inputController;

  ProjectManager &projectManager; // Reference to access projects
  int selectedProjectIndex;       // Currently highlighted project index (0 for "No Project")
  int numProjectsToShow;          // How many projects fit on the screen (now 1)
  ProjectList projectsWithNone;   // Local copy including the "No Project" option
  bool needsInitialRender;        // Flag for first update draw
  unsigned long lastActivityTime; // For timeout

  // Helper methods
  void loadProjects();
  void renderDisplay();
  void updateLedColor();
  void handleInput();
};