#include "controllers/LEDController.h"
#include <Arduino.h> // Ensure Arduino types (min, uint32_t, etc.) are included
#include <cmath>     // Include for fmodf

// Define animation constants locally in this file
const float RADAR_SWEEP_SPEED_LEDS_PER_SEC = 1.0f; // Halved speed
const uint8_t RADAR_SWEEP_TAIL_LENGTH = 12; // Longer tail (assuming 16 LEDs total for ~4 gap)

#define LED_OFFSET -1 // Offset to align 12 o'clock position

LEDController::LEDController(uint8_t ledPin, uint16_t numLeds, uint8_t brightness)
    : leds(numLeds, ledPin),
      numLeds(numLeds),
      brightness(brightness),
      currentAnimation(None),
      lastUpdateTime(0),
      currentStep(0),
      currentCycle(0),
      decayStarted(false),
      previewMode(false),
      lastColor(0),
      lastAnimation(None) {}

void LEDController::begin()
{
  leds.begin();
  leds.setBrightness(brightness);
  leds.show();
}

void LEDController::update()
{
  switch (currentAnimation)
  {
  case FillAndDecay:
    handleFillAndDecay();
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
  default:
    break;
  }
}

void LEDController::startFillAndDecay(uint32_t color, uint32_t totalDuration)
{
  stopCurrentAnimation();
  currentAnimation = FillAndDecay;
  animationColor = color;
  animationDuration = totalDuration;
  currentStep = 0;
  pixelIndex = 0;
  brightnessLevel = brightness;
  decayStarted = false;
  lastUpdateTime = millis();
  Serial.printf("LED: Starting FillAndDecay. Color: %06X, Duration: %lu ms\n", color, totalDuration);
}

void LEDController::setSpinner(uint32_t color, int cycles)
{
  stopCurrentAnimation();
  currentAnimation = Spinner;
  animationColor = color;
  animationCycles = cycles;
  currentCycle = 0;
  currentStep = 0;
  lastUpdateTime = millis();
}

void LEDController::setBreath(uint32_t color, int cycles, bool endFilled, uint32_t speed)
{
  stopCurrentAnimation();
  currentAnimation = Breath;
  animationColor = color;
  animationCycles = cycles;
  this->endFilled = endFilled;
  animationSpeed = speed;
  currentCycle = 0;
  currentStep = 0;
  lastUpdateTime = millis();
}

void LEDController::setSolid(uint32_t color)
{
  stopCurrentAnimation();
  leds.fill(color);
  leds.show();
}

void LEDController::turnOff()
{
  stopCurrentAnimation();
  leds.clear();
  leds.show();
}

uint32_t LEDController::scaleColor(uint32_t color, uint8_t brightnessLevel)
{
  uint8_t r = (color >> 16 & 0xFF) * brightnessLevel / 255;
  uint8_t g = (color >> 8 & 0xFF) * brightnessLevel / 255;
  uint8_t b = (color & 0xFF) * brightnessLevel / 255;
  return leds.Color(r, g, b);
}

