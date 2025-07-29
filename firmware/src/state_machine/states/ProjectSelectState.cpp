#include "ProjectSelectState.h"
#include "../include/StateMachine.h"
#include "../../SimpleEncoder.h"
#include "../include/LEDController.h"
#include "../../ui/ScreenManager.h"
#include "../../ProjectManager.h"

extern SimpleEncoder simpleEncoder;
extern ScreenManager screenManager;

ProjectSelectState::ProjectSelectState() : selectedIndex(0), projectCount(0), lastActivityTime(0)
{
}

ProjectSelectState::~ProjectSelectState()
{
}

void ProjectSelectState::onEnter()
{
  // Get the selected project from StateMachine (now uses same indexing as ProjectManager)
  selectedIndex = stateMachine.getSelectedProjectIndex();
  ProjectManager& pm = ProjectManager::getInstance();
  projectCount = pm.getProjectCount();
  
  ESP_LOGI(getLogTag(), "Loading project selection with index: %d", selectedIndex);
  ESP_LOGI(getLogTag(), "Showing project list with %d projects", projectCount);
  
  // Set LED to match current project color
  LEDController* ledController = stateMachine.getLEDController();
  if (ledController) {
    // Get project color directly from ProjectManager (authoritative source)
    uint32_t projectColor = ProjectManager::getInstance().getSelectedProjectColor();
    ledController->setSolid(projectColor);
  }
  
  // Reset encoder position to current selection
  simpleEncoder.setPosition(selectedIndex);
  
  // Initialize activity timer
  lastActivityTime = millis();
}

void ProjectSelectState::onUpdate()
{
  // Check for timeout - return to IdleState after 30 seconds of inactivity
  if (millis() - lastActivityTime > TIMEOUT_DURATION_MS) {
    ESP_LOGI(getLogTag(), "Project selection timeout after 30 seconds - returning to IdleState");
    stateMachine.changeState((State*)stateMachine.idleState);
    return;
  }
  
  // Handle encoder rotation for scrolling
  int delta = simpleEncoder.readDelta();
  
  if (delta != 0) {
    ESP_LOGI(getLogTag(), "Encoder delta detected: %d (was at index %d)", delta, selectedIndex);
    
    // Reset activity timer on encoder input
    lastActivityTime = millis();
    
    // Update selected index with wrapping
    selectedIndex += delta;
    if (selectedIndex < 0) selectedIndex = projectCount - 1;
    if (selectedIndex >= projectCount) selectedIndex = 0;
    
    ESP_LOGI(getLogTag(), "Selected project index changed to: %d", selectedIndex);
    
    // Update the display
    screenManager.updateProjectDisplay(selectedIndex);
    
    // Update LED color based on selection
    LEDController* ledController = stateMachine.getLEDController();
    if (ledController) {
      // Get updated project color from ScreenManager (which has the current selection)
      ledController->setSolid(screenManager.getCurrentProjectColor());
    }
  }
  
  // Tap handling is done in UIEventHandler
  
  yield();
}

void ProjectSelectState::onExit()
{
  // Store selected project index - all systems now use the same 0-5 indexing
  ESP_LOGI(getLogTag(), "EXITING: selectedIndex=%d", selectedIndex);
  ESP_LOGI(getLogTag(), "Selected project: %s (index %d)", 
           screenManager.getCurrentProjectName(), selectedIndex);
  
  // Store in state machine (now uses same indexing as ProjectManager)
  stateMachine.setSelectedProjectIndex(selectedIndex);
  
  // Also update the ProjectManager to keep it synchronized
  ProjectManager& pm = ProjectManager::getInstance();
  pm.selectProject(selectedIndex);
  
  // Update the ScreenManager's project index to match
  screenManager.updateProjectDisplay(selectedIndex);
}

void ProjectSelectState::resetActivityTimer() {
  lastActivityTime = millis();
  ESP_LOGD(getLogTag(), "Activity timer reset due to touch input");
}