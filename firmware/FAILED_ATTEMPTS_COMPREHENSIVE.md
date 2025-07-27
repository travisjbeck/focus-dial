# Comprehensive List of Failed Attempts - 4 Days of Failures

## UPDATE - FASTLED ALSO FAILS
- **FastLED 3.10.1** - Initially appeared to work but crashes with same interrupt watchdog timeout after ~3 minutes
- Crash occurs at approximately 3:00 mark (same as all other LED libraries)
- Pattern: "Guru Meditation Error: Core 1 panic'ed (Interrupt wdt timeout on CPU1)"

## Current Problems Still Not Fixed:
1. **Touch wake-up doesn't work** - device doesn't wake at all
2. **Encoder wake-up doesn't work** - device doesn't wake at all
3. **Screen flickers during sleep** - continuous flickering
4. **Device won't wake up from sleep** - completely unresponsive
5. **All LED libraries crash after 3 minutes** - no working LED solution

## FAILED LED FIXES:

### Library Issues
- **Downgraded NeoPixel from 1.15.1 to 1.12.3** - LEDs briefly worked then caused crashes
- **Added delays after RMT init** - didn't prevent crashes
- **Added prepareForSleep() method** - didn't help
- **Disabled brightness calls** - didn't fix color corruption
- **Added watchdog feeds during init** - didn't prevent crashes
- **NeoPixelBus with RMT** - immediate crash: "CONFLICT! driver_ng is not allowed"
- **NeoPixelBus with I2S** - compilation error, method doesn't exist for ESP32-S3
- **Custom bit-banging implementation** - crash loop, LEDs don't work or sleep
- **LiteLED library v2.0.2** - runs initially but crashes with interrupt watchdog timeout after ~3 minutes
- **FastLED 3.10.1** - same interrupt watchdog timeout crash after ~3 minutes

### Architecture Issues  
- **Deferred LED init to 2 seconds after boot** - still crashed
- **Disabled LED controller completely** - prevented crashes but no LEDs
- **Removed startup LED animation** - didn't help
- **Skipped power sufficiency test** - didn't help
- **Power management min CPU 80MHz** - didn't prevent RMT conflicts
- **RTC GPIO hold during sleep** - LEDs still don't sleep properly

## FAILED WATCHDOG CRASH FIXES:

### Task Watchdog Attempts
- **Added watchdog reset every 30 seconds** - didn't prevent interrupt watchdog
- **Added multiple esp_task_wdt_reset() calls** - wrong watchdog type
- **Configured task watchdog timeout** - not the issue
- **Added watchdog feeds in state transitions** - wrong watchdog

### Interrupt Watchdog Attempts
- **Removed critical sections** - there weren't any to remove
- **Disabled startup animations** - didn't help
- **Added delays everywhere** - didn't help
- **Found duplicate interrupts on encoder pins** - only partial fix

### Sleep Mode Workarounds
- **Replaced light sleep with deep idle** - didn't prevent crashes
- **Used delay loops instead of sleep** - didn't help
- **Disabled RMT/LED during sleep** - crashes continued

### NEVER EVER DO AGAIN - Display Power Control
- **NEVER use power.setLDO2(false) to turn off display** - This breaks everything
- **NEVER control display power via PMU LDO2** - Causes system instability
- Attempted to fix flickering by turning off display power - COMPLETE FAILURE

## FAILED SLEEP/WAKE FIXES:

### Light Sleep Implementation Attempts
- **Proper esp_light_sleep_start() with GPIO wake** - Device enters sleep but NEVER wakes up
- **GPIO_INTR_LOW_LEVEL wake configuration** - Device becomes completely unresponsive
- **gpio_set_pull_mode with PULLUP** - No wake functionality
- **Device requires physical reset after sleep** - Complete failure

### Display Sleep Attempts
- **Set brightness to 0** - still flickers
- **Called displayOff()** - method doesn't exist
- **Tried sendCommand(0x10)** - method doesn't exist  
- **Fill screen black + brightness 0** - still flickers
- **Added delays after brightness 0** - still flickers

### Wake Configuration Attempts
- **GPIO wake with LOW_LEVEL** - doesn't wake properly
- **GPIO wake with ANYEDGE** - not supported, compiler error
- **Added pinMode INPUT_PULLUP** - didn't help
- **Configured multiple wake sources** - only encoder works
- **Light sleep mode** - causes reboot not wake
- **Deep idle polling** - blocked in previous attempts

### Touch Wake Attempts
- **Touch on GPIO 11 with LOW_LEVEL** - doesn't work
- **Touch interrupt polling** - doesn't detect
- **Different GPIO modes** - none worked
- **Wake source configuration** - ignored by system

## FAILED ARCHITECTURE FIXES:

### Duplicate Interrupt Handler
- **Disabled InputController** - fixed crashes but broke other things
- **SimpleEncoder + InputController both on pins 17/18** - caused conflicts
- **Removed one system** - partially worked but incomplete

### State Machine Issues
- **Multiple watchdog registrations** - caused conflicts
- **Inconsistent state transitions** - not properly handled
- **Memory monitoring overhead** - added complexity

## ROOT CAUSES STILL NOT ADDRESSED:

1. **ESP32-S3 sleep mode implementation** - not working as documented
2. **Display controller sleep command** - no proper method available
3. **Touch controller wake integration** - not properly configured
4. **NeoPixel/RMT conflict with ESP32 Core 3.2.0** - fundamental incompatibility
5. **Light sleep causes full reboot** - not a proper wake cycle

## What We Know:
- Interrupt watchdog fires after ~3 minutes if LEDs enabled
- Only encoder wake works (but causes reboot)
- Touch wake never works
- Display can't properly sleep (no hardware sleep command)
- LEDs conflict with system stability

## What Hasn't Been Tried Yet:
- Different ESP32 Arduino Core version (currently 3.2.0)
- Hardware PWM for display backlight control
- External FET to cut display power
- Different sleep modes (modem sleep, etc)
- Proper touch controller wake configuration via I2C
- Direct RMT control without library (tried but failed)
- Checking for ESP32-S3 errata/known issues

## ROOT CAUSE IDENTIFIED AND FIXED:
- The crash wasn't caused by LED libraries - it was the sleep transition at 3 minutes
- When transitioning to sleep, RMT operations must be completed first
- The interrupt watchdog timeout happened because sleep transition blocked ISR completion
- **SOLUTION**: Shut down LED controller BEFORE state transition to sleep
- This allows RMT to finish cleanly before sleep mode preparation begins

## POTENTIAL SOLUTIONS NOT YET TRIED:
1. **SPI with DMA method** - Use hardware SPI instead of RMT for WS2812 control
2. **I2S method** - Use I2S peripheral instead of RMT (library exists for ESP32-S3)
3. **Disable encoder interrupts** - Use polling instead of interrupts for encoder
4. **Use FreeRTOS task** - Move all LED operations to dedicated task with proper priority
5. **Downgrade to ESP32 Arduino Core 2.x** - May have better RMT implementation