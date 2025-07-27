# Timer Migration PRD - ESP32-S3-Touch-AMOLED-1.75

## Executive Summary

This document outlines the product requirements for migrating an existing timer application from ESP32 XIAO hardware with OLED display to the new ESP32-S3-Touch-AMOLED-1.75 hardware. The migration will preserve all existing functionality while leveraging the new hardware's superior display capabilities, touch interface, and power management features.

**Core Development Philosophy: Test Constantly, Build Consistently, Iterate in Small Chunks**

## Development Methodology

### Continuous Testing & Iteration

1. **Micro-Iterations**
   - Each feature implemented in small chunks
   - Compile and test after EVERY code change
   - Never accumulate untested changes
   - Roll back immediately if build fails

2. **Build-First Approach**
   ```bash
   # Standard build & test cycle (run after EVERY change)
   cd /Users/Travis/Developer/waveshare_hardware_test/TheTimerArduino && \
   arduino-cli compile --fqbn esp32:esp32:esp32s3:USBMode=hwcdc --build-property "build.psram_type=opi" && \
   arduino-cli upload -p /dev/cu.usbmodem32301 --fqbn esp32:esp32:esp32s3:USBMode=hwcdc && \
   sleep 2 && \
   esptool.py --port /dev/cu.usbmodem32301 read_mac >/dev/null 2>&1 && \
   sleep 0.5 && \
   timeout 30 cat /dev/cu.usbmodem32301
   ```

3. **Error-First Development**
   - Expect errors and plan for them
   - Add serial debugging before functionality
   - Test edge cases before happy path
   - Document every error and solution

4. **Incremental Feature Addition**
   - Start with minimal viable feature
   - Test thoroughly
   - Add one small enhancement
   - Test again
   - Repeat

## Hardware Specifications

### New Hardware: ESP32-S3-Touch-AMOLED-1.75
- **Display**: 466x466 AMOLED with 16-bit RGB565 color
- **Touch**: Capacitive touch screen (CSTXXX driver)
- **MCU**: ESP32-S3 with 8MB PSRAM
- **Interface**: QSPI (4-wire SPI) for display
- **Power**: AXP2101 PMU with battery management
- **RTC**: Built-in real-time clock controller
- **Connectivity**: WiFi, Bluetooth
- **Expansion**: TCA9554 IO expander for additional GPIO

### Peripheral Connections

#### Breakout Board Header (8-pin)
The ESP32-S3-Touch-AMOLED-1.75 provides an 8-pin header with:
- **IO18**: GPIO18
- **IO17**: GPIO17  
- **IO16**: GPIO16
- **RXD**: UART RX (can be used as GPIO)
- **TXD**: UART TX (can be used as GPIO)
- **3V3**: 3.3V power supply
- **GND**: Ground
- **VBUS**: 5V from USB

#### Rotary Encoder Connections
- **Encoder A**: IO17 (GPIO17)
- **Encoder B**: IO18 (GPIO18)
- **Encoder Common**: GND
- **Note**: The old firmware used GPIO43/8. Code will need to be updated for GPIO17/18.

#### NeoPixel LED Ring Connections
- **Data Pin**: IO16 (GPIO16)
- **Power**: 3V3 (for consistent 3.3V operation)
- **Ground**: GND
- **LED Count**: 24 LEDs (upgraded from 16)
- **Type**: NEO_GRB + NEO_KHZ800
- **Note**: Using 3.3V ensures consistent brightness whether on USB or battery power.

### Testing Points for Hardware
1. **GPIO Availability Test**: Verify each pin works before wiring
2. **Voltage Test**: Confirm 3.3V on all peripherals
3. **Current Draw Test**: Measure with all LEDs on
4. **Signal Integrity Test**: Check encoder pulses with scope

## Current Implementation Status

### Already Completed in TheTimerArduino
- ✅ Display initialized (466x466 AMOLED with Arduino_CO5300)
- ✅ Touch input working (CSTXXX driver)
- ✅ LVGL configured with proper buffers
- ✅ First UI screen complete (idle timer display)
- ✅ Custom fonts loaded (Roboto Mono 120pt, Barlow 24pt)
- ✅ Screen transitions with fade animation
- ✅ Tap navigation between screens
- ✅ Inactivity timeout (3 minutes → light sleep)
- ✅ Power button → deep sleep
- ✅ BOOT button → wake from deep sleep
- ✅ Touch wake from light sleep

### Hardware Setup
- **Physical Rotary Encoder**: Will connect to GPIO 17/18
- **NeoPixel LED Ring**: Will connect to GPIO 16
- **Touch Input**: Replaces all button functions with simple tap

## Migration Strategy

