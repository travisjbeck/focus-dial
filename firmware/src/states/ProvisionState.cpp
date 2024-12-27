#include "StateMachine.h"
#include "Controllers.h"

void ProvisionState::enter()
{
    Serial.println("Entering Provision State");
    inputController.releaseHandlers();
    displayController.drawProvisionScreen();
    ledController.setSolid(AMBER);
    networkController.startProvisioning();
}

void ProvisionState::update()
{
    ledController.update();
    if (networkController.isWiFiProvisioned() && networkController.isWiFiConnected())
    {
        Serial.println("Provisioning Complete, WiFi Connected");
        displayController.showConnected();
        networkController.stopProvisioning();
        stateMachine.changeState(&StateMachine::idleState);
    }
}

void ProvisionState::exit()
{
    Serial.println("Exiting Provision State");
    networkController.stopProvisioning();
}
