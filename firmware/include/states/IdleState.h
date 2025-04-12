#pragma once

#include "State.h"

class IdleState : public State
{
public:
  IdleState();
  void enter() override;
  void update() override;
  void exit() override;
  void setTimer(int duration);
  int getDefaultDuration() const;

private:
  int defaultDuration;
  unsigned long lastActivity;
};