### Key Differences from Original Plan
1. **Physical Encoder Retained**: No virtual dial needed - using real rotary encoder
2. **Simplified Touch**: Primarily single tap, with long-press only in timer states to end timer
3. **Starting Point**: Display, touch, and sleep already working in TheTimerArduino
4. **UI Foundation**: First screen (idle timer) already implemented with custom fonts

### Input Mapping
- **Screen Tap** = Old button click (context-sensitive per state)
- **Long Press** = End timer early (only in TimerState and PausedState)
- **Encoder Rotation** = Value adjustment (unchanged)
- **Power Button** = Deep sleep (already implemented)
- **Touch Wake** = Wake from light sleep (already implemented)

### State Machine Navigation Flow

**Initial Boot**:
1. **StartupState** (2 seconds splash) → 
2. **ProvisionState** (if no WiFi configured) → Enter WiFi credentials
3. Skip to **IdleState** if WiFi already configured

**From IdleState** (main screen showing 25:00):
- **Rotate Encoder** → **AdjustState** (timer duration setting)
- **Tap Screen** → **ProjectSelectState**

**From AdjustState** (timer duration setting):
- **Rotate Encoder** → Adjust timer duration (5-240 minutes)
- **Tap Screen** → Save duration and return to **IdleState**

**From ProjectSelectState**:
- **Rotate Encoder** → Scroll through projects
- **Tap Screen** → Start timer → **TimerState**

**From TimerState** (timer running):
- **Tap Screen** → **PausedState**
- **Long Press** → End timer → **DoneState**

**From PausedState**:
- **Tap Screen** → Resume → **TimerState**
- **Long Press** → End timer → **DoneState**

**From DoneState**:
- **Tap Screen** → Return to **IdleState**

## Core Features to Migrate

### Implementation Order (Test-Driven)

#### Phase 1: State Machine Integration
**Port State Machine Framework**
- [ ] Copy state machine files from old firmware
- [ ] Update CMakeLists.txt / includes
- [ ] Create empty state implementations
- [ ] Test state transitions work
- [ ] Connect screen tap to state changes

**Integrate with Existing UI**
- [ ] Connect idle screen to IdleState
- [ ] Update timer display from state
- [ ] Test tap starts/stops timer
- [ ] Verify state persistence

#### Phase 2: Hardware Peripherals
**Physical Encoder Integration**
- [ ] Wire encoder to GPIO 17/18
- [ ] Test encoder rotation detection
- [ ] Port InputController with new pins
- [ ] Test value adjustment in AdjustState
- [ ] Verify interrupt handling works

**NeoPixel LED Ring**
- [ ] Wire LEDs to GPIO 16 with 3.3V power
- [ ] Test single LED at 3.3V
- [ ] Port LEDController to new pin
- [ ] Test all animation patterns
- [ ] Verify brightness levels adequate

#### Phase 3: State Implementation
**Core States**
- [ ] IdleState (with all tests)
- [ ] AdjustState (with all tests)
- [ ] TimerState (with all tests)
- [ ] ProjectSelectState (with all tests)
- [ ] PausedState & DoneState
- [ ] SleepState & power management
- [ ] Integration testing

#### Phase 4: Network Features
**WiFi Provisioning**
- [ ] Test 1: AP mode starts
- [ ] Test 2: Can connect to AP
- [ ] Test 3: Provisioning saves credentials
- [ ] Feature: WiFi provisioning works

**Web Server**
- [ ] Test 1: Server starts
- [ ] Test 2: Index page loads
- [ ] Test 3: API responds
- [ ] Feature: Basic web interface

**Project Management**
- [ ] Test 1: Create project via API
- [ ] Test 2: List projects
- [ ] Test 3: Delete project
- [ ] Test 4: Projects persist
- [ ] Feature: Full project CRUD

### Testing Infrastructure

#### Serial Debug Framework
```cpp
// Add to every state enter()
USBSerial.println("=== ENTERING STATE: StateName ===");
USBSerial.print("Free heap: ");
USBSerial.println(ESP.getFreeHeap());
USBSerial.print("Timestamp: ");
USBSerial.println(millis());

// Add to every state exit()
USBSerial.println("=== EXITING STATE: StateName ===");
```

#### Memory Monitoring
```cpp
// Run every 10 seconds in loop()
static unsigned long lastMemCheck = 0;
if (millis() - lastMemCheck > 10000) {
    USBSerial.print("HEAP: ");
    USBSerial.print(ESP.getFreeHeap());
    USBSerial.print(" PSRAM: ");
    USBSerial.println(ESP.getFreePsram());
    lastMemCheck = millis();
}
```

#### Crash Recovery
```cpp
// Add to setup()
esp_reset_reason_t reset_reason = esp_reset_reason();
USBSerial.print("Reset reason: ");
USBSerial.println(reset_reason);
if (reset_reason == ESP_RST_PANIC) {
    USBSerial.println("RECOVERED FROM CRASH!");
    // Log crash context if available
}
```

