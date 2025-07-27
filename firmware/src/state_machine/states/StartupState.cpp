#include "StartupState.h"
#include "../include/StateMachine.h"
#include "IdleState.h"
#include "ProvisionState.h"

StartupState::StartupState()
{
}

StartupState::~StartupState()
{
}

void StartupState::onEnter()
{
  ESP_LOGI(getLogTag(), "Showing splash screen");
  
  // Set LED to teal spinner animation for startup
  LEDController* ledController = stateMachine.getLEDController();
  if (ledController) {
    ledController->startSpinner(0x00FFFF, -1); // Full brightness teal spinner
  }
  
  // TODO: Show splash screen
  // TODO: Initialize hardware
}

void StartupState::onUpdate()
{
  // Check if splash duration has elapsed
  if (millis() - getEntryTime() >= SPLASH_DURATION) {
    ESP_LOGI(getLogTag(), "Splash complete, starting timer application");
    
    // For timer application, skip WiFi provisioning and go directly to IdleState
    // TODO: Later add WiFi configuration check if network features are needed
    ESP_LOGI(getLogTag(), "Transitioning to IdleState for timer operation");
    stateMachine.changeState(stateMachine.idleState);
  }
  
  yield();
}

void StartupState::onExit()
{
  ESP_LOGI(getLogTag(), "Startup complete");
}