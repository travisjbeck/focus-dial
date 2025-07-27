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

// Define the static instance of the new state, providing required arguments including global projectManager
ProjectSelectState StateMachine::projectSelectState(stateMachine, displayController, ledController, inputController, projectManager);

StateMachine::StateMachine()
    : currentState(&startupState) // Remove isTransitioning init
{
  stateMutex = xSemaphoreCreateMutex(); // Initialize the mutex
}

// Clean up the state and delete the mutex
StateMachine::~StateMachine()
{
  if (stateMutex != NULL)
  {
    vSemaphoreDelete(stateMutex); // Delete the mutex
  }
}

void StateMachine::begin()
{
  if (currentState)
  {
    currentState->enter();
  }
}

void StateMachine::update()
{
  // Restore original update logic (check transition flag?)
  if (!transition && currentState != nullptr)
  {
    currentState->update(); // Call update on the current state
  }
}

void StateMachine::changeState(State *newState)
{
  // Restore original changeState logic using mutex and transition flag
  if (xSemaphoreTake(stateMutex, portMAX_DELAY) == pdTRUE)
  {
    if (!newState || newState == currentState)
    { // Check if change needed
      xSemaphoreGive(stateMutex);
      return;
    }

    transition = true; // Set original transition flag

    if (currentState != nullptr)
    {
      currentState->exit();
    }

    Serial.printf("Changing state from %p to %p\n", (void *)currentState, (void *)newState);
    currentState = newState; // Assign the new state

    if (currentState != nullptr)
    {
      currentState->enter();
    }

    transition = false; // Clear original transition flag

    xSemaphoreGive(stateMutex); // Release the mutex
  }
  else
  {
    Serial.println("Error: Could not obtain state machine mutex in changeState");
  }
}

// --- Context Passing Methods ---

void StateMachine::setPendingDuration(int duration)
{
  pendingDuration = duration;
}

int StateMachine::getPendingDuration()
{
  return pendingDuration;
}

void StateMachine::setPendingElapsedTime(unsigned long seconds)
{
  pendingElapsedTime = seconds;
}

unsigned long StateMachine::getPendingElapsedTime()
{
  return pendingElapsedTime;
}

void StateMachine::setPendingProjectId(const String &projectId)
{
  pendingProjectId = projectId;
}

String StateMachine::getPendingProjectId() const
{
  return pendingProjectId;
}

void StateMachine::clearPendingProject()
{
  pendingProjectId = ""; // Reset pending project ID
}

// Check if the current state is IdleState
bool StateMachine::isInIdleState() const
{
  return currentState == &idleState;
}

// Reset LED color by notifying IdleState to restore its default LED pattern
void StateMachine::resetLEDColor()
{
  if (isInIdleState())
  {
    idleState.restoreDefaultLEDPattern();
  }
}

// Getter for the current state
State *StateMachine::getCurrentState() const
{
  return currentState;
}