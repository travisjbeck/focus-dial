#include "StateMachine.h"
#include "Controllers.h"

StartupState::StartupState() : startEnter(0) {}

void StartupState::enter()
{
  Serial.println("StartupState::enter() - TOP");

  Serial.println("StartupState::enter() - Calling drawSplashScreen()...");
  displayController.drawSplashScreen();
  Serial.println("StartupState::enter() - Returned from drawSplashScreen().");

  Serial.println("StartupState::enter() - Calling ledController.setSpinner(FD_TEAL)...");
  ledController.setSpinner(FD_TEAL, -1);
  Serial.println("StartupState::enter() - Returned from ledController.setSpinner().");

  startEnter = millis();
  Serial.print("StartupState::enter() - startEnter set to: ");
  Serial.println(startEnter);
  Serial.println("StartupState::enter() - BOTTOM (Finished)");
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
