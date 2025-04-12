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

void StateMachine::setPendingProject(const String &name, const String &color)
{
  pendingProjectName = name;
  pendingProjectColor = color;
}

String StateMachine::getPendingProjectName() const
{
  return pendingProjectName;
}

String StateMachine::getPendingProjectColor() const
{
  return pendingProjectColor;
}

void StateMachine::clearPendingProject()
{
  pendingProjectName = "";
  pendingProjectColor = ""; // Reset pending details
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