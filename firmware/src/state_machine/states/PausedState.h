#pragma once

#include "../include/State.h"

class PausedState : public State
{
public:
  PausedState();
  virtual ~PausedState();
  
  const char* getStateName() const override { return "PausedState"; }
  bool canTransitionTo(const State* nextState) const override;

protected:
  void onEnter() override;
  void onUpdate() override;
  void onExit() override;
  const char* getLogTag() const override { return "PausedState"; }

private:
  unsigned long pauseStartTime;
};