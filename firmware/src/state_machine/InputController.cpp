#include "include/InputController.h"
#include <esp_timer.h>
#include <driver/gpio.h>

static const char* TAG = "InputController";

// Static instance for interrupt handling
InputController* InputController::instance = nullptr;

InputController::InputController(uint8_t encoderPinA, uint8_t encoderPinB, uint8_t encoderButton) :
  pinA(encoderPinA),
  pinB(encoderPinB),
  pinButton(encoderButton),
  hasButton(encoderButton != 255),
  encoderPosition(0),
  lastEncoderPosition(0),
  lastInterruptTime(0),
  buttonPressed(false),
  buttonPressTime(0),
  rotateHandler(nullptr),
  buttonHandler(nullptr),
  encoderMutex(nullptr),
  debugOutput(false),
  interruptCount(0),
  validRotationCount(0),
  debounceRejectCount(0)
{
  if (instance != nullptr) {
    ESP_LOGW(TAG, "Multiple InputController instances - replacing existing");
  }
  instance = this;
}

InputController::~InputController()
{
  cleanup();
  if (instance == this) {
    instance = nullptr;
  }
}

bool InputController::begin()
{
  ESP_LOGI(TAG, "Initializing InputController on pins A=%d, B=%d%s", 
           pinA, pinB, hasButton ? String(", Button=" + String(pinButton)).c_str() : "");
  
  if (!validatePins()) {
    ESP_LOGE(TAG, "Invalid pin configuration");
    return false;
  }
  
  // Create mutex for thread safety
  encoderMutex = xSemaphoreCreateMutex();
  if (!encoderMutex) {
    ESP_LOGE(TAG, "Failed to create encoder mutex");
    return false;
  }
  
  // Configure encoder pins
  pinMode(pinA, INPUT_PULLUP);
  pinMode(pinB, INPUT_PULLUP);
  
  if (hasButton) {
    pinMode(pinButton, INPUT_PULLUP);
  }
  
  // Setup interrupts
  setupInterrupts();
  
  // Initialize state
  encoderPosition = 0;
  lastEncoderPosition = 0;
  interruptCount = 0;
  validRotationCount = 0;
  debounceRejectCount = 0;
  
  ESP_LOGI(TAG, "InputController initialized successfully");
  return true;
}

