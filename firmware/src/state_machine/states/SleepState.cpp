#include "SleepState.h"
#include "../include/StateMachine.h"
#include "../include/LEDController.h"
#include "../../ui/ScreenManager.h"
#include <nvs_flash.h>
#include <nvs.h>
#include <XPowersLib.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <Arduino.h>
#include "driver/rtc_io.h"
#include <cmath>

extern XPowersAXP2101 power;
extern LEDController* g_ledController;
extern ScreenManager screenManager;

#define WAKE_BUTTON_PIN GPIO_NUM_0  // BOOT button as wake source

SleepState::SleepState() : sleepInitiated(false), isDeepSleep(false), hasWokenUp(false), wifiWasConnected(false)
{
}

SleepState::~SleepState()
{
}

void SleepState::onEnter()
{
  sleepInitiated = false;
  hasWokenUp = false;
  
  ESP_LOGI(getLogTag(), "=== SLEEP STATE ENTERED ===");
  ESP_LOGI(getLogTag(), "Preparing for %s sleep", isDeepSleep ? "deep" : "light");
  
  // Check GPIO states immediately on entry
  int gpio17_state = digitalRead(17);
  int gpio18_state = digitalRead(18);
  ESP_LOGI(getLogTag(), "GPIO states on entry: GPIO17=%d, GPIO18=%d", gpio17_state, gpio18_state);
  
  // Save current state before sleep
  saveStateToNVS();
  
  // Save WiFi state before disconnecting
  saveWiFiState();
  
  // Turn off display and LEDs
  ESP_LOGI(getLogTag(), "Turning off display and LEDs");
  ESP_LOGI(getLogTag(), "Calling turnOffDisplay on screenManager");
  screenManager.turnOffDisplay();
  if (g_ledController) {
    ESP_LOGI(getLogTag(), "Turning off LEDs");
    g_ledController->turnOff();
  }
  
  // Disable WiFi and Bluetooth to save power
  if (!isDeepSleep) {
    // For light sleep, just stop WiFi to save power
    WiFi.mode(WIFI_OFF);
  } else {
    // For deep sleep, fully disable WiFi and Bluetooth
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    esp_wifi_stop();
    esp_wifi_deinit();
    
    // Disable Bluetooth if it was enabled
    // Check if Bluetooth is enabled before trying to disable
    if (esp_bluedroid_get_status() == ESP_BLUEDROID_STATUS_ENABLED) {
      esp_bluedroid_disable();
    }
    if (esp_bluedroid_get_status() != ESP_BLUEDROID_STATUS_UNINITIALIZED) {
      esp_bluedroid_deinit();
    }
    if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED) {
      esp_bt_controller_disable();
    }
    if (esp_bt_controller_get_status() != ESP_BT_CONTROLLER_STATUS_IDLE) {
      esp_bt_controller_deinit();
    }
  }
  
  // Disable all GPIO interrupts before configuring wake sources
  if (!isDeepSleep) {
    // Disable encoder interrupts to prevent interference with sleep wake
    gpio_intr_disable(GPIO_NUM_17);
    gpio_intr_disable(GPIO_NUM_18);
    // Disable touch interrupt
    gpio_intr_disable(GPIO_NUM_11);
    // Disable button interrupt  
    gpio_intr_disable(GPIO_NUM_21);
  }
  
  // Configure wake sources
  configureWakeupSources();
  
  // Clear any pending interrupts
  if (power.isBatteryConnect()) {
    power.clearIrqStatus();
  }
}

void SleepState::onUpdate()
{
  ESP_LOGI(getLogTag(), "onUpdate called: sleepInitiated=%d, hasWokenUp=%d", sleepInitiated, hasWokenUp);
  
  if (!sleepInitiated) {
    sleepInitiated = true;
    ESP_LOGI(getLogTag(), "About to enter sleep mode (deep=%d)", isDeepSleep);
    
    // Check GPIO states before sleep
    if (!isDeepSleep) {
      int gpio17_state = digitalRead(17);
      int gpio18_state = digitalRead(18);
      ESP_LOGI(getLogTag(), "GPIO states before sleep: GPIO17=%d, GPIO18=%d", gpio17_state, gpio18_state);
    }
    
    // Small delay to ensure everything is ready
    yieldMs(100);
    
    // Enter sleep mode - this will block until wake for light sleep
    enterSleepMode();
    
    // If we reach here, we've woken from light sleep (deep sleep restarts device)
    if (!isDeepSleep) {
      ESP_LOGI(getLogTag(), "Light sleep completed - transitioning to IdleState");
      stateMachine.transitionTo("IdleState");
    }
  }
  // For light sleep, we should not reach this else block in normal operation
  
  yieldMs(100);
}

