#include "StateMachine.h"
#include "Controllers.h"

bool resetSelected = false; // button selection

void ResetState::enter()
{
    Serial.println("Entering Reset State");

    ledController.setBreath(MAGENTA, -1, false, 10);

    // Register state-specific handlers
    inputController.onEncoderRotateHandler([this](int delta)
                                           {
        if (delta > 0) {
            resetSelected = true;  // Select "RESET"
        } else if (delta < 0) {
            resetSelected = false;  // Select "CANCEL"
        } });

    inputController.onPressHandler([this]()
                                   {
        if (resetSelected) {
            Serial.println("Reset State: RESET button pressed, rebooting.");
            displayController.showReset();
            networkController.reset();
            resetStartTime = millis();
        } else {
            Serial.println("Reset State: CANCEL button pressed, returning to Idle.");
            displayController.showCancel();
            stateMachine.changeState(&StateMachine::idleState);
        } });
}

void ResetState::update()
{

    inputController.update();
    ledController.update();
    displayController.drawResetScreen(resetSelected);

    if (resetStartTime > 0 && (millis() - resetStartTime >= 1000))
    {
        Serial.println("Restarting ...");
        ESP.restart(); // Restart after 1 second
    }
}

void ResetState::exit()
{
    Serial.println("Exiting Reset State");
    inputController.releaseHandlers();
    ledController.turnOff();
}
