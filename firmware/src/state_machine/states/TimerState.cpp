#include "TimerState.h"
#include "../include/StateMachine.h"
#include "DoneState.h"
#include "PausedState.h"
#include "../include/LEDController.h"
#include "../../ProjectManager.h"

TimerState::TimerState() : 
  startTime(0), 
  pausedTime(0), 
  totalDuration(25), 
  isRunning(false),
  lastDisplayUpdate(0),
  lastProgressUpdate(0),
  selectedProjectId(""),
  currentPhase(STARTING)
{
}

TimerState::~TimerState()
{
}

bool TimerState::validateStateEntry() const
{
  // Validate timer duration is reasonable
  int duration = stateMachine.getPendingDuration();
  if (duration < 1 || duration > 240) { // 1-240 minutes
    ESP_LOGE(getLogTag(), "Invalid timer duration: %d minutes", duration);
    return false;
  }
  
  // Validate project selection if required
  String projectId = stateMachine.getPendingProjectId();
  if (projectId.length() == 0) {
    ESP_LOGW(getLogTag(), "No project selected - using default");
  }
  
  ESP_LOGD(getLogTag(), "Timer validation passed - %d min, project: %s", 
           duration, projectId.c_str());
  return true;
}

bool TimerState::canTransitionTo(const State* nextState) const
{
  if (!nextState) {
    return false;
  }
  
  const char* nextStateName = nextState->getStateName();
  
  // TimerState can transition to pause, done, idle (end timer), or sleep states
  if (strcmp(nextStateName, "PausedState") == 0 ||
      strcmp(nextStateName, "DoneState") == 0 ||
      strcmp(nextStateName, "IdleState") == 0 ||
      strcmp(nextStateName, "SleepState") == 0) {
    return true;
  }
  
  ESP_LOGW(getLogTag(), "Invalid transition from TimerState to %s", nextStateName);
  return false;
}

void TimerState::onEnter()
{
  // Check if this is a resume from pause or a fresh start
  bool isResuming = isRunning; // If already running, we're resuming
  
  if (!isResuming) {
    // Fresh start
    totalDuration = stateMachine.getPendingDuration();
    selectedProjectId = stateMachine.getPendingProjectId();
    
    startTime = millis();
    pausedTime = 0;
    isRunning = true;
    lastDisplayUpdate = 0;
    lastProgressUpdate = 0;
    currentPhase = STARTING;
    
    ESP_LOGI(getLogTag(), "Starting %d-minute timer for project: %s", 
             totalDuration, selectedProjectId.c_str());
  } else {
    // Resuming - timer state is preserved, just restart the running flag
    isRunning = true;
    ESP_LOGI(getLogTag(), "Resuming timer with %lu seconds remaining", getRemainingSeconds());
  }
  
  initializeTimer();
}

void TimerState::onUpdate()
{
  if (!isRunning) {
    ESP_LOGW(getLogTag(), "Update called while timer not running");
    return;
  }
  
  // Check for timer completion first
  checkTimerCompletion();
  if (!isRunning) return; // Timer completed
  
  // Update displays
  updateCountdownDisplay();
  updateLEDProgress();
  
  // Handle input events
  handleTimerEvents();
  
  // Yield to other tasks
  yield();
}

void TimerState::onExit()
{
  isRunning = false;
  
  // Calculate final elapsed time
  unsigned long elapsed = getElapsedSeconds();
  stateMachine.setPendingElapsedTime(elapsed);
  
  // Clean up input handler
  InputController* inputController = stateMachine.getInputController();
  if (inputController) {
    inputController->releaseHandlers();
  }
  
  // Turn off LED progress
  LEDController* ledController = stateMachine.getLEDController();
  if (ledController) {
    ledController->turnOff();
  }
  
  ESP_LOGI(getLogTag(), "Timer stopped - %lu seconds elapsed (%.1f%% complete)", 
           elapsed, getProgressPercentage());
}

void TimerState::initializeTimer()
{
  ESP_LOGD(getLogTag(), "Initializing timer display and controls");
  
  // Initialize LED progress ring
  LEDController* ledController = stateMachine.getLEDController();
  if (ledController) {
    // Start timer progress animation with project color
    uint32_t projectColor = ProjectManager::getInstance().getSelectedProjectColor();
    ledController->startTimerProgress(projectColor, 0.0f); // Project color, 0% progress
    ESP_LOGD(getLogTag(), "LED timer progress initialized with project color");
  }
  
  // Set up encoder button handler for pause functionality
  InputController* inputController = stateMachine.getInputController();
  if (inputController) {
    inputController->onEncoderButtonHandler([this]() {
      ESP_LOGI(getLogTag(), "Timer paused by button press");
      stateMachine.changeState(stateMachine.pausedState);
    });
    ESP_LOGD(getLogTag(), "Button handler for pause registered");
  }
  
  // TODO: Initialize countdown display with totalDuration
  // TODO: Set up touch event handlers
  // TODO: Apply project color scheme if available
  
  ESP_LOGD(getLogTag(), "Timer initialization complete");
}