void SleepState::onExit()
{
  ESP_LOGI(getLogTag(), "Waking up from sleep");
  
  // The display and peripherals are already reinitialized in setup()
  // after deep sleep wake, so we only need to handle light sleep wake
  
  // Turn display back on for light sleep wake
  if (!isDeepSleep) {
    ESP_LOGI(getLogTag(), "Light sleep wake - checking screenManager");
    ESP_LOGI(getLogTag(), "Calling turnOnDisplay()");
    screenManager.turnOnDisplay();
    ESP_LOGI(getLogTag(), "turnOnDisplay() called");
    
    // TEMPORARILY DISABLED - WiFi reconnection after wake was causing reboots
    // Restore WiFi connection if it was connected before sleep
    // if (wifiWasConnected) {
    //   ESP_LOGI(getLogTag(), "Restoring WiFi connection after light sleep");
    //   restoreWiFiConnection();
    // }
    ESP_LOGI(getLogTag(), "WiFi reconnection disabled - manual restart required if web access needed");
  } else {
    ESP_LOGI(getLogTag(), "Deep sleep wake - display will be reinitialized in setup()");
  }
  
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  
  if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
    ESP_LOGI(getLogTag(), "Woken by BOOT button (ext0)");
  } else if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT1) {
    // ext1 wake - get which GPIO caused the wake using proven method
    uint64_t GPIO_reason = esp_sleep_get_ext1_wakeup_status();
    int wakeup_pin = (int)(log(GPIO_reason) / log(2));
    
    if (wakeup_pin == 17) {
      ESP_LOGI(getLogTag(), "Woken by encoder A rotation (GPIO17)");
    } else if (wakeup_pin == 18) {
      ESP_LOGI(getLogTag(), "Woken by encoder B rotation (GPIO18)");
    } else {
      ESP_LOGI(getLogTag(), "Woken by ext1 on GPIO: %d", wakeup_pin);
    }
    
    // Update activity time since user interacted with device
    stateMachine.updateActivityTime();
  } else if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) {
    ESP_LOGI(getLogTag(), "Woken by timer");
  } else {
    ESP_LOGI(getLogTag(), "Woken by other source: %d", wakeup_reason);
  }
  
  // Re-enable GPIO interrupts after wake from light sleep
  if (!isDeepSleep) {
    // Reconfigure RTC GPIO pins for digital use after wake
    rtc_gpio_deinit(GPIO_NUM_17);
    rtc_gpio_deinit(GPIO_NUM_18);
    
    // Re-enable encoder interrupts
    gpio_intr_enable(GPIO_NUM_17);
    gpio_intr_enable(GPIO_NUM_18);
    // Re-enable touch interrupt
    gpio_intr_enable(GPIO_NUM_11);
    // Re-enable button interrupt
    gpio_intr_enable(GPIO_NUM_21);
  }
  
  // Clear power management interrupts
  if (power.isBatteryConnect()) {
    power.clearIrqStatus();
  }
}

void SleepState::saveStateToNVS()
{
  nvs_handle_t nvs_handle;
  esp_err_t err = nvs_open("sleep_state", NVS_READWRITE, &nvs_handle);
  
  if (err == ESP_OK) {
    // Save current state machine state
    const char* currentState = stateMachine.getCurrentState()->getStateName();
    nvs_set_str(nvs_handle, "last_state", currentState);
    
    // Save timer information if in timer state
    if (strcmp(currentState, "TimerState") == 0) {
      // TODO: Save current timer time and project
      // This would require access to timer state data
    }
    
    nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    
    ESP_LOGI(getLogTag(), "State saved to NVS: %s", currentState);
  } else {
    ESP_LOGW(getLogTag(), "Failed to save state to NVS");
  }
}

