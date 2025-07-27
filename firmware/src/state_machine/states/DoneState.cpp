#include "DoneState.h"
#include "../include/StateMachine.h"
#include "IdleState.h"
#include "../../ui/ScreenManager.h"

DoneState::DoneState() : elapsedTime(0), stateStartTime(0)
{
}

DoneState::~DoneState()
{
}

void DoneState::onEnter()
{
  elapsedTime = stateMachine.getPendingElapsedTime();
  stateStartTime = millis();
  
  ESP_LOGI(getLogTag(), "Timer completed in %lu seconds", elapsedTime);
  ESP_LOGI(getLogTag(), "Done screen will auto-transition to idle after %lu seconds", AUTO_TRANSITION_DELAY / 1000);
}

void DoneState::onUpdate()
{
  // Check for auto-transition timeout
  unsigned long currentTime = millis();
  if (currentTime - stateStartTime >= AUTO_TRANSITION_DELAY) {
    ESP_LOGI(getLogTag(), "Auto-transitioning to idle after 30 seconds");
    stateMachine.changeState(stateMachine.idleState);
    return;
  }
  
  // Update breathing animation
  extern ScreenManager screenManager;
  screenManager.updateDoneBreathing();
  
  // Touch handling is done via TouchManager
  yield();
}

void DoneState::onExit()
{
  ESP_LOGI(getLogTag(), "Returning to idle");
}