#include "IdleState.h"
#include "../include/StateMachine.h"
#include "AdjustState.h"
#include "../../SimpleEncoder.h"
#include "../../BrandColors.h"
#include "../../LEDGradient.h"
#include "../../ProjectManager.h"
#include <Adafruit_NeoPixel.h>

extern SimpleEncoder simpleEncoder;
extern Adafruit_NeoPixel pixels; // Access to raw NeoPixel object

IdleState::IdleState() : 
  lastTouchTime(0), 
  displayInitialized(false),
  timerDisplayShown(false),
  currentDisplayDuration(25)
{
}

IdleState::~IdleState()
{
}

bool IdleState::validateStateEntry() const
{
  // Validate that we have access to the state machine
  if (&stateMachine == nullptr) {
    ESP_LOGE(getLogTag(), "State machine reference is null");
    return false;
  }
  
  // Validate display system is ready
  // TODO: Add actual display validation when LVGL is integrated
  
  ESP_LOGD(getLogTag(), "State entry validation passed");
  return true;
}

bool IdleState::canTransitionTo(const State* nextState) const
{
  if (!nextState) {
    return false;
  }
  
  const char* nextStateName = nextState->getStateName();
  
  // IdleState can transition to most states
  if (strcmp(nextStateName, "AdjustState") == 0 ||
      strcmp(nextStateName, "ProjectSelectState") == 0 ||
      strcmp(nextStateName, "ProvisionState") == 0 ||
      strcmp(nextStateName, "SleepState") == 0) {
    return true;
  }
  
  ESP_LOGW(getLogTag(), "Invalid transition from IdleState to %s", nextStateName);
  return false;
}

void IdleState::onEnter()
{
  lastTouchTime = 0;
  displayInitialized = false;
  timerDisplayShown = false;
  
  // Get current timer duration from state machine
  currentDisplayDuration = stateMachine.getPendingDuration();
  
  ESP_LOGI(getLogTag(), "Initializing idle display with %d second timer", currentDisplayDuration);
  
  // Set LED to breathing effect with project color
  LEDController* ledController = stateMachine.getLEDController();
  if (ledController) {
    // Get current project color from ProjectManager
    uint32_t projectColor = ProjectManager::getInstance().getSelectedProjectColor();
    ledController->startBreath(projectColor, -1, 3000); // Infinite breathing, 3 second cycle
  }
  
  initializeDisplay();
}

void IdleState::onUpdate()
{
  if (!displayInitialized) {
    ESP_LOGW(getLogTag(), "Update called before display initialization");
    return;
  }
  
  // Update timer display if duration changed
  updateTimerDisplay();
  
  // Handle input events
  handleTouchInput();
  handleEncoderInput();
  
  // Yield to other tasks
  yield();
}

void IdleState::onExit()
{
  ESP_LOGI(getLogTag(), "Cleaning up idle display");
  displayInitialized = false;
  timerDisplayShown = false;
}

void IdleState::initializeDisplay()
{
  ESP_LOGD(getLogTag(), "Setting up LVGL screen for idle display");
  
  // TODO: Initialize LVGL screen with timer display
  // TODO: Set up touch event handlers
  // TODO: Configure screen layout
  
  displayInitialized = true;
  ESP_LOGD(getLogTag(), "Display initialization complete");
}

void IdleState::updateTimerDisplay()
{
  int newDuration = stateMachine.getPendingDuration();
  
  if (newDuration != currentDisplayDuration || !timerDisplayShown) {
    currentDisplayDuration = newDuration;
    
    ESP_LOGD(getLogTag(), "Updating timer display to show %d seconds", currentDisplayDuration);
    
    // TODO: Update LVGL timer display with new duration
    // Format: MM:SS (e.g., "25:00")
    
    timerDisplayShown = true;
  }
}

void IdleState::handleTouchInput()
{
  // TODO: Check for touch events
  // On tap: transition to ProjectSelectState
  
  unsigned long currentTime = millis();
  if (currentTime - lastTouchTime < TAP_DEBOUNCE_MS) {
    return; // Debounce protection
  }
  
  // Simulate touch detection for now
  // if (touchDetected) {
  //   lastTouchTime = currentTime;
  //   ESP_LOGI(getLogTag(), "Touch detected - transitioning to ProjectSelectState");
  //   stateMachine.changeState(static_cast<State*>(stateMachine.projectSelectState));
  // }
}

void IdleState::handleEncoderInput()
{
  // Check for encoder rotation using simple encoder
  int delta = simpleEncoder.readDelta();
  
  if (delta != 0) {
    ESP_LOGI(getLogTag(), "Encoder rotation detected (delta: %d) - transitioning to AdjustState", delta);
    
    // Apply the initial encoder change to the duration
    int newDuration = stateMachine.getPendingDuration() + (delta * 5 * 60); // 5 minutes in seconds
    
    // Constrain duration between 0 and 120 minutes (2 hours)
    if (newDuration < 0) newDuration = 0;
    if (newDuration > 120 * 60) newDuration = 120 * 60; // 120 minutes in seconds
    
    // Round to nearest 5 minutes (300 seconds)
    newDuration = ((newDuration + 150) / 300) * 300;
    
    // Set the new duration before transitioning
    stateMachine.setPendingDuration(newDuration);
    
    // Don't reset encoder - let it continue naturally
    stateMachine.changeState(stateMachine.adjustState);
  }
}