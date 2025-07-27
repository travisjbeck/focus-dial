#include "AdjustState.h"
#include "../include/StateMachine.h"
#include "IdleState.h"
#include "../../SimpleEncoder.h"
#include "../../BrandColors.h"
#include "../../ProjectManager.h"

extern SimpleEncoder simpleEncoder;

AdjustState::AdjustState() : currentDuration(0), lastEncoderTime(0), lastEncoderPosition(0)
{
}

AdjustState::~AdjustState()
{
}

void AdjustState::onEnter()
{
  int pendingDuration = stateMachine.getPendingDuration();
  ESP_LOGI(getLogTag(), "ADJUST DEBUG: StateMachine.getPendingDuration() = %d", pendingDuration);
  
  currentDuration = pendingDuration;
  lastEncoderTime = millis(); // Initialize activity timer
  
  ESP_LOGI(getLogTag(), "ADJUST DEBUG: currentDuration set to %d", currentDuration);
  ESP_LOGI(getLogTag(), "Starting with duration %d seconds", currentDuration);
  
  // Initialize encoder position tracking
  InputController* inputController = stateMachine.getInputController();
  if (inputController) {
    // Position should be 0 after reset in IdleState
    lastEncoderPosition = 0;
    ESP_LOGI(getLogTag(), "Initial encoder position: %d", lastEncoderPosition);
  }
  
  // Set up button handler for saving duration
  if (inputController) {
    inputController->onEncoderButtonHandler([this]() {
      ESP_LOGI(getLogTag(), "Button pressed - saving duration and returning to IdleState");
      stateMachine.changeState(stateMachine.idleState);
    });
  }
  
  // Set LED to show duration gradient with project color
  LEDController* ledController = stateMachine.getLEDController();
  if (ledController) {
    // Get current project color and use it for the duration gradient
    uint32_t projectColor = ProjectManager::getInstance().getSelectedProjectColor();
    ledController->showDurationGradient(currentDuration / 60, 120, projectColor); // currentDuration is in seconds, convert to minutes
  }
  
  // Initialize gradient offset for rotation effect
  gradientOffset = 0;
}

void AdjustState::onUpdate()
{
  // Handle encoder rotation using simple encoder
  int delta = simpleEncoder.readDelta();
    
    if (delta != 0) {
      if (currentDuration == 0 && delta > 0) {
        // From indeterminate to 10 seconds
        currentDuration = 10;
      } else if (currentDuration == 10 && delta < 0) {
        // From 10 seconds back to indeterminate
        currentDuration = 0;
      } else if (currentDuration == 10 && delta > 0) {
        // From 10 seconds to 5 minutes
        currentDuration = 5 * 60;
      } else if (currentDuration == 5 * 60 && delta < 0) {
        // From 5 minutes back to 10 seconds
        currentDuration = 10;
      } else {
        // Normal adjustment by 5 minutes per encoder step
        currentDuration += (delta * 5 * 60); // 5 minutes in seconds
        
        // Constrain duration between 5 minutes and 120 minutes
        if (currentDuration < 5 * 60 && currentDuration != 10) {
          currentDuration = 5 * 60;
        }
        if (currentDuration > 120 * 60) {
          currentDuration = 120 * 60;
        }
        
        // Round to nearest 5 minutes (but not for special 10-second value)
        if (currentDuration != 10) {
          // Round to nearest 5-minute interval (300 seconds)
          currentDuration = ((currentDuration + 150) / 300) * 300;
        }
      }
      
      ESP_LOGI(getLogTag(), "Duration adjusted to %d seconds (delta: %d)", currentDuration, delta);
      
      // Update the state machine's pending duration immediately so UI can reflect it
      stateMachine.setPendingDuration(currentDuration);
      
      // Update LED gradient to show new duration
      LEDController* ledController = stateMachine.getLEDController();
      if (ledController) {
        // Get current project color and use it for the duration gradient
        uint32_t projectColor = ProjectManager::getInstance().getSelectedProjectColor();
        ledController->showDurationGradient(currentDuration / 60, 120, projectColor); // currentDuration is in seconds, convert to minutes
      }
    }
  
  // Update activity time when encoder moves
  if (delta != 0) {
    lastEncoderTime = millis();
  }
  
  // Check for timeout - return to IdleState after inactivity
  if (millis() - lastEncoderTime > ADJUST_TIMEOUT_MS) {
    ESP_LOGI(getLogTag(), "Adjust timeout - returning to IdleState");
    stateMachine.changeState(stateMachine.idleState);
  }
  
  // TODO: Handle tap to save and return to IdleState immediately
  
  yield();
}

void AdjustState::onExit()
{
  // Save the adjusted duration
  stateMachine.setPendingDuration(currentDuration);
  ESP_LOGI(getLogTag(), "Saved duration %d seconds", currentDuration);
}