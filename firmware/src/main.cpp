#include <Arduino.h>
#include "Config.h"
#include "StateMachine.h"
#include "Controllers.h"
#include "managers/ProjectManager.h"

// Global instances of controllers
DisplayController displayController(OLED_WIDTH, OLED_HEIGHT, OLED_ADDR);
LEDController ledController(LED_PIN, NUM_LEDS, LED_BRIGHTNESS);
InputController inputController(BUTTON_PIN, ENCODER_A_PIN, ENCODER_B_PIN);
NetworkController networkController;
Preferences preferences;
ProjectManager projectManager;

// --- Add static function to get the global instance ---
ProjectManager &getProjectManagerInstance()
{
  return projectManager;
}

void setup()
{
  Serial.begin(115200);

  // Initialize Project Manager first (loads data needed by others)
  if (!projectManager.begin())
  {
    Serial.println("FATAL: Failed to initialize Project Manager!");
    // Optional: Enter a safe error state? Loop forever?
    while (1)
    {
      delay(1000);
    }
  }

  // Initialize controllers
  inputController.begin();
  displayController.begin();
  ledController.begin();
  networkController.begin();

  // Startup state
  stateMachine.changeState(&StateMachine::startupState);
}

void loop()
{
  // Update state machine
  stateMachine.update();
  // If any animation needs to run
  displayController.updateAnimation();
}
