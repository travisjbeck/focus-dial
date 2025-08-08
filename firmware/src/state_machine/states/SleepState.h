#pragma once

#include "../include/State.h"
#include <esp_sleep.h>

class SleepState : public State
{
public:
  SleepState();
  virtual ~SleepState();
  
  const char* getStateName() const override { return "SleepState"; }
  
  // Configuration
  void setDeepSleep(bool deep) { isDeepSleep = deep; }
  bool getDeepSleep() const { return isDeepSleep; }
  void setUseExternalWake(bool useExternal) { useExternalWake = useExternal; }
  bool getUseExternalWake() const { return useExternalWake; }
  
  // State restoration for deep sleep wake
  void restoreStateFromRTC();

protected:
  void onEnter() override;
  void onUpdate() override;
  void onExit() override;
  const char* getLogTag() const override { return "SleepState"; }

private:
  bool sleepInitiated;
  bool isDeepSleep;
  bool hasWokenUp;
  bool wifiWasConnected;
  String savedSSID;
  String savedPassword;
  bool useExternalWake = false; // when in deep sleep, use GPIO16 ext0 wake instead of BOOT button
  
  void saveStateToNVS();
  void saveStateToRTC();
  void configureWakeupSources();
  void enterSleepMode();
  void saveWiFiState();
  void restoreWiFiConnection();
};