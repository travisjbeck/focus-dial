#include "ProvisionState.h"
#include "../include/StateMachine.h"

ProvisionState::ProvisionState() : apModeStarted(false)
{
}

ProvisionState::~ProvisionState()
{
}

void ProvisionState::onEnter()
{
  apModeStarted = false;
  
  ESP_LOGI(getLogTag(), "Starting WiFi provisioning");
  
  // TODO: Start AP mode
  // TODO: Show provisioning UI
}

void ProvisionState::onUpdate()
{
  // TODO: Handle WiFi credential entry
  // TODO: Check for successful connection
  // TODO: Transition to IdleState when done
  
  yield();
}

void ProvisionState::onExit()
{
  ESP_LOGI(getLogTag(), "WiFi provisioning complete");
}