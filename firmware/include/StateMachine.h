#pragma once

#include <Arduino.h>
#include "State.h"
#include "states/AdjustState.h"
#include "states/DoneState.h"
#include "states/IdleState.h"
#include "states/PausedState.h"
#include "states/ProvisionState.h"
#include "states/ResetState.h"
#include "states/SleepState.h"
#include "states/StartupState.h"
#include "states/TimerState.h"
#include "states/ProjectSelectState.h"

class StateMachine
{
public:
  StateMachine();
  ~StateMachine();

  // Public methods for initialization and control
  void begin(); // Ensure begin is declared
  void update();
  void changeState(State *newState);

  // Static states
  static AdjustState adjustState;
  static SleepState sleepState;
  static DoneState doneState;
  static IdleState idleState;
  static PausedState pausedState;
  static ProvisionState provisionState;
  static ResetState resetState;
  static StartupState startupState;
  static TimerState timerState;
  static ProjectSelectState projectSelectState;

  // Methods to pass context between states
  void setPendingDuration(int duration);
  int getPendingDuration();

  // Methods to pass selected project info
  void setPendingProject(const String &name, const String &color);
  String getPendingProjectName() const;
  String getPendingProjectColor() const;
  void clearPendingProject();

private:
  State *currentState;          // Pointer to the current state
  SemaphoreHandle_t stateMutex; // Mutex to protect transitions
  bool transition = false;
  int pendingDuration;        // To pass duration from AdjustState to TimerState
  String pendingProjectName;  // To pass project name to TimerState/Webhook
  String pendingProjectColor; // To pass project color to TimerState
};

extern StateMachine stateMachine; // Global instance of the StateMachine