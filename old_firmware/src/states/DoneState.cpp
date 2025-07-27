#include "StateMachine.h"
#include "Controllers.h"

DoneState::DoneState() : doneEnter(0) {}

void DoneState::enter()
{
  Serial.println("Entering Done State");

  doneEnter = millis();
  ledController.setBreath(FD_GREEN, 10, true, 50); // Changed from GREEN, Breath green 10 times, then solid

  // Register state-specific handlers
  // inputController.onPressHandler([this]() // Button deprecated - Phase 1
  //                                {
  //                                  Serial.println("Done State: Button Pressed - Returning to Idle");
  //                                  stateMachine.changeState(&StateMachine::idleState); // Transition back to Idle State
  //                                });

  // Send 'stop' action to webhook handler (which will fetch project details) - MOVED to TimerState exit/handlers
  // networkController.sendWebhookAction("stop");
}

void DoneState::update()
{
  inputController.update();
  ledController.update();

  // Get the final elapsed time stored in StateMachine
  unsigned long finalElapsedTime = stateMachine.getPendingElapsedTime();
  displayController.drawDoneScreen(finalElapsedTime);

  if (millis() - doneEnter >= (CHANGE_TIMEOUT * 1000))
  {
    // Transition to Idle after timeout
    stateMachine.changeState(&StateMachine::idleState);
  }
}

void DoneState::exit()
{
  Serial.println("Exiting Done State");
  inputController.releaseHandlers();
}
