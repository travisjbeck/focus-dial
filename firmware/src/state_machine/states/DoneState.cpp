#include "DoneState.h"
#include "../include/StateMachine.h"
#include "IdleState.h"
#include "../../ui/ScreenManager.h"
#include "../../audio/AlarmController.h"
#include <Preferences.h>
#include "HWCDC.h"

extern HWCDC USBSerial;

DoneState::DoneState() : elapsedTime(0), stateStartTime(0), alarmStartTime(0), alarmTriggered(false)
{
}

DoneState::~DoneState()
{
}

void DoneState::onEnter()
{
  elapsedTime = stateMachine.getPendingElapsedTime();
  stateStartTime = millis();
  alarmTriggered = false;
  
  ESP_LOGI(getLogTag(), "Timer completed in %lu seconds", elapsedTime);
  ESP_LOGI(getLogTag(), "Done screen will auto-transition to idle after %lu seconds", AUTO_TRANSITION_DELAY / 1000);
  
  // Check if alarm is enabled in preferences
  Preferences prefs;
  bool alarmEnabled = true; // Default to enabled
  if (prefs.begin("alarm", true)) {
    alarmEnabled = prefs.getBool("enabled", true);
    prefs.end();
  }
  
  // Trigger alarm if enabled
  if (alarmEnabled) {
    ESP_LOGI(getLogTag(), "Playing timer completion alarm");
    USBSerial.println("DoneState: Timer completed, triggering alarm!");
    extern AlarmController alarmController;
    
    // Try to get selected sound from preferences
    String selectedSound = "";
    if (prefs.begin("alarm", true)) {
      selectedSound = prefs.getString("sound", "");
      prefs.end();
    }
    
    USBSerial.println("DoneState: Calling playAlarm()");
    if (selectedSound.length() > 0) {
      alarmController.playAlarm(selectedSound.c_str());
    } else {
      alarmController.playAlarm(); // Use default sound
    }
    
    alarmTriggered = true;
    alarmStartTime = millis();
  } else {
    USBSerial.println("DoneState: Alarm is disabled in preferences");
  }
}

void DoneState::onUpdate()
{
  unsigned long currentTime = millis();
  
  // Check if alarm has been playing too long
  if (alarmTriggered && (currentTime - alarmStartTime >= ALARM_MAX_DURATION)) {
    extern AlarmController alarmController;
    if (alarmController.isPlaying()) {
      ESP_LOGI(getLogTag(), "Stopping alarm after timeout");
      alarmController.stopAlarm();
    }
  }
  
  // Check for auto-transition timeout
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
  
  // Stop alarm if still playing
  if (alarmTriggered) {
    extern AlarmController alarmController;
    if (alarmController.isPlaying()) {
      alarmController.stopAlarm();
    }
  }
}