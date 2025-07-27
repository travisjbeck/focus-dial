#pragma once

#include "../include/State.h"

class DoneState : public State
{
public:
  DoneState();
  virtual ~DoneState();
  
  const char* getStateName() const override { return "DoneState"; }

protected:
  void onEnter() override;
  void onUpdate() override;
  void onExit() override;
  const char* getLogTag() const override { return "DoneState"; }

private:
  unsigned long elapsedTime;
  unsigned long stateStartTime;
  static const unsigned long AUTO_TRANSITION_DELAY = 30000; // 30 seconds
};