#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "State.h"
#include "states/AdjustState.h"
#include "states/DoneState.h"
#include "states/IdleState.h"
#include "states/PausedState.h"
#include "states/ProjectSelectState.h"
#include "states/ProvisionState.h"
#include "states/ResetState.h"
#include "states/SleepState.h"
#include "states/StartupState.h"
#include "states/TimerState.h"

class StateMachine
{
public:
  StateMachine();
  ~StateMachine();

  // Public methods for initialization and control
  void begin(); // Ensure begin is declared
  void update();
  void changeState(State *newState);
  State *getCurrentState() const;

  // Static states
  static AdjustState adjustState;
  static SleepState sleepState;
  static DoneState doneState;
  static IdleState idleState;
  static PausedState pausedState;
  static ProjectSelectState projectSelectState;
  static ProvisionState provisionState;
  static ResetState resetState;
  static StartupState startupState;
  static TimerState timerState;

  // Methods to pass context between states
  void setPendingDuration(int duration);
  int getPendingDuration();
  void setPendingElapsedTime(unsigned long seconds);
  unsigned long getPendingElapsedTime();

  // Methods to pass selected project info
  void setPendingProjectId(const String &projectId);
  String getPendingProjectId() const;
  void clearPendingProject();

  // New methods for LED color preview
  bool isInIdleState() const;
  void resetLEDColor();

private:
  State *currentState;          // Pointer to the current state
  SemaphoreHandle_t stateMutex; // Mutex to protect transitions
  bool transition = false;
  int pendingDuration;              // To pass duration from AdjustState to TimerState
  unsigned long pendingElapsedTime; // To pass final time from TimerState to DoneState
  String pendingProjectId;          // Store selected project ID for TimerState/Webhook
};

extern StateMachine stateMachine; // Global instance of the StateMachine