void LEDController::handleFillAndDecay()
{
  // Original FillAndDecay Logic
  uint32_t fillDuration = 300; // Initial quick fill duration in ms
  // Ensure decayDuration is not negative if animationDuration is very short
  uint32_t decayDuration = (animationDuration > fillDuration) ? (animationDuration - fillDuration) : 0;

  // Calculate total steps based on number of LEDs and brightness levels
  // Each LED fades through 'brightness' levels
  uint32_t totalSteps = numLeds * brightness;
  // Calculate duration per step, avoid division by zero
  uint32_t stepDuration = (totalSteps > 0) ? (decayDuration / totalSteps) : 0;

  // Phase 1: Quick Fill
  if (currentStep < numLeds)
  {
    // Calculate duration for each LED to turn on during the fill phase
    uint32_t stepDurationFill = (numLeds > 0) ? (fillDuration / numLeds) : 0;

    // Check if it's time to light up the next LED (or if duration is zero)
    if (stepDurationFill == 0 || millis() - lastUpdateTime >= stepDurationFill)
    {
      // Calculate the index with offset, wrapping around
      int adjustedIndex = (currentStep + LED_OFFSET + numLeds) % numLeds;
      // Set the pixel to the full animation color
      uint32_t setColor = scaleColor(animationColor, brightness);
      leds.setPixelColor(adjustedIndex, setColor);
      leds.show();
      currentStep++;
      lastUpdateTime = millis();
    }
  }
  // Phase 2: Decay
  else
  {
    // Initialize decay phase only once
    if (!decayStarted)
    {
      decayStarted = true;
      pixelIndex = 0;               // Start decaying from the first pixel
      brightnessLevel = brightness; // Start at full brightness for decay
      lastUpdateTime = millis();
      Serial.println("LED: Decay phase started.");
    }

    // Check if it's time for the next decay step (and if decay is needed)
    if (stepDuration > 0 && millis() - lastUpdateTime >= stepDuration)
    {
      lastUpdateTime = millis();

      // If current pixel still has brightness, decrease it
      if (brightnessLevel > 0)
      {
        brightnessLevel--;
        // Calculate the index of the pixel currently being decayed
        int adjustedIndex = (pixelIndex + LED_OFFSET + numLeds) % numLeds;
        uint32_t setColor = scaleColor(animationColor, brightnessLevel);
        leds.setPixelColor(adjustedIndex, setColor);
        leds.show();
      }
      // If current pixel brightness reached zero, turn it off and move to the next
      else
      {
        int adjustedIndex = (pixelIndex + LED_OFFSET + numLeds) % numLeds;
        leds.setPixelColor(adjustedIndex, 0); // Turn off the pixel
        leds.show();
        pixelIndex++;                    // Move to the next pixel
        brightnessLevel = brightness; // Reset brightness for the next pixel decay
      }

      // Check if all pixels have completed their decay
      if (pixelIndex >= numLeds)
      {
        Serial.println("LED: FillAndDecay finished.");
        stopCurrentAnimation(); // Animation complete
      }
    }
    // Handle cases where decay duration is zero (turn off immediately after fill)
    else if (stepDuration == 0 && decayStarted && pixelIndex < numLeds)
    {
       Serial.println("LED: Decay duration is zero, clearing LEDs.");
       turnOff(); // Use turnOff to clear and stop animation
    }
  }
}

void LEDController::handleSpinner()
{
  uint32_t stepDuration = 100;
  if (millis() - lastUpdateTime >= stepDuration)
  {
    leds.clear();
    for (int i = 0; i < numLeds; i++)
    {
      leds.setPixelColor((i + currentStep) % numLeds, scaleColor(animationColor, i * 255 / numLeds));
    }
    leds.show();
    currentStep++;
    lastUpdateTime = millis();

    if (currentStep >= numLeds)
    {
      currentStep = 0;
      currentCycle++;
      if (animationCycles != -1 && currentCycle >= animationCycles)
      {
        stopCurrentAnimation();
      }
    }
  }
}

void LEDController::handleBreath()
{
  if (millis() - lastUpdateTime >= animationSpeed)
  {
    uint8_t fadeBrightness = (currentStep <= 127) ? currentStep * 2 : (255 - currentStep) * 2;

    for (int i = 0; i < numLeds; i++)
    {
      leds.setPixelColor(i, scaleColor(animationColor, fadeBrightness));
    }
    leds.show();
    currentStep++;

    if (currentStep >= 255)
    {
      currentStep = 0;
      currentCycle++;

      // Adjust the number of cycles if `endFilled` is true
      int effectiveCycles = animationCycles;
      if (endFilled && effectiveCycles > 0)
      {
        effectiveCycles--;
      }

      if (effectiveCycles != -1 && currentCycle >= effectiveCycles)
      {
        if (endFilled)
        {
          // Additional half cycle to fill the LEDs
          for (int i = 0; i < numLeds; i++)
          {
            leds.setPixelColor(i, animationColor);
          }
          leds.show();
        }
        else
        {
          turnOff();
        }
        stopCurrentAnimation();
      }
    }
    lastUpdateTime = millis();
  }
}

