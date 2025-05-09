#include "StateMachine.h"
#include "Controllers.h"

DoneState::DoneState() : doneEnter(0) {}

void DoneState::enter()
{
  Serial.println("Entering Done State");

  doneEnter = millis();
  ledController.setBreath(GREEN, -1, true, 2);

  // Register state-specific handlers
  inputController.onPressHandler([]()
                                 {
        Serial.println("Done State: Button pressed");
        stateMachine.changeState(&StateMachine::idleState); });

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
