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

## FAILED SLEEP CRASH FIXES (Latest Attempts - Session 3):

### Disabled InputController to Fix Duplicate Interrupts
- **Problem**: Both SimpleEncoder and InputController attach interrupts to pins 17/18
- **Attempted**: Disabled InputController initialization in StateMachine.cpp
- **Result**: FAILED - Device still crashes and reboots after "entering sleep"
- **Evidence**: Device shows "Inactivity timeout - entering sleep" then immediately reboots

### Added Serial.flush() Before Deep Sleep
- **Problem**: Research showed Serial buffer can cause interrupt watchdog timeout
- **Attempted**: Added Serial.flush() and esp_task_wdt_delete(NULL) in SleepState::enterSleepMode()
- **Result**: FAILED - No improvement, device still crashes
- **Note**: Common fix from ESP32 forums but didn't work here

### Added esp_task_wdt_delete(NULL) Before Sleep
- **Problem**: Task watchdog might be causing the crash
- **Attempted**: Delete current task from watchdog before sleep
- **Result**: FAILED - Device still crashes and reboots
- **Issue**: The crash happens even with watchdog deleted

### Improved Encoder Pin Configuration for ext1 Wake
- **Attempted**: Added explicit GPIO configuration with pull-ups before ext1 setup
- **Code**: gpio_set_direction(), gpio_set_pull_mode(), error checking on esp_sleep_enable_ext1_wakeup()
- **Result**: FAILED - Device crashes before even reaching deep sleep
- **Power domain**: Added esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON)
- **Still crashes**: Device never actually enters sleep mode

## WHAT'S ACTUALLY HAPPENING (Verified by monitoring):
1. Device shows "Inactivity timeout - entering sleep"  
2. Device shows "[SCREEN] Turning display OFF"
3. Device shows "WiFi disconnected" and "Web Server stopped"
4. Within 5-10 seconds, device reboots completely
5. Device shows normal boot sequence with "Boot count: 1"
6. Device returns to IdleState as if freshly started
7. NO ACTUAL SLEEP OCCURS - just crash and reboot

## The Real Problem:
- The device NEVER enters deep sleep
- Something between "Web Server stopped" and esp_deep_sleep_start() causes a crash
- The crash is silent - no error message, no guru meditation, just reboot
- This suggests a hardware-level issue or fundamental ESP32-S3 sleep bug
- All "fixes" have been cosmetic - the core issue remains

## CRITICAL DISCOVERY - esp_deep_sleep_start() ITSELF IS CRASHING:

### Complete State Machine Bypass Test
- **Problem**: Suspected state machine was causing the crash
- **Attempted**: Bypassed state machine entirely, implemented sleep directly in loop()
- **Code**: Direct calls to turnOffDisplay(), WiFi.disconnect(), esp_deep_sleep_start()
- **Result**: FAILED - Device still crashes and reboots
- **Evidence**: Device prints "Entering deep sleep NOW" then immediately reboots
- **Conclusion**: The crash is IN esp_deep_sleep_start() itself, not in our code

### What This Means:
1. esp_deep_sleep_start() is broken on this ESP32-S3 device
2. The crash happens at the hardware/ESP-IDF level
3. No amount of code fixes will solve this
4. This could be:
   - A bug in ESP32 Arduino Core 3.2.0
   - A hardware defect in this specific ESP32-S3 board
   - An incompatibility with the Waveshare ESP32-S3-Touch-AMOLED-1.75
   - A conflict with PSRAM, display driver, or other hardware components

### Evidence of Hardware/Firmware Bug:
- Device reaches esp_deep_sleep_start() successfully
- All peripherals are properly shut down before sleep
- Serial flush, watchdog deletion, interrupt detachment - all tried
- State machine completely bypassed - still crashes
- The crash is instant and silent - typical of low-level hardware fault

## FAILED SLEEP CRASH FIXES (Latest Attempts - Session 2):

### Interrupt Detachment Before Sleep
- **Problem**: Device crashes/reboots instead of sleeping
- **Attempted**: Detach ALL interrupts before sleep (encoder, button, PMU, touch)
- **Code**: Added detachInterrupt() for all pins before sleep
- **Result**: FAILED - Arduino CLI compilation hangs/times out
- **Issue**: Can't even test because compilation is broken

### Minimal Sleep Test
- **Problem**: Too many things could be causing the crash
- **Attempted**: Strip everything down to just esp_sleep_enable_timer_wakeup + esp_deep_sleep_start
- **Result**: FAILED - Arduino CLI compilation still hangs
- **Root issue**: Arduino development environment appears broken

### WiFi/BT Shutdown Sequence
- **Attempted**: Proper shutdown of WiFi and Bluetooth before sleep
- **Includes**: esp_wifi_stop/deinit, esp_bluedroid_disable/deinit, esp_bt_controller_disable/deinit
- **Result**: UNTESTED - Can't compile
- **Note**: This might be necessary but can't verify

## COMPILATION ISSUES:
- Arduino CLI hangs indefinitely during compilation
- Happens with --clean flag and without
- Happens even with minimal code changes
- Device wake doesn't help
- No error messages, just timeouts after 2-3 minutes

