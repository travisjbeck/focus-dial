#include "StartupState.h"
#include "../include/StateMachine.h"
#include "IdleState.h"
#include "ProvisionState.h"
#include <Preferences.h>

StartupState::StartupState()
{
}

StartupState::~StartupState()
{
}

void StartupState::onEnter()
{
  ESP_LOGI(getLogTag(), "Showing splash screen");
  
  // Set LED to brand color spinner animation for startup
  LEDController* ledController = stateMachine.getLEDController();
  if (ledController) {
    ledController->startSpinner(0xc2e189, -1); // Full brightness green spinner (brand color)
  }
  
  // TODO: Show splash screen
  // TODO: Initialize hardware
}

void StartupState::onUpdate()
{
  // Check if splash duration has elapsed
  if (millis() - getEntryTime() >= SPLASH_DURATION) {
    ESP_LOGI(getLogTag(), "Splash complete, starting timer application");
    
    // Check if WiFi is configured
    Preferences preferences;
    preferences.begin("wifi", true);
    bool wifiConfigured = preferences.getBool("configured", false);
    preferences.end();
    
    if (!wifiConfigured) {
        ESP_LOGI(getLogTag(), "WiFi not configured, transitioning to ProvisionState");
        stateMachine.changeState(stateMachine.provisionState);
    } else {
        ESP_LOGI(getLogTag(), "WiFi already configured, transitioning to IdleState");
        stateMachine.changeState(stateMachine.idleState);
    }
  }
  
  yield();
}

void StartupState::onExit()
{
  ESP_LOGI(getLogTag(), "Startup complete");
}