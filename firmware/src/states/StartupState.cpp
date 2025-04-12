#include "StateMachine.h"
#include "Controllers.h"

StartupState::StartupState() : startEnter(0) {}

void StartupState::enter()
{
  Serial.println("Entering Splash State");

  displayController.drawSplashScreen();
  ledController.setSpinner(TEAL, -1);

  startEnter = millis();
}

void StartupState::update()
{
  ledController.update();

  if (millis() - startEnter >= (SPLASH_DURATION * 1000))
  {
    if (networkController.isWiFiProvisioned())
    {
      stateMachine.changeState(&StateMachine::idleState); // Transition to Idle
    }
    else
    {
      stateMachine.changeState(&StateMachine::provisionState); // Trigger Provision
    }
  }
}

void StartupState::exit()
{
  ledController.turnOff();
  Serial.println("Exiting Splash State");
}