void SleepState::configureWakeupSources()
{
  if (isDeepSleep) {
    // Power button sleep: Only BOOT button can wake
    esp_sleep_enable_ext0_wakeup(WAKE_BUTTON_PIN, 0); // Wake on LOW
    ESP_LOGI(getLogTag(), "Power button deep sleep configured: Only BOOT button can wake");
  } else {
    // Use ESP32-S3 compatible wake configuration  
    // Configure encoder wake with ext1 for GPIO17/18
    rtc_gpio_pullup_en(GPIO_NUM_17);
    rtc_gpio_pullup_en(GPIO_NUM_18);
    
    // Check current GPIO states to determine wake condition
    int gpio17_state = digitalRead(17);
    int gpio18_state = digitalRead(18);
    ESP_LOGI(getLogTag(), "GPIO states: GPIO17=%d, GPIO18=%d", gpio17_state, gpio18_state);
    
    // If both pins are HIGH, wake on LOW. If either is LOW, wake on HIGH
    esp_sleep_ext1_wakeup_mode_t wake_mode = ESP_EXT1_WAKEUP_ANY_HIGH;
    if (gpio17_state == 1 && gpio18_state == 1) {
      wake_mode = ESP_EXT1_WAKEUP_ANY_LOW;
      ESP_LOGI(getLogTag(), "Both pins HIGH - wake on ANY_LOW");
    } else {
      wake_mode = ESP_EXT1_WAKEUP_ANY_HIGH; 
      ESP_LOGI(getLogTag(), "Pin(s) LOW - wake on ANY_HIGH");
    }
    
    // Use standard ext1 wake function for ESP32-S3
    uint64_t ext1_mask = (1ULL << GPIO_NUM_17) | (1ULL << GPIO_NUM_18);
    esp_sleep_enable_ext1_wakeup(ext1_mask, wake_mode);
    
    ESP_LOGI(getLogTag(), "Light sleep configured: encoder wake on GPIO17/18 (ext1)");
    
    // Keep RTC peripherals powered
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    
    ESP_LOGI(getLogTag(), "Encoder wake configured: RTC GPIO17/18 with ext1 wake");
  }
}

void SleepState::enterSleepMode()
{
  if (isDeepSleep) {
    ESP_LOGI(getLogTag(), "Entering DEEP sleep mode (power button)...");
    ESP_LOGI(getLogTag(), "Press BOOT button to wake!");
    
    // Give time for serial output
    delay(100);
    
    // Enter deep sleep - ESP32 will restart on wake
    esp_deep_sleep_start();
  } else {
    // For inactivity timeout, use LIGHT sleep (proven working in Session 5)
    // Light sleep works where deep sleep fails on this ESP32-S3 hardware
    ESP_LOGI(getLogTag(), "Entering LIGHT sleep mode (inactivity timeout)...");
    ESP_LOGI(getLogTag(), "Rotate encoder to wake!");
    
    // Check GPIO states right before sleep
    int gpio17_state = digitalRead(17);
    int gpio18_state = digitalRead(18);
    ESP_LOGI(getLogTag(), "Final GPIO check: GPIO17=%d, GPIO18=%d", gpio17_state, gpio18_state);
    
    // Give time for serial output
    delay(100);
    
    ESP_LOGI(getLogTag(), "Calling esp_light_sleep_start() NOW...");
    // Enter light sleep - device maintains RAM and wakes to same state
    esp_light_sleep_start();
    
    // After waking from light sleep, we continue execution here
    ESP_LOGI(getLogTag(), "=== WOKE FROM LIGHT SLEEP ===");
    
    // Check what caused the wake
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    ESP_LOGI(getLogTag(), "Wake cause: %d (%s)", wakeup_reason, 
      (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) ? "EXT0" :
      (wakeup_reason == ESP_SLEEP_WAKEUP_EXT1) ? "EXT1" :
      (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) ? "TIMER" :
      (wakeup_reason == ESP_SLEEP_WAKEUP_TOUCHPAD) ? "TOUCH" :
      (wakeup_reason == ESP_SLEEP_WAKEUP_ULP) ? "ULP" : 
      (wakeup_reason == ESP_SLEEP_WAKEUP_GPIO) ? "GPIO" :
      (wakeup_reason == ESP_SLEEP_WAKEUP_UART) ? "UART" : "UNKNOWN");
    
    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT1) {
      uint64_t GPIO_reason = esp_sleep_get_ext1_wakeup_status();
      int wakeup_pin = (int)(log(GPIO_reason) / log(2));
      ESP_LOGI(getLogTag(), "Woken by encoder on GPIO: %d", wakeup_pin);
    }
  }
}

void SleepState::saveWiFiState()
{
  wifiWasConnected = (WiFi.status() == WL_CONNECTED);
  if (wifiWasConnected) {
    savedSSID = WiFi.SSID();
    savedPassword = WiFi.psk();
    ESP_LOGI(getLogTag(), "Saved WiFi state - Connected to: %s", savedSSID.c_str());
  } else {
    ESP_LOGI(getLogTag(), "WiFi not connected - no state to save");
  }
}

