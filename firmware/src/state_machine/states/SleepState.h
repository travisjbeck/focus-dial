#pragma once

#include "../include/State.h"

class SleepState : public State
{
public:
  SleepState();
  virtual ~SleepState();
  
  const char* getStateName() const override { return "SleepState"; }

protected:
  void onEnter() override;
  void onUpdate() override;
  void onExit() override;
  const char* getLogTag() const override { return "SleepState"; }

private:
  bool sleepInitiated;
};