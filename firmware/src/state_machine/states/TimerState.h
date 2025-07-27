#pragma once

#include "../include/State.h"

class TimerState : public State
{
public:
  TimerState();
  virtual ~TimerState();
  
  // State interface implementation
  const char* getStateName() const override { return "TimerState"; }
  bool validateStateEntry() const override;
  bool canTransitionTo(const State* nextState) const override;
  
  // Public accessors for UI
  unsigned long getRemainingSeconds() const;
  float getProgressPercentage() const;
  
  // Pause/Resume functionality
  void pauseTimer();
  void resumeTimer(unsigned long additionalPausedTime);

protected:
  // State lifecycle implementation
  void onEnter() override;
  void onUpdate() override;
  void onExit() override;
  
  // Logging
  const char* getLogTag() const override { return "TimerState"; }

private:
  unsigned long startTime;
  unsigned long pausedTime;
  int totalDuration; // In minutes
  bool isRunning;
  unsigned long lastDisplayUpdate;
  unsigned long lastProgressUpdate;
  String selectedProjectId;
  
  // Timer state tracking
  enum TimerPhase {
    STARTING,
    RUNNING,
    WARNING,    // Last 5 minutes
    CRITICAL    // Last 1 minute
  } currentPhase;
  
  // Private methods
  void initializeTimer();
  void updateCountdownDisplay();
  void updateLEDProgress();
  void checkTimerCompletion();
  void handleTimerEvents();
  unsigned long getElapsedSeconds() const;
};