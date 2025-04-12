#include "controllers/LEDController.h"

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
  // --- DEBUG: Simplify to just set all LEDs to animationColor ---
  if (!decayStarted)
  { // Use decayStarted flag to run this only once
    Serial.printf("LED DEBUG: Setting all LEDs to %06X\n", animationColor);
    leds.fill(animationColor); // Use the raw animationColor directly
    leds.show();
    decayStarted = true; // Set flag so it doesn't repeat
  }
  // Do nothing else, just keep the color solid

  /* Original Logic:
  uint32_t fillDuration = 300; // Initial fill duration
  uint32_t decayDuration = animationDuration - fillDuration;
  uint32_t totalSteps = (numLeds + 1) * brightness;
  uint32_t stepDuration = decayDuration / totalSteps;

  if (currentStep < numLeds)
  {
    // Quick fill phase
    uint32_t stepDurationFill = fillDuration / numLeds;
    if (millis() - lastUpdateTime >= stepDurationFill)
    {
      int adjustedIndex = (currentStep + LED_OFFSET + numLeds) % numLeds;
      uint32_t setColor = scaleColor(animationColor, brightness);
      leds.setPixelColor(adjustedIndex, setColor);
      leds.show();
      currentStep++;
      lastUpdateTime = millis();
    }
  }
  else
  {
    // Initialize decay phase
    if (!decayStarted)
    {
      decayStarted = true;
      pixelIndex = 0;
      brightnessLevel = brightness;
      lastUpdateTime = millis();
      Serial.println("LED: Decay phase started.");
    }

    // Decay phase
    if (millis() - lastUpdateTime >= stepDuration)
    {
      lastUpdateTime = millis();

      if (brightnessLevel > 0)
      {
        brightnessLevel--;
        int adjustedIndex = (pixelIndex + LED_OFFSET + numLeds) % numLeds;
        uint32_t setColor = scaleColor(animationColor, brightnessLevel);
        leds.setPixelColor(adjustedIndex, setColor);
        leds.show();
      }
      else
      {
        int adjustedIndex = (pixelIndex + LED_OFFSET + numLeds) % numLeds;
        leds.setPixelColor(adjustedIndex, 0);
        leds.show();
        pixelIndex++;
        brightnessLevel = brightness;
      }

      if (pixelIndex >= numLeds)
      {
        Serial.println("LED: FillAndDecay finished.");
        stopCurrentAnimation();
      }
    }
  }
  */
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