void InputController::update()
{
  if (!encoderMutex) return;
  
  // Check for encoder changes with thread safety
  if (xSemaphoreTake(encoderMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    int currentPosition = encoderPosition;
    int delta = currentPosition - lastEncoderPosition;
    
    if (delta != 0) {
      lastEncoderPosition = currentPosition;
      
      if (rotateHandler) {
        rotateHandler(delta);
      }
      
      if (debugOutput) {
        logEncoderEvent(delta);
      }
      
      validRotationCount++;
    }
    
    // Handle button if present
    if (hasButton && buttonPressed) {
      uint32_t currentTime = millis();
      if (currentTime - buttonPressTime > BUTTON_DEBOUNCE_MS) {
        buttonPressed = false;
        
        if (buttonHandler) {
          buttonHandler();
        }
        
        if (debugOutput) {
          ESP_LOGI(TAG, "Button pressed");
        }
      }
    }
    
    xSemaphoreGive(encoderMutex);
  }
}

void InputController::cleanup()
{
  ESP_LOGI(TAG, "Cleaning up InputController");
  
  cleanupInterrupts();
  releaseHandlers();
  
  if (encoderMutex) {
    vSemaphoreDelete(encoderMutex);
    encoderMutex = nullptr;
  }
}

void InputController::onEncoderRotateHandler(std::function<void(int delta)> handler)
{
  rotateHandler = handler;
}

void InputController::onEncoderButtonHandler(std::function<void()> handler)
{
  buttonHandler = handler;
}

int InputController::getEncoderPosition() const
{
  return encoderPosition;
}

void InputController::resetEncoderPosition()
{
  if (xSemaphoreTake(encoderMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    encoderPosition = 0;
    lastEncoderPosition = 0;
    xSemaphoreGive(encoderMutex);
    
    if (debugOutput) {
      ESP_LOGI(TAG, "Encoder position reset");
    }
  }
}

void InputController::releaseHandlers()
{
  rotateHandler = nullptr;
  buttonHandler = nullptr;
  ESP_LOGI(TAG, "All handlers released");
}

bool InputController::isEncoderConnected() const
{
  // Simple connectivity test - check if pins are not floating
  int readA1 = digitalRead(pinA);
  int readB1 = digitalRead(pinB);
  
  delay(1);
  
  int readA2 = digitalRead(pinA);
  int readB2 = digitalRead(pinB);
  
  // If pins are floating, they might change between reads
  // Stable reads suggest proper pull-ups and connection
  return (readA1 == readA2) && (readB1 == readB2);
}

void InputController::enableDebugOutput(bool enable)
{
  debugOutput = enable;
  if (enable) {
    ESP_LOGI(TAG, "Debug output enabled");
    dumpEncoderState();
  }
}

void IRAM_ATTR InputController::encoderInterruptHandler()
{
  if (instance) {
    instance->handleEncoderChange();
  }
}

void IRAM_ATTR InputController::buttonInterruptHandler()
{
  if (instance) {
    instance->handleButtonPress();
  }
}

void IRAM_ATTR InputController::handleEncoderChange()
{
  uint32_t currentTime = esp_timer_get_time();
  
  // Debounce check
  if (currentTime - lastInterruptTime < DEBOUNCE_TIME_US) {
    debounceRejectCount++;
    return;
  }
  
  // Rate limiting check
  if (currentTime - lastInterruptTime < MAX_ROTATION_RATE_US) {
    debounceRejectCount++;
    return;
  }
  
  lastInterruptTime = currentTime;
  interruptCount++;
  
  // Read current pin states
  int stateA = digitalRead(pinA);
  int stateB = digitalRead(pinB);
  
  // Determine rotation direction using standard quadrature decoding
  static int lastStateA = HIGH;
  static int lastStateB = HIGH;
  
  if (stateA != lastStateA) {
    if (stateA == LOW) {
      // A went from HIGH to LOW
      if (stateB == HIGH) {
        encoderPosition++; // Clockwise
      } else {
        encoderPosition--; // Counter-clockwise
      }
    }
  }
  
  lastStateA = stateA;
  lastStateB = stateB;
}

void IRAM_ATTR InputController::handleButtonPress()
{
  if (!hasButton) return;
  
  uint32_t currentTime = millis();
  
  // Simple debounce and edge detection
  if (digitalRead(pinButton) == LOW && !buttonPressed) {
    buttonPressed = true;
    buttonPressTime = currentTime;
  }
}

bool InputController::validatePins() const
{
  // Check if pins are valid GPIO numbers for ESP32-S3
  if (pinA >= GPIO_NUM_MAX || pinB >= GPIO_NUM_MAX) {
    ESP_LOGE(TAG, "Invalid GPIO pin numbers");
    return false;
  }
  
  if (hasButton && pinButton >= GPIO_NUM_MAX) {
    ESP_LOGE(TAG, "Invalid button GPIO pin number");
    return false;
  }
  
  if (pinA == pinB) {
    ESP_LOGE(TAG, "Encoder pins A and B cannot be the same");
    return false;
  }
  
  return true;
}

void InputController::logEncoderEvent(int delta) const
{
  ESP_LOGI(TAG, "Encoder: delta=%d, position=%d, interrupts=%u, valid=%u, rejected=%u", 
           delta, encoderPosition, interruptCount, validRotationCount, debounceRejectCount);
}

void InputController::setupInterrupts()
{
  ESP_LOGI(TAG, "Setting up interrupts on pins %d and %d", pinA, pinB);
  
  // Attach interrupts for both encoder pins
  attachInterrupt(digitalPinToInterrupt(pinA), encoderInterruptHandler, CHANGE);
  attachInterrupt(digitalPinToInterrupt(pinB), encoderInterruptHandler, CHANGE);
  
  if (hasButton) {
    attachInterrupt(digitalPinToInterrupt(pinButton), buttonInterruptHandler, FALLING);
    ESP_LOGI(TAG, "Button interrupt attached to pin %d", pinButton);
  }
}

void InputController::cleanupInterrupts()
{
  detachInterrupt(digitalPinToInterrupt(pinA));
  detachInterrupt(digitalPinToInterrupt(pinB));
  
  if (hasButton) {
    detachInterrupt(digitalPinToInterrupt(pinButton));
  }
  
  ESP_LOGI(TAG, "Interrupts cleaned up");
}

bool InputController::testEncoderConnectivity()
{
  ESP_LOGI(TAG, "Testing encoder connectivity...");
  
  bool connected = isEncoderConnected();
  
  ESP_LOGI(TAG, "Pin A: %d, Pin B: %d", digitalRead(pinA), digitalRead(pinB));
  ESP_LOGI(TAG, "Encoder connectivity: %s", connected ? "GOOD" : "POOR");
  
  if (hasButton) {
    ESP_LOGI(TAG, "Button pin %d: %d", pinButton, digitalRead(pinButton));
  }
  
  return connected;
}

void InputController::dumpEncoderState() const
{
  ESP_LOGI(TAG, "=== Encoder State Dump ===");
  ESP_LOGI(TAG, "Pin A: %d, Pin B: %d%s", pinA, pinB, 
           hasButton ? String(", Button: " + String(pinButton)).c_str() : "");
  ESP_LOGI(TAG, "Position: %d", encoderPosition);
  ESP_LOGI(TAG, "Interrupt count: %u", interruptCount);
  ESP_LOGI(TAG, "Valid rotations: %u", validRotationCount);
  ESP_LOGI(TAG, "Debounce rejects: %u", debounceRejectCount);
  ESP_LOGI(TAG, "Handlers: rotate=%s, button=%s", 
           rotateHandler ? "SET" : "NULL",
           buttonHandler ? "SET" : "NULL");
  ESP_LOGI(TAG, "Current pin states: A=%d, B=%d%s", 
           digitalRead(pinA), digitalRead(pinB),
           hasButton ? String(", Button=" + String(digitalRead(pinButton))).c_str() : "");
}

uint32_t InputController::getInterruptCount() const
{
  return interruptCount;
}

// Global instance
InputController* g_inputController = nullptr;