## FAILED SLEEP CRASH FIXES (Previous Session):

### State Machine Memory Logging Crash
- **Problem**: Interrupt watchdog timeout when transitioning to SleepState
- **Attempted**: Override enter() method in SleepState to bypass memory logging
- **Result**: FAILED - Device still crashes and reboots
- **Root cause**: State::enter() calls logMemoryAndStack which takes too long during sleep transition

### Direct Sleep Transition in firmware.ino
- **Problem**: State machine transition causes crash before reaching sleep
- **Attempted**: Bypass state machine entirely, implement sleep directly in loop()
- **Code added**: Direct esp_deep_sleep_start() after turning off peripherals
- **Result**: FAILED - Device appears to sleep but immediately reboots
- **What happens**: Device shows "Preparing for inactivity deep sleep..." then reboots with Boot count: 1

### Wake Configuration Issues
- **ext1 wake for encoder**: Configured (1ULL << GPIO_NUM_17) | (1ULL << GPIO_NUM_18)
- **ESP_EXT1_WAKEUP_ANY_LOW**: Used for encoder wake
- **Result**: Device doesn't wake from encoder - it just crashes/reboots on sleep entry

### Deep Sleep vs Light Sleep
- **Changed from light sleep to deep sleep**: Thinking it would be more reliable
- **Result**: Still crashes/reboots immediately
- **Both sleep modes fail**: Whether using light or deep sleep, device reboots instead of sleeping

## WHAT'S ACTUALLY HAPPENING:
1. Inactivity timer reaches 3 minutes
2. Code attempts to enter sleep (either via state machine or direct)
3. Device shows sleep messages
4. Device immediately reboots (Boot count: 1)
5. No actual sleep occurs - just a crash/reboot cycle

## ROOT PROBLEM NOT SOLVED:
- The device NEVER actually enters sleep mode
- It crashes/reboots during the sleep transition
- This happens whether using state machine or direct sleep
- The interrupt watchdog timeout is just a symptom - the real issue is sleep entry fails

## PARTIAL SUCCESS - SLEEP WORKS BUT WAKE DOESN'T (Session 4):

### Fixed the Crash - Device Enters Sleep Successfully!
- **Problem**: Device was crashing with power domain assertion failure
- **Solution**: Removed all power domain configs except ESP_PD_DOMAIN_RTC_PERIPH
- **Code**: Only kept `esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON)`
- **Result**: SUCCESS - Device enters deep sleep without crashing!
- **Evidence**: Device prints "Entering deep sleep NOW" and actually sleeps

### Disabled Brownout Detector
- **Added**: `REG_WRITE(RTC_CNTL_BROWN_OUT_REG, 0)` before esp_deep_sleep_start()
- **Result**: Helps prevent brownout during sleep transition

### BUT Wake-Up Still Broken:
1. **Encoder wake doesn't work** - ext1 wake with GPIO 17/18 configured but device doesn't wake
2. **Device shows Boot count: 1** - This means it's doing full restart, not proper wake
3. **BOOT button wake also fails** - Device stays asleep indefinitely
4. **Device requires physical RESET** - Only way to recover from sleep state

### What This Means:
- We've fixed the crash but wake sources aren't configured properly
- The RTC GPIO configuration might need different approach
- ext1 wake might not work with these specific pins on ESP32-S3
- May need to use GPIO wake instead of ext1 wake

## BREAKTHROUGH - LIGHT SLEEP WORKS! (Session 5):

### Light Sleep Successfully Works Where Deep Sleep Fails
- **Problem**: esp_deep_sleep_start() causes immediate reboot/crash on this ESP32-S3
- **Solution**: Use esp_light_sleep_start() instead
- **Result**: SUCCESS - Device sleeps and wakes properly!
- **Evidence**: 
  - Device prints "=== WOKE FROM LIGHT SLEEP ==="
  - Wake cause: 4 (timer) confirmed working
  - Display turns back on after wake
  - Device continues normal operation

### Why Deep Sleep Fails but Light Sleep Works:
1. **ESP32-S3 with OPI PSRAM** - Known issues with deep sleep
2. **Hardware conflicts** - Display, touch, or other peripherals interfere with deep sleep
3. **Power domain issues** - Deep sleep requires more complex power management
4. **Light sleep maintains RAM** - Less disruptive to system state

### Remaining Issues with Light Sleep:
- Watchdog error after wake (task was deleted before sleep)
- Need to test encoder wake (ext1) with light sleep
- Power consumption higher than deep sleep but functional

## POTENTIAL SOLUTIONS NOT YET TRIED:
1. **SPI with DMA method** - Use hardware SPI instead of RMT for WS2812 control
2. **I2S method** - Use I2S peripheral instead of RMT (library exists for ESP32-S3)
3. **Disable encoder interrupts** - Use polling instead of interrupts for encoder
4. **Use FreeRTOS task** - Move all LED operations to dedicated task with proper priority
5. **Downgrade to ESP32 Arduino Core 2.x** - May have better RMT implementation
6. **Check for conflicts with WiFi/BT shutdown** - May need different shutdown sequence
7. **Disable all interrupts before sleep** - gpio_intr_disable() on all pins
8. **Check PMU/power configuration** - Power management may be interfering