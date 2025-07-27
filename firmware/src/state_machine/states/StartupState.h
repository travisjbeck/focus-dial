#pragma once

#include "../include/State.h"

class StartupState : public State
{
public:
  StartupState();
  virtual ~StartupState();
  
  const char* getStateName() const override { return "StartupState"; }

protected:
  void onEnter() override;
  void onUpdate() override;
  void onExit() override;
  const char* getLogTag() const override { return "StartupState"; }

private:
  static const unsigned long SPLASH_DURATION = 2000; // 2 seconds
};