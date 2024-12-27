#include <Arduino.h>
#include "Config.h"
#include "StateMachine.h"
#include "Controllers.h"

// Global instances of controllers
DisplayController displayController(OLED_WIDTH, OLED_HEIGHT, OLED_ADDR);
LEDController ledController(LED_PIN, NUM_LEDS, LED_BRIGHTNESS);
InputController inputController(BUTTON_PIN, ENCODER_A_PIN, ENCODER_B_PIN);
NetworkController networkController;
Preferences preferences;

void setup() {
    Serial.begin(115200);
    
    // Initialize controllers
    inputController.begin();
    displayController.begin();
    ledController.begin();
    networkController.begin();

    // Startup state
    stateMachine.changeState(&StateMachine::startupState);
}

void loop() {
    // Update state machine
    stateMachine.update();
    // If any animation needs to run
    displayController.updateAnimation();
}
