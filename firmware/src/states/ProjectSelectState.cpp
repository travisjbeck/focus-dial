#include "states/ProjectSelectState.h"
#include "StateMachine.h"
#include "Controllers.h"

#define PROJECT_SELECT_TIMEOUT 30000 // 30 seconds

ProjectSelectState::ProjectSelectState(StateMachine &sm, DisplayController &display, LEDController &leds, InputController &input, ProjectManager &pm)
    : stateMachine(sm),
      displayController(display),
      ledController(leds),
      inputController(input),
      projectManager(pm),
      selectedProjectIndex(0),
      numProjectsToShow(1),
      needsInitialRender(true),
      lastActivityTime(0) // Initialize
{
  // Constructor implementation (if needed)
  // Note: sm, display, leds, input are not stored as members here,
  // assuming they are accessed via global instances (e.g., displayController)
}

void ProjectSelectState::enter()
{
  Serial.println("Entering Project Select State");

  // 1. Load projects and prepend "No Project"
  loadProjects(); // Use helper

  // 2. Determine initial selection (from last used)
  int lastUsedIndex = projectManager.getLastProjectIndex();
  if (lastUsedIndex >= 0 && (lastUsedIndex + 1) < projectsWithNone.size())
  {
    selectedProjectIndex = lastUsedIndex + 1;
  }
  else
  {
    selectedProjectIndex = 0; // Default to "No Project"
  }
  Serial.printf("Initial selected index: %d\n", selectedProjectIndex);

  // 3. Register Input Handlers
  handleInput(); // Use helper

  needsInitialRender = true;   // Set flag for first update
  lastActivityTime = millis(); // Reset activity timer on entry
}

void ProjectSelectState::update()
{
  // Draw the screen on the very first update cycle
  if (needsInitialRender)
  {
    renderDisplay();
    needsInitialRender = false; // Clear flag
  }

  // Original Update logic:
  inputController.update(); // Process inputs which trigger handlers

  // Check for timeout
  if (millis() - lastActivityTime >= PROJECT_SELECT_TIMEOUT)
  {
    Serial.println("ProjectSelectState: Timeout - Returning to Idle");
    stateMachine.changeState(&StateMachine::idleState);
  }
}

void ProjectSelectState::exit()
{
  Serial.println("Exiting Project Select State");
  inputController.releaseHandlers(); // Important!
  ledController.turnOff();           // Turn off project color LED
}

// --- Helper Methods ---

void ProjectSelectState::renderDisplay()
{
  // Draw the screen
  displayController.drawProjectSelectionScreen(projectsWithNone, selectedProjectIndex, 0, numProjectsToShow);
  // Update LED to match selection
  updateLedColor();
}

void ProjectSelectState::updateLedColor()
{
  // Get color from projectsWithNone[selectedProjectIndex]
  // Convert hex to uint32_t using LEDController::hexColorToUint32
  // Set LED color using ledController.setSolid() or similar
  if (selectedProjectIndex >= 0 && selectedProjectIndex < projectsWithNone.size())
  {
    uint32_t color = LEDController::hexColorToUint32(projectsWithNone[selectedProjectIndex].color);
    ledController.setSolid(color);
  }
  else
  {
    ledController.turnOff(); // Should not happen
  }
}

void ProjectSelectState::loadProjects()
{
  projectsWithNone.clear();
  Project noProject = {"No Project", "#FF0000"}; // Red for no project
  projectsWithNone.push_back(noProject);
  const auto &actualProjects = projectManager.getProjects();
  projectsWithNone.insert(projectsWithNone.end(), actualProjects.begin(), actualProjects.end());
}

void ProjectSelectState::handleInput()
{
  inputController.onPressHandler([this]()
                                 {
    Serial.println("ProjectSelectState: Button pressed - Confirming project");
    int duration = stateMachine.getPendingDuration();
    int indexToSave = (selectedProjectIndex == 0) ? -1 : selectedProjectIndex - 1;
    projectManager.setLastProjectIndex(indexToSave);
    Serial.printf("Selected project index %d (saved as %d)\n", selectedProjectIndex, indexToSave);
    String selectedName = "";
    String selectedColor = "#FFFFFF"; // Default to White
    if (selectedProjectIndex == 0) { /* No project */ }
    else if (selectedProjectIndex > 0 && selectedProjectIndex < projectsWithNone.size()) {
        selectedName = projectsWithNone[selectedProjectIndex].name;
        selectedColor = projectsWithNone[selectedProjectIndex].color;
    } else { selectedName = "Error"; selectedColor = "#FF0000"; }
    stateMachine.setPendingProject(selectedName, selectedColor);
    StateMachine::timerState.setTimer(duration, 0);
    displayController.showTimerStart();
    stateMachine.changeState(&StateMachine::timerState); });

  // Add double-click handler to go back to Idle
  inputController.onDoublePressHandler([this]()
                                       {
    Serial.println("ProjectSelectState: Double click - Returning to Idle");
    stateMachine.changeState(&StateMachine::idleState); });

  inputController.onEncoderRotateHandler([this](int delta)
                                         {
                                           int listSize = projectsWithNone.size();
                                           if (listSize == 0)
                                             return;

                                           selectedProjectIndex += delta;                                                  // Add delta (assuming positive is clockwise/down)
                                           selectedProjectIndex = (selectedProjectIndex % listSize + listSize) % listSize; // Modulo for wrapping

                                           Serial.printf("ProjectSelectState: Encoder Delta: %d, Selected: %d\n", delta, selectedProjectIndex);

                                           renderDisplay();             // Redraw with new selection
                                           lastActivityTime = millis(); // Reset activity timer on encoder rotate
                                         });
}