#include "include/LEDController.h"
#include <esp_timer.h>
#include <esp_task_wdt.h>
#include <cmath>
#include "../BrandColors.h"

static const char* TAG = "LEDController";

// Animation constants for 24-LED ring
const float LEDController::RADAR_SWEEP_SPEED_LEDS_PER_SEC = 2.0f;
const uint8_t LEDController::RADAR_SWEEP_TAIL_LENGTH = 8;
const int LEDController::LED_OFFSET = 18; // Rotate 180 degrees to align 12 o'clock correctly (6 + 12 = 18)
const uint32_t LEDController::DEFAULT_UPDATE_INTERVAL_MS = 50;

// Global instance
LEDController* g_ledController = nullptr;

LEDController::LEDController(uint8_t ledPin, uint16_t numLeds, uint8_t brightness) :
  ledPin(ledPin),
  numLeds(numLeds),
  brightness(brightness),
  leds(numLeds, ledPin, NEO_GRB + NEO_KHZ800), // GRB format - standard for WS2812 LEDs
  currentAnimation(None),
  animationColor(0),
  animationR(0),
  animationG(0),
  animationB(0),
  animationDuration(0),
  animationSpeed(1000),
  animationProgress(0.0f),
  lastUpdateTime(0),
  animationStartTime(0),
  currentStep(0),
  currentCycle(0),
  animationCycles(-1),
  endFilled(false),
  decayStarted(false),
  sweepPosition(0.0f),
  previewMode(false),
  lastColor(0),
  lastAnimation(None),
  ledMutex(nullptr),
  debugOutput(false),
  updateCount(0),
  totalUpdateTime(0)
{
}

LEDController::~LEDController()
{
  cleanup();
}

bool LEDController::begin()
{
  ESP_LOGI(TAG, "Initializing LEDController on pin %d with %d LEDs", ledPin, numLeds);
  
  if (!validateConfiguration()) {
    ESP_LOGE(TAG, "Invalid LED configuration");
    return false;
  }
  
  // Create mutex for thread safety
  ledMutex = xSemaphoreCreateMutex();
  if (!ledMutex) {
    ESP_LOGE(TAG, "Failed to create LED mutex");
    return false;
  }
  
  // Initialize NeoPixel strip
  leds.begin();
  // DO NOT call setBrightness() - it permanently corrupts colors!
  // Brightness scaling will be done in color calculations instead
  clearAll();
  showLEDs();
  
  ESP_LOGI(TAG, "LEDController initialized successfully");
  ESP_LOGI(TAG, "Configuration: %d LEDs, brightness %d, pin %d", numLeds, brightness, ledPin);
  
  return true;
}

