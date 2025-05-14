#include <Arduino.h>
#include "Config.h"
#include "StateMachine.h"
#include "Controllers.h"
#include "managers/ProjectManager.h"
#include "nvs_flash.h" // For NVS - Keep for robust init
#include "nvs.h"       // For NVS - Keep for robust init

// Global instances of controllers
DisplayController displayController(OLED_WIDTH, OLED_HEIGHT, OLED_ADDR);
LEDController ledController(LED_PIN, NUM_LEDS, LED_BRIGHTNESS);
InputController inputController(BUTTON_PIN, ENCODER_A_PIN, ENCODER_B_PIN);
NetworkController networkController;
Preferences preferences; // This is the main Preferences object the old firmware uses
ProjectManager projectManager;

// REMOVED NVS Handle and constants for testing (main_nvs_handle, etc.)

// --- Add static function to get the global instance ---
ProjectManager &getProjectManagerInstance()
{
  return projectManager;
}

void setup()
{
  Serial.begin(115200);
  delay(2000); // Delay for serial monitor connection

  Serial.println("--- Main Firmware Booting --- ");

  // --- Robust NVS Flash Initialization (MUST BE DONE FIRST) ---
  esp_err_t nvs_err = nvs_flash_init();
  if (nvs_err == ESP_ERR_NVS_NO_FREE_PAGES || nvs_err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      Serial.println("NVS: Erasing and re-initializing flash due to no free pages or new version...");
      ESP_ERROR_CHECK(nvs_flash_erase()); // Erase the NVS partition
      nvs_err = nvs_flash_init();        // Retry initialization
  }
  ESP_ERROR_CHECK(nvs_err); // Check the result of the (possibly retried) initialization
  Serial.println("NVS: Flash initialized successfully by main.cpp.");
  // --- End Robust NVS Flash Initialization ---

  // REMOVED the NVS run_counter test block from here.
  // The global 'preferences' object will now be used by other modules (e.g., ProjectManager)
  // which rely on Preferences.begin() internally, and they will benefit from the robust
  // nvs_flash_init() performed above.

  // Initialize Project Manager first (loads data needed by others)
  if (!projectManager.begin()) // projectManager.begin() calls _preferences.begin() internally
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
  displayController.begin(); // Assumes old display, will likely have issues
  ledController.begin();     // Assumes old LED setup
  networkController.begin(); // Uses global preferences object

  // Startup state
  stateMachine.changeState(&StateMachine::startupState);
}

void loop()
{
  // Update state machine
  stateMachine.update();
  // If any animation needs to run
  // displayController.updateAnimation(); // Might cause issues if displayController init failed
}
