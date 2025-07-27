#pragma once

#include "../include/State.h"

class IdleState : public State
{
public:
  IdleState();
  virtual ~IdleState();
  
  // State interface implementation
  const char* getStateName() const override { return "IdleState"; }
  bool validateStateEntry() const override;
  bool canTransitionTo(const State* nextState) const override;

protected:
  // State lifecycle implementation
  void onEnter() override;
  void onUpdate() override;
  void onExit() override;
  
  // Logging
  const char* getLogTag() const override { return "IdleState"; }

private:
  unsigned long lastTouchTime;
  bool displayInitialized;
  bool timerDisplayShown;
  int currentDisplayDuration; // Current timer duration being displayed
  
  // Private methods
  void initializeDisplay();
  void updateTimerDisplay();
  void handleTouchInput();
  void handleEncoderInput();
};