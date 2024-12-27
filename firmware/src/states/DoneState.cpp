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

    // Send 'Stop' webhook
    networkController.sendWebhookAction("stop");
}

void DoneState::update()
{
    inputController.update();
    ledController.update();

    displayController.drawDoneScreen();

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