### Error Handling Strategy

1. **Never Silent Fail**
   - Every function returns success/failure
   - Every failure logs detailed error
   - Every error has recovery path

2. **Defensive Programming**
   ```cpp
   // Example pattern for all functions
   bool initializeComponent() {
       USBSerial.println("Initializing component X...");
       
       if (!component.begin()) {
           USBSerial.println("ERROR: Component X failed to initialize");
           USBSerial.println("Attempting recovery...");
           delay(100);
           if (!component.begin()) {
               USBSerial.println("FATAL: Component X unrecoverable");
               return false;
           }
       }
       
       USBSerial.println("Component X initialized successfully");
       return true;
   }
   ```

3. **Test Harness Mode**
   ```cpp
   // Add compile flag for test mode
   #ifdef TEST_MODE
   void runTestSuite() {
       testDisplay();
       testTouch();
       testEncoder();
       testLEDs();
       testStates();
       testMemory();
       reportResults();
   }
   #endif
   ```

## UI/UX Requirements

### Iterative UI Development

1. **Phase 1: Boxes and Text**
   - Just get text on screen
   - Verify positioning
   - Test touch zones

2. **Phase 2: Fonts and Sizes**
   - Add custom fonts one at a time
   - Test memory impact
   - Verify readability

3. **Phase 3: Colors and Polish**
   - Add colors incrementally
   - Test brightness levels
   - Verify power consumption

4. **Phase 4: Animations**
   - Start with simple fades
   - Test performance impact
   - Add complexity gradually

### Screen Layouts (Based on TheTimerUI.png)

#### Testing Each Screen
1. **Create static mockup first**
2. **Test touch zones with colored boxes**
3. **Add real content**
4. **Test state persistence**
5. **Test transitions in/out**

## Technical Requirements

### Build Configuration
```bash
# Always use these exact flags
--fqbn esp32:esp32:esp32s3:USBMode=hwcdc
--build-property "build.psram_type=opi"
```

### Critical Testing Points

1. **Display Buffer Allocation**
   - Test with small buffer first
   - Increase size gradually
   - Monitor free memory
   - Find optimal size

2. **Touch Calibration**
   - Log raw coordinates
   - Test all four corners
   - Verify center point
   - Check edge detection

3. **Power Management**
   - Test wake sources individually
   - Measure current in each state
   - Verify sleep entry/exit
   - Test battery life

## Continuous Integration

### Testing Checklist
- [ ] Each feature: Build and upload
- [ ] After implementation: Test all states
- [ ] Regular intervals: Memory leak check
- [ ] Each error: Document and fix immediately

### Error Log Template
```
Error: [Description]
Context: [What was being tested]
Serial Output: [Paste output]
Solution: [How it was fixed]
Prevention: [How to avoid in future]
```

## Acceptance Criteria

### Incremental Acceptance
Each feature must pass these gates:

1. **Gate 1: Builds without warnings**
2. **Gate 2: Uploads successfully**
3. **Gate 3: Runs for 1 minute without crash**
4. **Gate 4: Survives 10 state transitions**
5. **Gate 5: Works after power cycle**
6. **Gate 6: Memory usage stable**

### Feature-Specific Tests
Document specific test cases for each feature BEFORE implementing.

## Risk Mitigation

### Common Pitfalls & Solutions

1. **Memory Fragmentation**
   - Test: Long-running state transitions
   - Solution: Pre-allocate all buffers
   - Prevention: No dynamic allocation in loop()

2. **I2C Conflicts**
   - Test: Access all I2C devices sequentially
   - Solution: Add mutexes if needed
   - Prevention: Single I2C manager class

3. **Display Corruption**
   - Test: Rapid screen switches
   - Solution: Proper LVGL mutex usage
   - Prevention: Single display update point

4. **Touch Glitches**
   - Test: Rapid touch events
   - Solution: Debouncing and rate limiting
   - Prevention: State-based touch handling

## Development Tools

### Essential Debugging Commands
```bash
# Monitor with timestamp
arduino-cli monitor -p /dev/cu.usbmodem32301 --timestamp

# Quick reset and monitor
esptool.py --port /dev/cu.usbmodem32301 read_mac && sleep 0.5 && cat /dev/cu.usbmodem32301

# Flash and monitor in one command
./build_and_monitor.sh
```

### Performance Monitoring
```cpp
// Add to critical sections
unsigned long start = micros();
// ... code to measure ...
unsigned long duration = micros() - start;
if (duration > 1000) { // Log if > 1ms
    USBSerial.print("PERF WARNING: ");
    USBSerial.print(functionName);
    USBSerial.print(" took ");
    USBSerial.print(duration);
    USBSerial.println("us");
}
```

