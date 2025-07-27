#pragma once

#include "../include/State.h"

class ProjectSelectState : public State
{
public:
  ProjectSelectState();
  virtual ~ProjectSelectState();
  
  const char* getStateName() const override { return "ProjectSelectState"; }
  
  // Public method to reset activity timer (called from TouchManager)
  void resetActivityTimer();

protected:
  void onEnter() override;
  void onUpdate() override;
  void onExit() override;
  const char* getLogTag() const override { return "ProjectSelectState"; }

private:
  int selectedIndex;
  int projectCount;
  unsigned long lastActivityTime;
  static const unsigned long TIMEOUT_DURATION_MS = 30000; // 30 seconds
};