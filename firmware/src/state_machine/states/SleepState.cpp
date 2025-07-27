#include "SleepState.h"
#include "../include/StateMachine.h"

SleepState::SleepState() : sleepInitiated(false)
{
}

SleepState::~SleepState()
{
}

void SleepState::onEnter()
{
  sleepInitiated = false;
  
  ESP_LOGI(getLogTag(), "Preparing for sleep");
  
  // TODO: Save current state to NVS
  // TODO: Configure wake sources
  // TODO: Turn off display
}

void SleepState::onUpdate()
{
  if (!sleepInitiated) {
    ESP_LOGI(getLogTag(), "Entering deep sleep");
    
    // TODO: Actually enter sleep mode
    // For now, just log
    sleepInitiated = true;
    
    // In real implementation, this would call esp_deep_sleep_start()
    // and execution would stop here
  }
  
  yieldMs(100);
}

void SleepState::onExit()
{
  ESP_LOGI(getLogTag(), "Waking up");
  
  // TODO: Restore state from NVS
  // TODO: Reinitialize display
}