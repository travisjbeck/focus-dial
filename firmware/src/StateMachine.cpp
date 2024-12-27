#include "StateMachine.h"

// Global state machine instance
StateMachine stateMachine;

// Initialize static states
AdjustState StateMachine::adjustState;
SleepState StateMachine::sleepState;
DoneState StateMachine::doneState;
IdleState StateMachine::idleState;
PausedState StateMachine::pausedState;
ProvisionState StateMachine::provisionState;
ResetState StateMachine::resetState;
StartupState StateMachine::startupState;
TimerState StateMachine::timerState;

StateMachine::StateMachine() {
    currentState = &startupState;  // Start with StartupState
    stateMutex = xSemaphoreCreateMutex();  // Initialize the mutex
}

// Clean up the state and delete the mutex
StateMachine::~StateMachine() {
    if (stateMutex != NULL) {
        vSemaphoreDelete(stateMutex);  // Delete the mutex
    }
}

void StateMachine::changeState(State* newState) {
    // Lock the mutex
    if (xSemaphoreTake(stateMutex, portMAX_DELAY) == pdTRUE) {
        transition = true;
        if (currentState != nullptr) {
            currentState->exit();
        }
        currentState = newState;  // Assign the new state (static state)
        currentState->enter();
        transition = false;
        xSemaphoreGive(stateMutex);  // Release the mutex
    }
}

void StateMachine::update() {
    if (!transition && currentState != nullptr) {
        currentState->update();  // Call update on the current state
    }
}