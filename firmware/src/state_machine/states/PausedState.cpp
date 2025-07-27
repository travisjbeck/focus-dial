#include "PausedState.h"
#include "../include/StateMachine.h"
#include "TimerState.h"
#include "../../ui/ScreenManager.h"
#include "../include/LEDController.h"
#include "../../ProjectManager.h"

PausedState::PausedState() : pauseStartTime(0)
{
}

PausedState::~PausedState()
{
}

bool PausedState::canTransitionTo(const State* nextState) const
{
  if (!nextState) {
    return false;
  }
  
  const char* nextStateName = nextState->getStateName();
  
  // PausedState can transition to resume timer, end timer, or sleep
  if (strcmp(nextStateName, "TimerState") == 0 ||
      strcmp(nextStateName, "IdleState") == 0 ||
      strcmp(nextStateName, "DoneState") == 0 ||
      strcmp(nextStateName, "SleepState") == 0) {
    return true;
  }
  
  ESP_LOGW(getLogTag(), "Invalid transition from PausedState to %s", nextStateName);
  return false;
}

void PausedState::onEnter()
{
  pauseStartTime = millis();
  
  // Pause the timer state
  TimerState* timerState = static_cast<TimerState*>(stateMachine.timerState);
  if (timerState) {
    timerState->pauseTimer();
  }
  
  ESP_LOGI(getLogTag(), "Timer paused");
  
  // Show the paused screen with current progress
  if (timerState) {
    unsigned long remainingSeconds = timerState->getRemainingSeconds();
    float progress = timerState->getProgressPercentage();
    
    extern ScreenManager screenManager;
    screenManager.showPausedScreen(remainingSeconds, progress);
    
    // Start breathing LED animation with project color
    LEDController* ledController = stateMachine.getLEDController();
    if (ledController) {
      ProjectManager& pm = ProjectManager::getInstance();
      uint32_t projectColor = pm.getSelectedProjectColor();
      
      // Start slow breathing animation to indicate pause
      ledController->startBreath(projectColor, -1, 2000); // Infinite cycles, 2 second period
      
      ESP_LOGI(getLogTag(), "Started breathing LED animation with project color: 0x%06X", projectColor);
    }
  }
}

void PausedState::onUpdate()
{
  // Update the paused screen with breathing effect
  extern ScreenManager screenManager;
  screenManager.updatePausedBreathing();
  
  // Touch events are handled by UIEventHandler
  // - Tap to resume (return to TimerState)
  // - Long press to end timer (go to IdleState)
  
  yield();
}

void PausedState::onExit()
{
  // Calculate how long we were paused
  unsigned long pauseDuration = millis() - pauseStartTime;
  
  // Resume the timer state with the pause duration
  TimerState* timerState = static_cast<TimerState*>(stateMachine.timerState);
  if (timerState) {
    timerState->resumeTimer(pauseDuration);
  }
  
  ESP_LOGI(getLogTag(), "Resuming timer after %lu ms pause", pauseDuration);
}