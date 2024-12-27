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

class StateMachine {
public:
    StateMachine();
    ~StateMachine();

    void changeState(State* newState);
    void update();

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

private:
    State* currentState;            // Pointer to the current state
    SemaphoreHandle_t stateMutex;   // Mutex to protect transitions
    bool transition = false;
};

extern StateMachine stateMachine;  // Global instance of the StateMachine