void LEDController::update()
{
  if (!ledMutex) return;
  
  uint64_t startTime = esp_timer_get_time();
  
  if (xSemaphoreTake(ledMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    unsigned long currentTime = millis();
    
    // Throttle updates to prevent overwhelming the system
    if (currentTime - lastUpdateTime >= DEFAULT_UPDATE_INTERVAL_MS) {
      switch (currentAnimation) {
        case TimerProgress:
          handleTimerProgress();
          break;
        case Spinner:
          handleSpinner();
          break;
        case Breath:
          handleBreath();
          break;
        case RadarSweep:
          handleRadarSweep();
          break;
        case FillAndDecay:
          handleFillAndDecay();
          break;
        case Solid:
          // No update needed for solid color
          break;
        case None:
        default:
          break;
      }
      
      lastUpdateTime = currentTime;
      updateCount++;
    }
    
    xSemaphoreGive(ledMutex);
  }
  
  uint64_t endTime = esp_timer_get_time();
  totalUpdateTime += (endTime - startTime);
}

void LEDController::cleanup()
{
  ESP_LOGI(TAG, "Cleaning up LEDController");
  
  turnOff();
  
  if (ledMutex) {
    vSemaphoreDelete(ledMutex);
    ledMutex = nullptr;
  }
}

void LEDController::startTimerProgress(uint32_t color, float progress)
{
  if (xSemaphoreTake(ledMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    stopCurrentAnimation();
    currentAnimation = TimerProgress;
    
    // Extract RGB components from input color
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    
    // Store RGB components for animation use
    animationR = r;
    animationG = g;
    animationB = b;
    animationColor = color;
    animationProgress = constrain(progress, 0.0f, 1.0f);
    
    if (debugOutput) {
      logAnimationChange("TimerProgress");
      ESP_LOGI(TAG, "Timer progress: %.1f%% with RGB color 0x%06X (R=%d G=%d B=%d)", 
               progress * 100.0f, color, r, g, b);
    }
    
    xSemaphoreGive(ledMutex);
  }
}

void LEDController::startSpinner(uint32_t color, int cycles)
{
  if (xSemaphoreTake(ledMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    stopCurrentAnimation();
    currentAnimation = Spinner;
    // Store RGB components separately
    animationR = (color >> 16) & 0xFF;
    animationG = (color >> 8) & 0xFF;
    animationB = color & 0xFF;
    animationColor = color;
    animationCycles = cycles;
    currentCycle = 0;
    currentStep = 0;
    animationStartTime = millis();
    
    if (debugOutput) {
      logAnimationChange("Spinner");
    }
    
    xSemaphoreGive(ledMutex);
  }
}

void LEDController::startBreath(uint32_t color, int cycles, uint32_t speed)
{
  if (xSemaphoreTake(ledMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    stopCurrentAnimation();
    currentAnimation = Breath;
    // Store RGB components from input color
    animationR = (color >> 16) & 0xFF;
    animationG = (color >> 8) & 0xFF;
    animationB = color & 0xFF;
    animationColor = color;
    animationCycles = cycles;
    animationSpeed = speed;
    currentCycle = 0;
    currentStep = 0;
    animationStartTime = millis();
    
    if (debugOutput) {
      logAnimationChange("Breath");
      ESP_LOGI(TAG, "Starting breath animation with RGB color 0x%06X (R=%d G=%d B=%d)", 
               color, animationR, animationG, animationB);
    }
    
    xSemaphoreGive(ledMutex);
  }
}

void LEDController::startRadarSweep(uint32_t color)
{
  if (xSemaphoreTake(ledMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    stopCurrentAnimation();
    currentAnimation = RadarSweep;
    // Store RGB components
    animationR = (color >> 16) & 0xFF;
    animationG = (color >> 8) & 0xFF;
    animationB = color & 0xFF;
    animationColor = color;
    sweepPosition = 0.0f;
    animationStartTime = millis();
    
    if (debugOutput) {
      logAnimationChange("RadarSweep");
    }
    
    xSemaphoreGive(ledMutex);
  }
}

void LEDController::startFillAndDecay(uint32_t color, uint32_t totalDuration)
{
  if (xSemaphoreTake(ledMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    stopCurrentAnimation();
    currentAnimation = FillAndDecay;
    // Store RGB components
    animationR = (color >> 16) & 0xFF;
    animationG = (color >> 8) & 0xFF;
    animationB = color & 0xFF;
    animationColor = color;
    animationDuration = totalDuration;
    currentStep = 0;
    decayStarted = false;
    animationStartTime = millis();
    
    if (debugOutput) {
      logAnimationChange("FillAndDecay");
    }
    
    xSemaphoreGive(ledMutex);
  }
}

void LEDController::setSolid(uint32_t color)
{
  ESP_LOGI(TAG, "setSolid called with RGB color: 0x%06X", color);
  
  if (xSemaphoreTake(ledMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    stopCurrentAnimation();
    currentAnimation = Solid;
    
    // Extract RGB components from input color
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    
    // Store RGB components for reference
    animationR = r;
    animationG = g;
    animationB = b;
    animationColor = color;
    
    // Apply color calibration for WS2812 LED characteristics
    uint32_t libraryColor = convertColorForLED(color);
    
    // Log color values for debugging
    ESP_LOGI(TAG, "setSolid: RGB color=0x%06X (R=%d G=%d B=%d), libraryColor=0x%08X", 
             color, r, g, b, libraryColor);
    
    ESP_LOGI(TAG, "Setting %d pixels...", numLeds);
    for (uint16_t i = 0; i < numLeds; i++) {
      setPixelColor(i, libraryColor);
    }
    
    ESP_LOGI(TAG, "Calling showLEDs()...");
    showLEDs();
    ESP_LOGI(TAG, "showLEDs() complete");
    
    if (debugOutput) {
      logAnimationChange("Solid");
    }
    
    xSemaphoreGive(ledMutex);
  } else {
    ESP_LOGE(TAG, "Failed to take LED mutex!");
  }
}

void LEDController::turnOff()
{
  if (xSemaphoreTake(ledMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    stopCurrentAnimation();
    clearAll();
    showLEDs();
    
    if (debugOutput) {
      ESP_LOGI(TAG, "LEDs turned off");
    }
    
    xSemaphoreGive(ledMutex);
  }
}

void LEDController::showAlphaGradient(int offset)
{
  if (xSemaphoreTake(ledMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    stopCurrentAnimation();
    clearAll();
    
    // Create a smooth gradient using the brand color
    for (int i = 0; i < numLeds; i++) {
      // Calculate position in gradient (0.0 to 1.0)
      float position = (float)i / (float)numLeds;
      
      // Create a wave pattern that wraps around
      float wave = (sin((position + offset / 100.0f) * 2 * PI) + 1.0f) / 2.0f;
      
      // Scale wave intensity for visible gradient
      wave = wave * 0.4f;  // Max 40% intensity
      
      // Apply wave to brand color and convert for LED
      uint32_t waveColor = ((uint8_t)(BRAND_COLOR_PRIMARY_R * wave) << 16) |
                           ((uint8_t)(BRAND_COLOR_PRIMARY_G * wave) << 8) |
                           (uint8_t)(BRAND_COLOR_PRIMARY_B * wave);
      
      uint16_t adjustedIndex = adjustPixelIndex(i);
      setPixelColor(adjustedIndex, convertColorForLED(waveColor));
    }
    
    showLEDs();
    xSemaphoreGive(ledMutex);
  }
}

void LEDController::showDurationGradient(int minutes, int maxMinutes)
{
  // Use brand color for backward compatibility
  showDurationGradient(minutes, maxMinutes, BRAND_COLOR_PRIMARY);
}

void LEDController::showDurationGradient(int minutes, int maxMinutes, uint32_t color)
{
  if (xSemaphoreTake(ledMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    stopCurrentAnimation();
    clearAll();
    
    // Extract RGB components from input color
    uint8_t colorR = (color >> 16) & 0xFF;
    uint8_t colorG = (color >> 8) & 0xFF;
    uint8_t colorB = color & 0xFF;
    
    // Calculate how many LEDs to light based on duration
    float ratio = (float)minutes / (float)maxMinutes;
    int ledsToLight = (int)(ratio * numLeds);
    
    ESP_LOGI(TAG, "Duration gradient: %d/%d minutes (%.1f%%), lighting %d/%d LEDs with RGB color R=%d G=%d B=%d", 
             minutes, maxMinutes, ratio * 100.0f, ledsToLight, numLeds, colorR, colorG, colorB);
    
    // Light LEDs with gradient (fill from end backwards)
    for (int i = 0; i < ledsToLight; i++) {
      // Fill from the end backwards to reverse direction
      int ledIndex = numLeds - 1 - i;
      
      uint16_t adjustedIndex = adjustPixelIndex(ledIndex);
      
      // Calculate brightness-adjusted color and apply LED conversion
      float intensity = 0.4f; // 40% max
      uint32_t dimmedColor = ((uint8_t)(colorR * intensity) << 16) |
                             ((uint8_t)(colorG * intensity) << 8) |
                             (uint8_t)(colorB * intensity);
      
      setPixelColor(adjustedIndex, convertColorForLED(dimmedColor));
    }
    
    // Add partial LED for smooth progress
    if (ledsToLight < numLeds) {
      float partial = (ratio * numLeds) - ledsToLight;
      if (partial > 0.0f) {
        int partialLedIndex = numLeds - 1 - ledsToLight;
        uint16_t adjustedIndex = adjustPixelIndex(partialLedIndex);
        
        float intensity = partial * 0.4f; // 40% max with partial brightness
        uint32_t partialColor = ((uint8_t)(colorR * intensity) << 16) |
                                ((uint8_t)(colorG * intensity) << 8) |
                                (uint8_t)(colorB * intensity);
        
        setPixelColor(adjustedIndex, convertColorForLED(partialColor));
      }
    }
    
    showLEDs();
    xSemaphoreGive(ledMutex);
  }
}

void LEDController::handleTimerProgress()
{
  clearAll();
  
  // Calculate number of LEDs to light based on progress
  int ledsToLight = (int)(animationProgress * numLeds);
  
  // Apply color calibration to the stored animation color
  uint32_t fullColor = convertColorForLED(animationColor);
  
  // Fill LEDs from end backwards (reverse direction)
  for (int i = 0; i < ledsToLight; i++) {
    int ledIndex = numLeds - 1 - i;
    uint16_t adjustedIndex = adjustPixelIndex(ledIndex);
    setPixelColor(adjustedIndex, fullColor);
  }
  
  // Add partial LED for smooth progress
  if (ledsToLight < numLeds) {
    float partialProgress = (animationProgress * numLeds) - ledsToLight;
    if (partialProgress > 0.0f) {
      // Calculate dimmed color for partial LED
      uint8_t r = (animationColor >> 16) & 0xFF;
      uint8_t g = (animationColor >> 8) & 0xFF; 
      uint8_t b = animationColor & 0xFF;
      
      uint32_t partialColor = ((uint8_t)(r * partialProgress) << 16) |
                              ((uint8_t)(g * partialProgress) << 8) |
                              (uint8_t)(b * partialProgress);
      
      // Place partial LED at the next position in reverse order  
      int partialLedIndex = numLeds - 1 - ledsToLight;
      uint16_t adjustedIndex = adjustPixelIndex(partialLedIndex);
      setPixelColor(adjustedIndex, convertColorForLED(partialColor));
    }
  }
  
  showLEDs();
}

void LEDController::handleSpinner()
{
  unsigned long elapsed = millis() - animationStartTime;
  
  clearAll();
  
  // Calculate spinner position - REVERSED DIRECTION
  int position = numLeds - ((elapsed / 100) % numLeds); // Move every 100ms, reversed
  
  // Light up 3 LEDs with trailing effect
  for (int i = 0; i < 3; i++) {
    int ledIndex = (position + i) % numLeds; // Changed from - to + for reversed trail
    uint16_t adjustedIndex = adjustPixelIndex(ledIndex);
    float brightness = 1.0f - (i * 0.33f); // Fade trail (1.0, 0.67, 0.33)
    
    // Calculate dimmed color and apply LED conversion
    uint8_t r = (animationColor >> 16) & 0xFF;
    uint8_t g = (animationColor >> 8) & 0xFF;
    uint8_t b = animationColor & 0xFF;
    
    uint32_t dimmedColor = ((uint8_t)(r * brightness) << 16) |
                           ((uint8_t)(g * brightness) << 8) |
                           (uint8_t)(b * brightness);
    
    setPixelColor(adjustedIndex, convertColorForLED(dimmedColor));
  }
  
  showLEDs();
  
  // Check for cycle completion
  if (animationCycles > 0) {
    int expectedCycles = elapsed / (numLeds * 100);
    if (expectedCycles >= animationCycles) {
      currentAnimation = None;
    }
  }
}

void LEDController::handleBreath()
{
  unsigned long elapsed = millis() - animationStartTime;
  
  // Calculate breathing intensity using sine wave
  float phase = (elapsed % animationSpeed) / (float)animationSpeed;
  float intensity = (sin(phase * 2 * PI) + 1.0f) / 2.0f; // 0.0 to 1.0
  
  // Use full intensity range for breathing effect
  // Scale from 0.3 to 1.0 (30% to 100%) for visible breathing
  intensity = 0.3f + (intensity * 0.7f);
  
  // Calculate brightness-adjusted color and apply LED conversion
  uint8_t r = (animationColor >> 16) & 0xFF;
  uint8_t g = (animationColor >> 8) & 0xFF;
  uint8_t b = animationColor & 0xFF;
  
  uint32_t dimmedColor = ((uint8_t)(r * intensity) << 16) |
                         ((uint8_t)(g * intensity) << 8) |
                         (uint8_t)(b * intensity);
  
  uint32_t breathColor = convertColorForLED(dimmedColor);
  
  // Set all pixels to the calculated color
  for (uint16_t i = 0; i < numLeds; i++) {
    setPixelColor(i, breathColor);
  }
  
  showLEDs();
  
  // Check for cycle completion
  if (animationCycles > 0) {
    int expectedCycles = elapsed / animationSpeed;
    if (expectedCycles >= animationCycles) {
      currentAnimation = None;
    }
  }
}

void LEDController::handleRadarSweep()
{
  unsigned long elapsed = millis() - animationStartTime;
  
  clearAll();
  
  // Update sweep position - REVERSED DIRECTION
  sweepPosition = numLeds - fmodf(elapsed * RADAR_SWEEP_SPEED_LEDS_PER_SEC / 1000.0f, numLeds);
  
  // Draw sweep with tail
  for (int i = 0; i < RADAR_SWEEP_TAIL_LENGTH; i++) {
    int ledIndex = ((int)sweepPosition + i) % numLeds;  // Changed from - to + for reversed tail
    uint16_t adjustedIndex = adjustPixelIndex(ledIndex);
    
    // Calculate brightness for tail effect
    float brightness = 1.0f - ((float)i / RADAR_SWEEP_TAIL_LENGTH);
    
    // Calculate dimmed color and apply LED conversion
    uint8_t r = (animationColor >> 16) & 0xFF;
    uint8_t g = (animationColor >> 8) & 0xFF;
    uint8_t b = animationColor & 0xFF;
    
    uint32_t dimmedColor = ((uint8_t)(r * brightness) << 16) |
                           ((uint8_t)(g * brightness) << 8) |
                           (uint8_t)(b * brightness);
    
    setPixelColor(adjustedIndex, convertColorForLED(dimmedColor));
  }
  
  showLEDs();
}

void LEDController::handleFillAndDecay()
{
  unsigned long elapsed = millis() - animationStartTime;
  uint32_t halfDuration = animationDuration / 2;
  
  if (elapsed < halfDuration && !decayStarted) {
    // Fill phase
    float progress = (float)elapsed / halfDuration;
    int ledsToLight = (int)(progress * numLeds);
    
    clearAll();
    uint32_t fillColor = convertColorForLED(animationColor);
    for (int i = 0; i < ledsToLight; i++) {
      uint16_t adjustedIndex = adjustPixelIndex(i);
      setPixelColor(adjustedIndex, fillColor);
    }
    showLEDs();
    
    if (ledsToLight >= numLeds) {
      decayStarted = true;
    }
  } else {
    // Decay phase
    float decayProgress = (float)(elapsed - halfDuration) / halfDuration;
    decayProgress = constrain(decayProgress, 0.0f, 1.0f);
    
    float brightness = 1.0f - decayProgress;
    
    // Calculate dimmed color and apply LED conversion
    uint8_t r = (animationColor >> 16) & 0xFF;
    uint8_t g = (animationColor >> 8) & 0xFF;
    uint8_t b = animationColor & 0xFF;
    
    uint32_t dimmedColor = ((uint8_t)(r * brightness) << 16) |
                           ((uint8_t)(g * brightness) << 8) |
                           (uint8_t)(b * brightness);
    
    uint32_t decayColor = convertColorForLED(dimmedColor);
    
    for (uint16_t i = 0; i < numLeds; i++) {
      setPixelColor(i, decayColor);
    }
    showLEDs();
    
    if (decayProgress >= 1.0f) {
      currentAnimation = None;
    }
  }
}

bool LEDController::testIndividualLEDs()
{
  ESP_LOGI(TAG, "Testing individual LEDs...");
  
  for (uint16_t i = 0; i < numLeds; i++) {
    clearAll();
    setPixelColor(i, 0xFF0000); // Red
    showLEDs();
    delay(100); // Reduced from 200ms to 100ms
    
    // Feed watchdog every few LEDs
    if (i % 4 == 0) {
      esp_task_wdt_reset();
    }
    
    if (debugOutput) {
      ESP_LOGI(TAG, "Testing LED %d", i);
    }
  }
  
  clearAll();
  showLEDs();
  
  ESP_LOGI(TAG, "Individual LED test completed");
  return true;
}

bool LEDController::testAllPatterns()
{
  ESP_LOGI(TAG, "Testing all animation patterns...");
  
  // Test solid colors
  setSolid(0xFF0000); // Red
  delay(500); // Reduced delay
  esp_task_wdt_reset();
  setSolid(0x00FF00); // Green
  delay(500);
  esp_task_wdt_reset();
  setSolid(0x0000FF); // Blue
  delay(500);
  esp_task_wdt_reset();
  
  // Test spinner
  startSpinner(0xFFFF00, 2); // Yellow spinner
  for (int i = 0; i < 25; i++) { // Reduced iterations
    update();
    delay(100);
    if (i % 10 == 0) esp_task_wdt_reset();
  }
  
  // Test breath
  startBreath(0xFF00FF, 2); // Magenta breath
  for (int i = 0; i < 25; i++) { // Reduced iterations
    update();
    delay(100);
    if (i % 10 == 0) esp_task_wdt_reset();
  }
  
  // Test radar sweep
  startRadarSweep(0x00FFFF); // Cyan radar
  for (int i = 0; i < 25; i++) { // Reduced iterations
    update();
    delay(100);
    if (i % 10 == 0) esp_task_wdt_reset();
  }
  
  turnOff();
  ESP_LOGI(TAG, "Pattern testing completed");
  return true;
}

bool LEDController::test3V3PowerSufficiency()
{
  ESP_LOGI(TAG, "Testing 3.3V power sufficiency...");
  
  // Test with all LEDs at maximum brightness
  setSolid(0xFFFFFF); // White at full brightness
  delay(1000);
  
  // Check if LEDs are visibly bright
  ESP_LOGI(TAG, "All LEDs should be bright white");
  ESP_LOGI(TAG, "Verify adequate brightness and no flickering");
  
  delay(2000);
  turnOff();
  
  ESP_LOGI(TAG, "3.3V power test completed");
  return true;
}

void LEDController::runDiagnosticSequence()
{
  ESP_LOGI(TAG, "=== Running LED Diagnostic Sequence ===");
  
  dumpLEDState();
  testIndividualLEDs();
  test3V3PowerSufficiency();
  testAllPatterns();
  
  ESP_LOGI(TAG, "=== Diagnostic Sequence Complete ===");
}

// Utility methods
uint32_t LEDController::hexColorToUint32(const String &hexColor)
{
  if (hexColor.length() != 7 || hexColor[0] != '#') {
    ESP_LOGW(TAG, "Invalid hex color format: %s", hexColor.c_str());
    return 0;
  }
  
  return strtol(hexColor.c_str() + 1, NULL, 16);
}

uint32_t LEDController::colorWheel(uint8_t wheelPos)
{
  wheelPos = 255 - wheelPos;
  if (wheelPos < 85) {
    return ((uint32_t)(255 - wheelPos * 3) << 16) | ((uint32_t)(0) << 8) | (wheelPos * 3);
  }
  if (wheelPos < 170) {
    wheelPos -= 85;
    return ((uint32_t)(0) << 16) | ((uint32_t)(wheelPos * 3) << 8) | (255 - wheelPos * 3);
  }
  wheelPos -= 170;
  return ((uint32_t)(wheelPos * 3) << 16) | ((uint32_t)(255 - wheelPos * 3) << 8) | (0);
}

uint32_t LEDController::dimColor(uint32_t color, uint8_t brightness)
{
  uint8_t r = (color >> 16) & 0xFF;
  uint8_t g = (color >> 8) & 0xFF;
  uint8_t b = color & 0xFF;
  
  // Use proper scaling to avoid overflow and color distortion
  r = (r * brightness) / 255;
  g = (g * brightness) / 255;
  b = (b * brightness) / 255;
  
  return leds.Color(r, g, b); // Let the library handle the color format
}

uint32_t LEDController::convertColorForLED(uint32_t displayColor)
{
  // Extract RGB components from display color (sRGB color space)
  uint8_t r_srgb = (displayColor >> 16) & 0xFF;
  uint8_t g_srgb = (displayColor >> 8) & 0xFF;
  uint8_t b_srgb = displayColor & 0xFF;
  
  // Convert sRGB (gamma ~2.2) to linear RGB for LED hardware
  auto sRGBToLinear = [](uint8_t srgb_value) -> float {
    float normalized = srgb_value / 255.0f;
    return pow(normalized, 2.2f);
  };
  
  // Convert to linear RGB
  float r_linear = sRGBToLinear(r_srgb);
  float g_linear = sRGBToLinear(g_srgb);
  float b_linear = sRGBToLinear(b_srgb);
  
  // Apply color temperature and WS2812 characteristic corrections
  // WS2812 has cooler color temperature (~5810K vs sRGB D65 6500K)
  // and green LEDs are typically brighter than red/blue
  r_linear = r_linear * 1.0f;   // Keep red unchanged
  g_linear = g_linear * 0.85f;  // Reduce green (WS2812 green is brighter)
  b_linear = b_linear * 0.80f;  // Reduce blue (compensate for cooler color temp)
  
  // Convert back to 8-bit values for WS2812 (stays in linear space)
  auto linearToWS2812 = [](float linear_value) -> uint8_t {
    return (uint8_t)constrain(linear_value * 255.0f, 0.0f, 255.0f);
  };
  
  uint8_t r_led = linearToWS2812(r_linear);
  uint8_t g_led = linearToWS2812(g_linear);
  uint8_t b_led = linearToWS2812(b_linear);
  
  ESP_LOGV(TAG, "Color conversion: sRGB(0x%06X) -> LED(0x%02X%02X%02X)", 
           displayColor, r_led, g_led, b_led);
  
  return leds.Color(r_led, g_led, b_led);
}

void LEDController::setPixelColor(uint16_t pixel, uint32_t color)
{
  if (pixel < numLeds) {
    leds.setPixelColor(pixel, color);
  }
}

void LEDController::clearAll()
{
  leds.clear();
}

void LEDController::showLEDs()
{
  // Add a small delay to ensure the RMT peripheral has time to process
  delayMicroseconds(50);
  leds.show();
  // Wait for the data to be sent
  delayMicroseconds(300); // 300us reset time for WS2812B
}

uint16_t LEDController::adjustPixelIndex(uint16_t index) const
{
  // Adjust for 12 o'clock alignment
  int adjusted = (index + LED_OFFSET + numLeds) % numLeds;
  return (uint16_t)adjusted;
}

bool LEDController::validateConfiguration() const
{
  if (numLeds == 0 || numLeds > 256) {
    ESP_LOGE(TAG, "Invalid LED count: %d", numLeds);
    return false;
  }
  
  if (ledPin >= GPIO_NUM_MAX) {
    ESP_LOGE(TAG, "Invalid LED pin: %d", ledPin);
    return false;
  }
  
  return true;
}

void LEDController::logAnimationChange(const char* newAnimation) const
{
  ESP_LOGI(TAG, "Animation changed to: %s (color: 0x%06X)", newAnimation, animationColor);
}

void LEDController::stopCurrentAnimation()
{
  currentAnimation = None;
  animationProgress = 0.0f;
  currentStep = 0;
  currentCycle = 0;
  decayStarted = false;
  sweepPosition = 0.0f;
}

bool LEDController::isAnimating() const
{
  return currentAnimation != None;
}

const char* LEDController::getCurrentAnimationName() const
{
  switch (currentAnimation) {
    case TimerProgress: return "TimerProgress";
    case Spinner: return "Spinner";
    case Breath: return "Breath";
    case RadarSweep: return "RadarSweep";
    case FillAndDecay: return "FillAndDecay";
    case Solid: return "Solid";
    case None: return "None";
    default: return "Unknown";
  }
}

void LEDController::dumpLEDState() const
{
  ESP_LOGI(TAG, "=== LED Controller State ===");
  ESP_LOGI(TAG, "Pin: %d, LEDs: %d, Brightness: %d", ledPin, numLeds, brightness);
  ESP_LOGI(TAG, "Current animation: %s", getCurrentAnimationName());
  ESP_LOGI(TAG, "Animation color: 0x%06X", animationColor);
  ESP_LOGI(TAG, "Update count: %u", updateCount);
  ESP_LOGI(TAG, "Preview mode: %s", previewMode ? "ON" : "OFF");
}

void LEDController::enableDebugOutput(bool enable)
{
  debugOutput = enable;
  if (enable) {
    ESP_LOGI(TAG, "Debug output enabled");
    dumpLEDState();
  }
}

void LEDController::setBrightness(uint8_t newBrightness)
{
  brightness = newBrightness;
  // DO NOT use leds.setBrightness() - it's destructive and corrupts colors!
  // Instead, brightness scaling should be done in color calculations
  ESP_LOGI(TAG, "Brightness stored as %d (colors will be scaled in calculations)", brightness);
}

uint8_t LEDController::getBrightness() const
{
  return brightness;
}

uint32_t LEDController::getUpdateCount() const
{
  return updateCount;
}

void LEDController::logPerformanceStats() const
{
  if (updateCount > 0) {
    uint64_t avgUpdateTime = totalUpdateTime / updateCount;
    ESP_LOGI(TAG, "Performance: %u updates, avg %.2f ms per update", 
             updateCount, (float)avgUpdateTime / 1000.0f);
  }
}

// Preview mode methods (simplified for now)
void LEDController::setPreviewMode(bool enabled)
{
  previewMode = enabled;
  if (enabled) {
    saveCurrentState();
  } else {
    restoreLastState();
  }
}

void LEDController::setPreviewColor(const String &hexColor)
{
  // First, make sure we're in preview mode
  if (!previewMode) {
    saveCurrentState();
    previewMode = true;
  }
  
  // Set the color on the LEDs
  uint32_t color = hexColorToUint32(hexColor);
  setSolid(color);
  ESP_LOGI(TAG, "LED preview color set to: %s (0x%06X)", hexColor.c_str(), color);
}

void LEDController::resetPreviewColor()
{
  if (previewMode) {
    previewMode = false;
    restoreLastState();
    ESP_LOGI(TAG, "LED preview mode exited, restored previous state");
  }
}

bool LEDController::isInPreviewMode() const
{
  return previewMode;
}

void LEDController::saveCurrentState()
{
  lastColor = animationColor;
  lastAnimation = currentAnimation;
}

void LEDController::restoreLastState()
{
  if (lastAnimation == Solid) {
    setSolid(lastColor);
  } else {
    currentAnimation = lastAnimation;
    animationColor = lastColor;
  }
}