void TimerState::updateCountdownDisplay()
{
  unsigned long currentTime = millis();
  
  // Update display every second
  if (currentTime - lastDisplayUpdate >= 1000) {
    unsigned long remaining = getRemainingSeconds();
    
    // Update phase based on remaining time
    TimerPhase newPhase = RUNNING;
    if (remaining <= 60) newPhase = CRITICAL;        // Last minute
    else if (remaining <= 300) newPhase = WARNING;   // Last 5 minutes
    
    if (newPhase != currentPhase) {
      currentPhase = newPhase;
      ESP_LOGI(getLogTag(), "Timer phase changed to %d, %lu seconds remaining", 
               currentPhase, remaining);
      // TODO: Update display styling based on phase
    }
    
    // Format time as MM:SS
    int minutes = remaining / 60;
    int seconds = remaining % 60;
    
    ESP_LOGD(getLogTag(), "Timer display: %02d:%02d (%lu sec remaining)", 
             minutes, seconds, remaining);
    
    // TODO: Update LVGL countdown display
    
    lastDisplayUpdate = currentTime;
  }
}

void TimerState::updateLEDProgress()
{
  unsigned long currentTime = millis();
  
  // Update LED progress every 250ms for smooth animation
  if (currentTime - lastProgressUpdate >= 250) {
    float progress = getProgressPercentage() / 100.0f; // Convert to 0.0-1.0 range
    
    ESP_LOGV(getLogTag(), "LED progress: %.1f%%", progress * 100.0f);
    
    // Update LED ring progress with project color, modified by timer phase
    LEDController* ledController = stateMachine.getLEDController();
    if (ledController) {
      // Get project color
      ProjectManager& pm = ProjectManager::getInstance();
      uint32_t baseColor = pm.getSelectedProjectColor();
      
      // Use project color throughout all timer phases
      uint32_t color = baseColor;
      
      ledController->startTimerProgress(color, progress);
    }
    
    lastProgressUpdate = currentTime;
  }
}

void TimerState::checkTimerCompletion()
{
  unsigned long remaining = getRemainingSeconds();
  
  if (remaining == 0) {
    unsigned long totalSeconds = totalDuration; // Already in seconds
    ESP_LOGI(getLogTag(), "Timer completed! %d minutes elapsed", totalDuration);
    
    stateMachine.setPendingElapsedTime(totalSeconds);
    stateMachine.changeState(stateMachine.doneState);
    isRunning = false;
  }
}

void TimerState::handleTimerEvents()
{
  // Button handling is done via callback in initializeTimer()
  
  // TODO: Check for touch events
  // - Tap to pause (transition to PausedState)
  // - Long press to end timer early (transition to DoneState)
  
  // TODO: Check for power button press (transition to SleepState)
}

unsigned long TimerState::getRemainingSeconds() const
{
  unsigned long elapsed = getElapsedSeconds();
  unsigned long totalSeconds = totalDuration; // Already in seconds
  
  if (elapsed >= totalSeconds) return 0;
  return totalSeconds - elapsed;
}

unsigned long TimerState::getElapsedSeconds() const
{
  if (startTime == 0) return 0; // Not started yet
  
  unsigned long currentTime = millis();
  unsigned long totalElapsed = currentTime - startTime;
  
  // Subtract total paused time
  if (totalElapsed >= pausedTime) {
    return (totalElapsed - pausedTime) / 1000;
  }
  return 0;
}

float TimerState::getProgressPercentage() const
{
  if (totalDuration == 0) return 0.0f;
  
  unsigned long elapsed = getElapsedSeconds();
  unsigned long totalSeconds = totalDuration; // Already in seconds
  
  return (float(elapsed) / float(totalSeconds)) * 100.0f;
}

void TimerState::pauseTimer()
{
  isRunning = false;
  ESP_LOGI(getLogTag(), "Timer paused at %lu seconds elapsed", getElapsedSeconds());
}

void TimerState::resumeTimer(unsigned long additionalPausedTime)
{
  pausedTime += additionalPausedTime;
  isRunning = true;
  ESP_LOGI(getLogTag(), "Timer resumed - total paused time: %lu seconds", pausedTime / 1000);
}