#include "StateMachine.h"
#include "Controllers.h"

PausedState::PausedState() : duration(0), elapsedTime(0), pauseEnter(0) {}

void PausedState::enter()
{
    Serial.println("Entering Paused State");
    pauseEnter = millis(); // Record the time when the pause started
    ledController.setBreath(YELLOW, -1, false, 20);

    // Register state-specific handlers
    inputController.onPressHandler([this]()
                                   {
                                       Serial.println("Paused State: Button Pressed");

                                       // Send 'Start' webhook (resume)
                                       networkController.sendWebhookAction("start");

                                       // Transition back to TimerState with the stored duration and elapsed time
                                       StateMachine::timerState.setTimer(duration, elapsedTime);
                                       displayController.showTimerResume();
                                       stateMachine.changeState(&StateMachine::timerState); // Transition back to Timer State
                                   });

    inputController.onDoublePressHandler([]()
                                         {
                                             Serial.println("Paused State: Button Double Pressed");

                                             // Send 'Stop' webhook (canceled)
                                             networkController.sendWebhookAction("stop");
                                             displayController.showCancel();
                                             stateMachine.changeState(&StateMachine::idleState); // Transition back to Idle State
                                         });
}

void PausedState::update()
{
    inputController.update();
    ledController.update();

    // Redraw the paused screen with remaining time
    int remainingTime = (duration * 60) - elapsedTime;
    displayController.drawPausedScreen(remainingTime);

    unsigned long currentTime = millis();

    // Check if the pause timeout has been reached
    if (currentTime - pauseEnter >= (PAUSE_TIMEOUT * 60 * 1000))
    {
        // Timeout reached, transition to Idle State
        Serial.println("Paused State: Timout");

        // Send 'Stop' webhook (timeout)
        networkController.sendWebhookAction("stop");
        displayController.showCancel();
        stateMachine.changeState(&StateMachine::idleState); // Transition back to Idle State
    }
}

void PausedState::exit()
{
    Serial.println("Exiting Paused State");
    inputController.releaseHandlers();
}

void PausedState::setPause(int duration, unsigned long elapsedTime)
{
    this->duration = duration;
    this->elapsedTime = elapsedTime;
}