## Migration Challenges & Solutions

### 1. Physical Encoder Pin Changes
**Challenge**: Old pins GPIO 43/8 don't exist on new hardware
**Solution**: Remap to GPIO 17/18 on breakout header
```cpp
#define ENCODER_A_PIN 17  // Was 43
#define ENCODER_B_PIN 18  // Was 8
```

### 2. Long Press Detection
**Challenge**: Need long press for ending timer early
**Solution**: Add LVGL long press events
```cpp
lv_obj_add_event_cb(screen, long_press_cb, LV_EVENT_LONG_PRESSED, NULL);
lv_obj_add_event_cb(screen, long_press_repeat_cb, LV_EVENT_LONG_PRESSED_REPEAT, NULL);
```

### 3. NeoPixel 3.3V Operation
**Challenge**: LEDs designed for 5V, using 3.3V supply
**Solution**: 
- Test signal integrity first
- Reduce brightness if needed
- Most WS2812B work fine at 3.3V

### 4. Display Memory Requirements
**Challenge**: 466x466 display needs large buffers
**Solution**: Already using PSRAM in TheTimerArduino
```cpp
buf1 = (lv_color_t*)heap_caps_malloc(buffer_size * sizeof(lv_color_t), 
                                      MALLOC_CAP_DMA | MALLOC_CAP_SPIRAM);
```

### 5. Touch Wake Configuration
**Challenge**: Different wake source than physical button
**Solution**: Already implemented using GPIO11
```cpp
gpio_wakeup_enable(GPIO_NUM_11, GPIO_INTR_LOW_LEVEL);
esp_sleep_enable_gpio_wakeup();
```

### 6. State Machine Integration
**Challenge**: Connect existing UI to state logic
**Solution**: 
- Port state files from old firmware
- Map touch/encoder events to state handlers
- Preserve existing screen transition code

### 7. WiFi Provisioning Check
**Challenge**: Skip provisioning if already configured
**Solution**: Check NVS on boot
```cpp
if (!networkController.isWiFiProvisioned()) {
    stateMachine.changeState(&provisionState);
} else {
    stateMachine.changeState(&idleState);
}
```

### 8. Project Color Preview
**Challenge**: Show colors on both display and LEDs
**Solution**: Update display preview while keeping LED feedback

## Testing Strategy

### Hardware Test Order
1. Encoder on new pins (GPIO 17/18)
2. NeoPixels at 3.3V (GPIO 16)
3. Combined encoder + LED operation
4. State transitions with real hardware

### Debug Infrastructure
```cpp
#define DEBUG_MIGRATION 1
#if DEBUG_MIGRATION
  #define MIG_LOG(x) USBSerial.println(x)
#else
  #define MIG_LOG(x)
#endif
```

## Success Metrics

1. **Build Success Rate**: >95% (track daily)
2. **Crash-Free Runtime**: >4 hours continuous
3. **Memory Stability**: <5% variation over 1 hour
4. **Response Time**: <100ms for all interactions
5. **Code Coverage**: Test every state transition

## Appendix: Pin Assignments Summary

| Function | GPIO Pin | Header Pin | Notes |
|----------|----------|------------|-------|
| LED Data | GPIO16 | IO16 | NeoPixel data line |
| Encoder A | GPIO17 | IO17 | Rotary encoder channel A |
| Encoder B | GPIO18 | IO18 | Rotary encoder channel B |
| Power Button | PMU IRQ | Internal | Via AXP2101 |
| Wake Button | GPIO0 | Internal | BOOT button |
| Touch INT | GPIO11 | Internal | Touch interrupt |
| Display CS | GPIO12 | Internal | QSPI chip select |

### Wiring Diagram
```
ESP32-S3-Touch-AMOLED Breakout Header:
┌─────────────────────┐
│ VBUS  ○             │ 5V USB Power
│ GND   ○─────────┐   │ Common Ground
│ 3V3   ○───┐     │   │ 3.3V Power
│ TXD   ○   │     │   │ (Not used)
│ RXD   ○   │     │   │ (Not used)
│ IO16  ○───┼─────┼───│ → NeoPixel Data
│ IO17  ○───┼─────┼───│ → Encoder A
│ IO18  ○───┼─────┼───│ → Encoder B
└───────────┼─────┼───┘
            │     │
    ┌───────┴─────┴────┐
    │   NeoPixel Ring  │
    │   VCC ← 3V3      │
    │   GND ← GND      │
    │   DIN ← IO16     │
    └──────────────────┘
    
    ┌──────────────────┐
    │  Rotary Encoder  │
    │   A   ← IO17     │
    │   B   ← IO18     │
    │   COM ← GND      │
    └──────────────────┘
```

---

*Remember: Small changes, constant testing, immediate fixes. Never let errors accumulate.*