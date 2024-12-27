#include "StateMachine.h"
#include "Controllers.h"

void SleepState::enter()
{
    Serial.println("Entering Sleep State");

    ledController.turnOff();
    displayController.clear();

    // Register state-specific handlers
    inputController.onPressHandler([]()
                                   {
        Serial.println("Sleep State: Button pressed");
        stateMachine.changeState(&StateMachine::idleState); });

    inputController.onLongPressHandler([]()
                                       {
        Serial.println("Sleep State: long pressed");
        stateMachine.changeState(&StateMachine::idleState); });

    inputController.onEncoderRotateHandler([this](int delta)
                                           {
        Serial.println("Sleep State: Encoder turned");
        stateMachine.changeState(&StateMachine::idleState); });
}

void SleepState::update()
{
    inputController.update();
}

void SleepState::exit()
{
    Serial.println("Exiting Sleep State");
    inputController.releaseHandlers();
}