void LEDController::handleRadarSweep()
{
  unsigned long currentTime = millis();
  unsigned long dt = currentTime - lastUpdateTime;

  // Calculate position increment based on speed and elapsed time
  float increment = (RADAR_SWEEP_SPEED_LEDS_PER_SEC * dt) / 1000.0f;
  if (numLeds == 0) return;
  // Use fmodf explicitly for float modulo
  // sweepPosition = fmodf(sweepPosition + increment, (float)numLeds); // Original CW logical attempt
  // To achieve visual CW, decrement the logical position
  sweepPosition = fmodf(sweepPosition - increment + numLeds, (float)numLeds);

  lastUpdateTime = currentTime;

  leds.clear();

  int leadPixel = (int)sweepPosition;

  // Draw the fading tail
  uint8_t tailLength = min((uint8_t)RADAR_SWEEP_TAIL_LENGTH, (uint8_t)numLeds);
  if (tailLength == 0) return;

  for (int i = 0; i < tailLength; ++i)
  {
    // Calculate index clockwise FROM the head position
    int currentPixelIndex = (leadPixel + i) % numLeds;

    // Brightness: Head (i=0) is brightest, fade for tail (i>0)
    uint8_t brightnessLevel;
    if (i == 0) {
        brightnessLevel = 255; // Head is full brightness
    } else {
        // Ensure tailLength is not 0 to avoid division by zero
        // brightnessLevel = (tailLength > 0) ? (255 * (tailLength - i)) / tailLength : 0; // Previous linear fade
        // Apply non-linear (quadratic) fade for a steeper drop-off
        if (tailLength > 1) {
            float normalized_pos_from_end = (float)(tailLength - 1 - i) / (tailLength - 1);
            float fade_factor = normalized_pos_from_end * normalized_pos_from_end; // Square the factor
            brightnessLevel = (uint8_t)(255.0f * fade_factor);
        } else {
            brightnessLevel = 0; // Should not happen if head is i=0, but safe fallback
        }
    }

    uint32_t dimmedColor = scaleColor(sweepColor, brightnessLevel);
    leds.setPixelColor(currentPixelIndex, dimmedColor);
  }

  leds.show();
}

void LEDController::stopCurrentAnimation()
{
  currentAnimation = None;
  currentStep = 0;
  currentCycle = 0;
  pixelIndex = 0;
  brightnessLevel = brightness;
  decayStarted = false;
  lastUpdateTime = millis();
}

void LEDController::printDebugInfo()
{
  Serial.printf("Anim: %d, Step: %d, Cycle: %d, PixelIdx: %d, Leds numb: %d, Brightness: %d, Color: 0x%06X, Dur: %lu, Speed: %lu, Cycles: %d, EndFilled: %d\n",
                currentAnimation, currentStep, currentCycle, pixelIndex, numLeds, brightness, animationColor, animationDuration, animationSpeed, animationCycles, endFilled);
}

// Implement preview mode methods

void LEDController::setPreviewMode(bool enabled)
{
  if (enabled && !previewMode)
  {
    // Entering preview mode
    saveCurrentState();
    previewMode = true;
  }
  else if (!enabled && previewMode)
  {
    // Exiting preview mode
    previewMode = false;
    restoreLastState();
  }
}

void LEDController::setPreviewColor(const String &hexColor)
{
  // First, make sure we're in preview mode
  if (!previewMode)
  {
    saveCurrentState();
    previewMode = true;
  }

  // Set the color on the LEDs
  uint32_t color = hexColorToUint32(hexColor);
  setSolid(color);
  Serial.printf("LED preview color set to: %s (0x%06X)\n", hexColor.c_str(), color);
}

void LEDController::resetPreviewColor()
{
  if (previewMode)
  {
    previewMode = false;
    restoreLastState();
    Serial.println("LED preview mode exited, restored previous state");
  }
}

bool LEDController::isInPreviewMode() const
{
  return previewMode;
}

// Helper methods for saving and restoring state

void LEDController::saveCurrentState()
{
  lastAnimation = currentAnimation;
  lastColor = animationColor;
  Serial.printf("Saved LED state: animation=%d, color=0x%06X\n",
                lastAnimation, lastColor);
}

void LEDController::restoreLastState()
{
  Serial.printf("Restoring LED state: animation=%d, color=0x%06X\n",
                lastAnimation, lastColor);

  switch (lastAnimation)
  {
  case None:
    turnOff();
    break;
  case FillAndDecay:
    startFillAndDecay(lastColor, animationDuration);
    break;
  case Spinner:
    setSpinner(lastColor, animationCycles);
    break;
  case Breath:
    setBreath(lastColor, animationCycles, endFilled, animationSpeed);
    break;
  default:
    setSolid(lastColor);
    break;
  }
}

// New function to start the Radar Sweep animation
void LEDController::startRadarSweep(uint32_t color)
{
  stopCurrentAnimation();
  currentAnimation = RadarSweep;
  sweepColor = color;
  sweepPosition = 0.0f;
  lastUpdateTime = millis(); // Use common lastUpdateTime for timing
  Serial.printf("LED: Starting RadarSweep. Color: %06X\n", color);
}