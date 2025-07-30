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

protected:
  void onEnter() override;
  void onUpdate() override;
  void onExit() override;
  const char* getLogTag() const override { return "SleepState"; }

private:
  bool sleepInitiated;
  bool isDeepSleep;
  bool hasWokenUp;
  
  void saveStateToNVS();
  void configureWakeupSources();
  void enterSleepMode();
};