void SleepState::restoreWiFiConnection()
{
  if (savedSSID.length() > 0) {
    ESP_LOGI(getLogTag(), "Reconnecting to WiFi: %s", savedSSID.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.begin(savedSSID.c_str(), savedPassword.c_str());
    // Note: Web server will be restarted automatically via WiFi event handler
  }
}

// RTC memory structure for state preservation during deep sleep
typedef struct {
  uint32_t magic;           // Magic number to validate structure
  uint32_t crc32;          // CRC32 checksum for data integrity
  uint32_t lastState;      // Last state machine state ID
  uint32_t currentProject; // Current project index
  uint32_t timerDuration;  // Timer duration in seconds (if in timer state)
  uint32_t timerRemaining; // Remaining time in seconds (if paused)
  bool timerWasActive;     // Whether timer was running
  bool wifiWasConnected;   // WiFi connection state
  char lastStateName[16];  // Last state name for debugging
  uint32_t sleepReason;    // Reason for entering sleep
} RTC_DATA_ATTR rtc_sleep_state_t;

static RTC_DATA_ATTR rtc_sleep_state_t rtc_state = {0};

void SleepState::saveStateToRTC()
{
  ESP_LOGI(getLogTag(), "Saving state to RTC memory");
  
  // Clear structure
  memset(&rtc_state, 0, sizeof(rtc_state));
  
  // Set magic number for validation
  rtc_state.magic = 0xDEADBEEF;
  
  // Save current state information
  const char* currentStateName = stateMachine.getCurrentState()->getStateName();
  strncpy(rtc_state.lastStateName, currentStateName, sizeof(rtc_state.lastStateName) - 1);
  
  // Map state names to IDs for compact storage
  if (strcmp(currentStateName, "IdleState") == 0) {
    rtc_state.lastState = 1;
  } else if (strcmp(currentStateName, "AdjustState") == 0) {
    rtc_state.lastState = 2;
  } else if (strcmp(currentStateName, "TimerState") == 0) {
    rtc_state.lastState = 3;
  } else if (strcmp(currentStateName, "PausedState") == 0) {
    rtc_state.lastState = 4;
  } else {
    rtc_state.lastState = 1; // Default to idle
  }
  
  // Save WiFi state
  rtc_state.wifiWasConnected = (WiFi.status() == WL_CONNECTED);
  
  // TODO: Save timer and project state when available
  // rtc_state.currentProject = stateMachine.getCurrentProjectIndex();
  // rtc_state.timerDuration = stateMachine.getTimerDuration();
  // rtc_state.timerRemaining = stateMachine.getTimerRemaining();
  // rtc_state.timerWasActive = stateMachine.isTimerActive();
  
  rtc_state.sleepReason = isDeepSleep ? 1 : 0; // 1 = power button, 0 = inactivity
  
  // Calculate CRC32 for integrity check
  rtc_state.crc32 = 0; // Clear CRC field before calculation
  // Simple checksum for now - can be enhanced with proper CRC32 later
  rtc_state.crc32 = rtc_state.magic + rtc_state.lastState + rtc_state.currentProject;
  
  ESP_LOGI(getLogTag(), "RTC state saved: state=%s, wifi=%d", 
           rtc_state.lastStateName, rtc_state.wifiWasConnected);
}

void SleepState::restoreStateFromRTC()
{
  // Check magic number and CRC
  if (rtc_state.magic != 0xDEADBEEF) {
    ESP_LOGW(getLogTag(), "RTC state invalid - cold boot or corruption");
    return;
  }
  
  uint32_t expectedCrc = rtc_state.magic + rtc_state.lastState + rtc_state.currentProject;
  if (rtc_state.crc32 != expectedCrc) {
    ESP_LOGW(getLogTag(), "RTC state CRC mismatch - data corruption");
    return;
  }
  
  ESP_LOGI(getLogTag(), "Restoring state from RTC memory: %s", rtc_state.lastStateName);
  
  // Restore WiFi state
  wifiWasConnected = rtc_state.wifiWasConnected;
  
  // TODO: Restore timer and project state when available
  // stateMachine.setCurrentProjectIndex(rtc_state.currentProject);
  // stateMachine.setTimerDuration(rtc_state.timerDuration);
  
  // Determine which state to return to
  switch (rtc_state.lastState) {
    case 1: // IdleState
      ESP_LOGI(getLogTag(), "Will return to IdleState after initialization");
      break;
    case 2: // AdjustState
      ESP_LOGI(getLogTag(), "Will return to AdjustState after initialization");
      break;
    case 3: // TimerState
      ESP_LOGI(getLogTag(), "Will return to TimerState after initialization");
      break;
    case 4: // PausedState
      ESP_LOGI(getLogTag(), "Will return to PausedState after initialization");
      break;
    default:
      ESP_LOGI(getLogTag(), "Unknown state, defaulting to IdleState");
      break;
  }
}