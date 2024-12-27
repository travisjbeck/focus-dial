#include "StateMachine.h"
#include "Controllers.h"

TimerState::TimerState() : duration(0), elapsedTime(0), startTime(0) {}

void TimerState::enter()
{
    Serial.println("Entering Timer State");

    // Start time based on the elapsed time
    startTime = millis() - (elapsedTime * 1000);

    displayController.drawTimerScreen(duration * 60);
    ledController.startFillAndDecay(RED, ((duration * 60) - elapsedTime) * 1000);

    // Register state-specific handlers
    inputController.onPressHandler([this]()
                                   {
                                       Serial.println("Timer State: Button Pressed");

                                       // Send 'Stop' webhook (pause)
                                       networkController.sendWebhookAction("stop");
                                       displayController.showTimerPause();

                                       // Transition to PausedState and set elapsed time
                                       StateMachine::pausedState.setPause(this->duration, this->elapsedTime); // Save current elapsed time
                                       stateMachine.changeState(&StateMachine::pausedState);                  // Transition to Paused State
                                   });

    inputController.onDoublePressHandler([this]()
                                         {
                                             Serial.println("Timer State: Button Double Pressed");

                                             // Send 'Stop' webhook (canceled)
                                             networkController.sendWebhookAction("stop");
                                             displayController.showCancel();
                                             stateMachine.changeState(&StateMachine::idleState); // Transition to IdleState
                                         });

    networkController.startBluetooth();
    networkController.sendWebhookAction("start");
}

void TimerState::update()
{
    inputController.update();
    ledController.update();

    unsigned long currentTime = millis();
    elapsedTime = (currentTime - startTime) / 1000;

    int remainingSeconds = duration * 60 - elapsedTime;

    displayController.drawTimerScreen(remainingSeconds);

    // Check if the timer is done
    if (remainingSeconds <= 0)
    {
        Serial.println("Timer State: Done");
        displayController.showTimerDone();
        stateMachine.changeState(&StateMachine::doneState); // Transition to Done State
    }
}

void TimerState::exit()
{
    inputController.releaseHandlers();
    networkController.stopBluetooth();
    ledController.turnOff();
    Serial.println("Exiting Timer State");
}

void TimerState::setTimer(int duration, unsigned long elapsedTime)
{
    this->duration = duration;
    this->elapsedTime = elapsedTime;
}