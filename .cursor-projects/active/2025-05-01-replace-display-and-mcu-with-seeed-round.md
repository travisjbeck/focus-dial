# Project: Replace Original MCU & OLED with Seeed Studio XIAO ESP32S3 & Round Touch Display
- **Created**: 2025-05-01
- **Status**: Active
- **Last Updated**: 2025-05-14

## Context & Requirements
This project focuses on upgrading the existing system by replacing its original microcontroller and square monochrome OLED display. The new hardware suite will consist of a Seeed Studio XIAO ESP32S3 Plus microcontroller and a Seeed Studio 1.28" Round Color Touch Display. This requires hardware wiring changes and significant firmware updates. The existing rotary encoder will be replaced with a smaller Bourns PER35 35mm rotary encoder that has the same connection points and functionality. The separate physical button will be replaced by the touch screen interaction. The project will now use a 16 LED NeoPixel for visual indicators. The target board, the Seeed Studio XIAO ESP32S3 Plus, features a dual-core ESP32-S3 processor, 20 GPIOs, WiFi, BLE 5.0, 8MB PSRAM, and 16MB Flash.

**References:** 

- [Seeed Studio Round Display Documentation](https://wiki.seeedstudio.com/get_start_round_display/)
- [Seeed Studio XIAO ESP32S3 Plus](https://www.seeedstudio.com/Seeed-Studio-XIAO-ESP32S3-Plus-p-6361.html)

## Seeed Studio XIAO ESP32S3 Plus Pinout

### Main Board Pins (Top View)

| GPIO Pin | Analog Pin | Digital Pin | Special Function | Notes |
|----------|------------|-------------|------------------|-------|
| GPIO1    | A0         | D0          | -                | Left side |
| GPIO2    | A1         | D1          | -                | Left side |
| GPIO3    | A2         | D2          | -                | Left side |
| GPIO4    | A3         | D3          | -                | Left side |
| GPIO5    | A4         | D4          | SDA              | Left side, I2C Data |
| GPIO6    | A5         | D5          | SCL              | Left side, I2C Clock |
| GPIO43   | -          | D6          | TX               | Left side, UART TX |
| -        | -          | -           | 5V               | Right side, Power |
| -        | -          | -           | GND              | Right side, Ground |
| -        | -          | -           | 3V3              | Right side, 3.3V |
| GPIO9    | A10        | D10         | MOSI             | Right side, SPI MOSI |
| GPIO8    | A9         | D9          | MISO             | Right side, SPI MISO |
| GPIO7    | A8         | D8          | SCK              | Right side, SPI Clock |
| GPIO44   | -          | D7          | RX               | Right side, UART RX |

### Additional Pins (Bottom View)

| GPIO Pin  | Digital Pin | Special Function | Notes               |
|-----------|-------------|------------------|---------------------|
| GPIO38    | D11         | I2S_SD           | Bottom right        |
| GPIO39    | D12         | I2S_SCK          | Bottom right, MTCK  |
| GPIO40    | D13         | I2S_WS           | Bottom right, MTDO  |
| GPIO41    | D14         | RX1              | Bottom right, MTDI  |
| GPIO42    | D15         | TX1              | Bottom right, MTMS  |
| GPIO10    | D16         | ADC              | Bottom right        |
| GPIO13    | D17         | SCK_1            | Bottom left         |
| GPIO12    | D18         | MISO_1           | Bottom left         |
| GPIO11    | D19         | MOSI_1           | Bottom left         |
| -         | -           | BAT              | Bottom, Battery connection |
| -         | -           | USB              | Top, USB connection |

### Pin Function Groups
- **Analog**: A0–A5, A8–A10 (A6–A7 are not broken out)
- **Digital**: D0-D19
- **I2C**: SDA (GPIO5), SCL (GPIO6)
- **SPI (Primary)**: MOSI (GPIO9), MISO (GPIO8), SCK (GPIO7)
- **SPI (Secondary)**: MOSI_1 (GPIO11), MISO_1 (GPIO12), SCK_1 (GPIO13)
- **UART (Primary)**: TX (GPIO43), RX (GPIO44)
- **UART (Secondary)**: TX1 (GPIO42), RX1 (GPIO41)
- **I2S**: I2S_SD (GPIO38), I2S_SCK (GPIO39), I2S_WS (GPIO40)
- **Power**: 5V, 3V3, GND, BAT

## Wiring Diagram (Breadboard Setup)

The following table and Mermaid diagram show a complete breadboard wiring reference for connecting the Seeed Studio XIAO ESP32S3 Plus to the 1.28″ Round Touch Display, 16-LED NeoPixel ring, and the Bourns PER35 35mm rotary encoder.

### Color-coded Wire Legend
| Color | Signal | Notes |
|-------|--------|-------|
| **Red** | 3V3 (power) | 3.3 V regulated output on XIAO, powers display |
| **Orange** | 5V (power) | Supplies NeoPixel ring (higher brightness) |
| **Black** | GND | Common ground for all modules |
| **Yellow** | SCK (SPI CLK) | Display clock (D8 / GPIO7) |
| **Green** | MOSI (SPI DATA) | Display data (D10 / GPIO9) |
| **Blue** | CS (Chip-Select) | Display CS (D3 / GPIO4) |
| **Teal** | DC (Data/Command) | Display DC (D2 / GPIO3) |
| **Gray** | RST (Reset) | Display RST (D1 / GPIO2) |
| **Brown** | SDA (I²C Data) | Touch-panel & RTC data (D4 / GPIO5) |
| **White** | SCL (I²C Clock) | Touch-panel & RTC clock (D5 / GPIO6) |
| **Lime** | NeoPixel DIN | Data-in for 16-LED ring (D0 / GPIO1) |
| **Purple** | ENC_A | Encoder channel A (D6 / GPIO43) |
| **Pink** | ENC_B | Encoder channel B (D9 / GPIO8) |
| **Orange** | TOUCH_INT | Touch Interrupt (Internal, needs D7 / GPIO44) |

### Connection Table
| Module | Pin on Module | Wire Color | XIAO Pin | Notes |
|--------|---------------|-----------|----------|-------|
| **Round Display** | 3V3 | Red | 3V3 | |
| | GND | Black | GND | |
| | D8 | Yellow | D8 (GPIO7) | SPI SCK |
| | D10 | Green | D10 (GPIO9) | SPI MOSI |
| | D3 | Blue | D3 (GPIO4) | SPI CS |
| | D2 | Teal | D2 (GPIO3) | SPI DC |
| | D1 | Gray | D1 (GPIO2) | SPI RST |
| | D9 | – | (Not connected) | |
| | D4 | Brown | D4 (GPIO5) | I2C SDA (Touch/RTC) |
| | D5 | White | D5 (GPIO6) | I2C SCL (Touch/RTC) |
| | (Internal) | Orange | D7 (GPIO44) | Touch INT Required |
| **NeoPixel Ring** | 5V | Orange | 5V | |
| | GND | Black | GND | |
| | DIN | Lime | D0 (GPIO1) |
| **Bourns Encoder** | GND | Black | GND | |
| | Channel A | Purple | D6 (GPIO43) |
| | Channel B | Pink | D9 (GPIO8) |

> **Tip:** Keep wire lengths short and route the Red/Orange power traces next to a ground line (Black) to minimize noise.
> **Important Encoder Note:** Verify the pinout (GND, Channel A, Channel B) of your specific Bourns PER35 encoder using its datasheet and physical orientation before wiring. The center pin is often GND, with A and B as the outer pins, but this MUST be confirmed. Swapping A and B will reverse rotation direction.

### Mermaid Breadboard Diagram
```mermaid
flowchart LR
    subgraph MCU[Seeed XIAO ESP32S3 Plus]
        XIAO3V3[(3V3)]
        XIAO5V[(5V)]
        XIAOGND((GND))
        XIAO_SCK[D8 / GPIO7\nSCK]
        XIAO_MOSI[D10 / GPIO9\nMOSI]
        XIAO_CS[D3 / GPIO4\nCS]
        XIAO_DC[D2 / GPIO3\nDC]
        XIAO_RST[D1 / GPIO2\nRST]
        XIAO_SDA[D4 / GPIO5\nSDA]
        XIAO_SCL[D5 / GPIO6\nSCL]
        XIAO_NEO[D0 / GPIO1\nNeo DIN]
        XIAO_ENCA[D6 / GPIO43\nENC_A]
        XIAO_ENCB[D9 / GPIO8\nENC_B]
        XIAO_TOUCHINT[D7 / GPIO44\nTOUCH_INT]
    end

    subgraph DISP[1.28" Round Touch Display]
        DISP_VCC[VCC]
        DISP_GND((GND))
        DISP_SCK[SCK]
        DISP_MOSI[MOSI]
        DISP_CS[CS]
        DISP_DC[DC]
        DISP_RST[RST]
        DISP_SDA[SDA]
        DISP_SCL[SCL]
    end

    subgraph NEO[NeoPixel Ring 16 LED]
        NEO_VCC[5V]
        NEO_GND((GND))
        NEO_DIN[DIN]
    end

    subgraph ENC[Bourns Encoder]
        ENC_GND((GND))
        ENC_A[Channel A]
        ENC_B[Channel B]
    end

    %% Power
    XIAO3V3 -- Red --> DISP_VCC
    XIAO5V -- Orange --> NEO_VCC
    XIAOGND -- Black --> DISP_GND
    XIAOGND -- Black --> NEO_GND
    XIAOGND -- Black --> ENC_GND

    %% SPI (Display)
    XIAO_SCK -- Yellow --> DISP_SCK
    XIAO_MOSI -- Green --> DISP_MOSI
    XIAO_CS -- Blue --> DISP_CS
    XIAO_DC -- Teal --> DISP_DC
    XIAO_RST -- Gray --> DISP_RST

    %% I2C (Touch)
    XIAO_SDA -- Brown --> DISP_SDA
    XIAO_SCL -- White --> DISP_SCL

    %% NeoPixel
    XIAO_NEO -- Lime --> NEO_DIN

    %% Encoder
    XIAO_ENCA -- Purple --> ENC_A
    XIAO_ENCB -- Pink --> ENC_B

    %% Touch Interrupt Note (Implicit Connection)
    XIAO_TOUCHINT -. Orange .-> DISP((Internal))

    %% Legend styles
    classDef power stroke-width:2px,stroke:red;
    classDef gnd stroke-width:2px,stroke:black;
```

> The diagram uses distinct colors for each signal type (see legend above). Most Markdown viewers that support Mermaid will render the colored connections automatically; if your viewer does not show colors, refer to the table and wire-color legend.

With this reference you can wire the modules on any standard breadboard without additional passive components.

## Development Plan for Main Firmware Upgrade

**Current Status & Context for New Agent (as of 2024-07-29):**
*   The primary goal is to upgrade the existing firmware (previously for an Adafruit MCU with an OLED) to work with a Seeed Studio XIAO ESP32S3 Plus, a Seeed 1.28" Round Touch Display, and updated pin configurations for peripherals (Encoder, NeoPixel).
*   Non-Volatile Storage (NVS) in the main firmware has been stabilized by implementing a robust `nvs_flash_init()` in `main.cpp` and ensuring correct `Preferences` library usage. The `platformio.ini` has been updated to target `main.cpp` and its dependencies (`build_src_filter = +<*> -<MinimalDisplayTest.cpp>`).
*   A `MinimalDisplayTest.cpp` sketch exists which successfully demonstrates basic operation of the new display, touch, LVGL, encoder (interrupt-driven), and NeoPixel ring with the XIAO ESP32S3. This sketch is a crucial reference for driver initialization and interaction patterns.
*   The main challenge now is integrating these new hardware drivers and LVGL-based UI into the existing state machine and controller architecture of the main firmware.

**Overall Strategy for Upgrade:**
1.  **Core Hardware First:** Get the fundamental new hardware (ESP32S3, display, touch, encoder, NeoPixel on new pins) working with basic drivers and LVGL *within the existing firmware structure*, initially bypassing or stubbing out complex application logic.
2.  **Controller by Controller:** Update individual controllers to work with the new hardware pins and drivers.
3.  **State by State UI Migration:** Rewrite the UI for each state using LVGL, replacing the old display methods. Integrate touch interactions.
4.  **Iterative Testing:** Compile and test (at least for basic functionality/no crashes) after each significant small step. Commit working changes frequently.

---

**Proposed Step-by-Step Plan:**

**Phase 0: Preparation & Configuration (Essential Groundwork)**
*   **Goal:** Align main firmware configuration with new hardware and ensure necessary libraries are linked.
1.  **[ ] Update `firmware/include/Config.h`:**
    *   **Action:**
        *   Change `ENCODER_A_PIN` to `43` (XIAO D6).
        *   Change `ENCODER_B_PIN` to `8` (XIAO D9).
        *   Change `LED_PIN` to `1` (XIAO D0).
        *   Comment out or remove `BUTTON_PIN` (e.g., `#define BUTTON_PIN 26 // Deprecated, replaced by touch`).
        *   Remove `OLED_WIDTH`, `OLED_HEIGHT`, `OLED_ADDR` (specific to old display).
    *   **Rationale:** Match pin definitions to the new hardware setup verified in `MinimalDisplayTest.cpp`.
2.  **[ ] Update `platformio.ini` (if necessary):**
    *   **Action:**
        *   Verify `lib_deps` includes:
            *   `lvgl/lvgl`
            *   `Seeed-Studio/Seeed_Arduino_RoundDisplay` (or the specific Seeed LVGL driver for `lv_xiao_round_screen.h`)
            *   (Adafruit NeoPixel, RotaryEncoder should be present and compatible).
        *   Confirm `build_src_filter = +<*> -<MinimalDisplayTest.cpp>` is set.
    *   **Rationale:** Ensure all libraries for the new hardware (display, LVGL) are available to the main firmware.

**Phase 1: Core System & LVGL Initialization in `main.cpp`**
*   **Goal:** Initialize the new display, touch, and LVGL within `main.cpp`, achieving a basic visual output.
1.  **[ ] Modify `firmware/src/main.cpp` - Basic LVGL Setup:**
    *   **Action:**
        *   Include LVGL and the XIAO round screen helper: `lvgl.h`, `lv_xiao_round_screen.h`.
        *   In `setup()`, after NVS init, add LVGL core init, display init (`lv_xiao_disp_init()`), touch init (`lv_xiao_touch_init()`), and LVGL tick setup. Refer to `MinimalDisplayTest.cpp` for exact calls.
        *   Log success: `Serial.println("LVGL, Display, and Touch initialized.");`
        *   Create a temporary LVGL label (e.g., "Main FW LVGL OK") and display it.
        *   Temporarily comment out initializations for `projectManager`, `inputController`, `displayController`, `ledController`, `networkController`, and the initial `stateMachine.changeState()`.
        *   In `loop()`, add `lv_timer_handler();`. Temporarily comment out `stateMachine.update();` and `displayController.updateAnimation();`.
    *   **Build & Test:** Compile and upload. Expect to see the "Main FW LVGL OK" label on the round display. This verifies core display/LVGL functionality in `main.cpp`. **(DONE - Verified 2025-05-14)**
    *   **Commit Point.** **(DONE - Phase 1, Step 1 completed and verified)**

**Phase 2: Controller Re-Integration & Hardware Adaptation**
*   **Goal:** Update individual controllers to work with the new hardware pins and drivers.
1.  **[ ] Update `LEDController` (`firmware/src/controllers/LEDController.cpp`):**
    *   **Action:** Since `Config.h` will be updated (Phase 0), `LEDController` should use the new `LED_PIN`. Verify `Adafruit_NeoPixel` usage is standard.
    *   In `main.cpp -> setup()`, uncomment `ledController.begin();`. Add a test call (e.g., `ledController.setSolid(0x00FF00);`).
    *   **Build & Test:** Verify LEDs initialize and respond as expected.
    *   **Commit Point.**
2.  **[ ] Update `InputController` for Encoder (`firmware/src/controllers/InputController.cpp`):**
    *   **Action:**
        *   Constructor will receive updated encoder pins from `Config.h`.
        *   Modify `InputController::begin()`:
            *   Remove setup for the old `BUTTON_PIN`.
            *   Ensure encoder pins use `INPUT_PULLUP`.
            *   Implement interrupt-driven `encoder.tick()` calls, referencing `MinimalDisplayTest.cpp`. This might involve static ISRs or a global ISR calling a public `tick()` method in `InputController`. Set `LatchMode` (e.g., `TWO03`).
        *   Modify `InputController::update()` to remove button polling. Encoder position is now ISR-driven.
    *   In `main.cpp -> setup()`, uncomment `inputController.begin();`.
    *   In `main.cpp -> loop()`, add temporary serial printing of encoder position (may need to add/expose a getter in `InputController`).
    *   **Build & Test:** Verify encoder values are correctly read via interrupts.
    *   **Commit Point.**
3.  **[ ] Refactor `DisplayController` - Phase 1: Decouple Old Display, Basic LVGL Structure**
    *   **Goal:** Remove old display dependencies and prepare `DisplayController` for LVGL.
    *   **Action (`DisplayController.h`):**
        *   Remove `Adafruit_SSD1306 oled;` member.
        *   Adjust constructor (remove old OLED params).
    *   **Action (`DisplayController.cpp`):**
        *   `DisplayController::begin()`: Remove all old `oled.*` calls. Can be minimal for now.
        *   For *every* drawing method (e.g., `drawSplashScreen`, `drawIdleScreen`, etc.):
            *   Comment out all existing `oled.*` drawing commands.
            *   Add `Serial.println("DisplayController::methodName called");`.
            *   Add a placeholder LVGL action (e.g., clear screen, show a label with method name).
                ```cpp
                // Example for drawIdleScreen
                if (lv_screen_active()) { // Check if screen is active
                    lv_obj_clean(lv_screen_active()); 
                    lv_obj_t* label = lv_label_create(lv_screen_active());
                    lv_label_set_text_fmt(label, "Screen: Placeholder for Idle");
                    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
                }
                ```
    *   In `main.cpp -> setup()`, uncomment `displayController.begin();`.
    *   **Build & Test:** Firmware should compile and run. Serial logs should indicate when draw methods are called (if other logic is re-enabled). Placeholder LVGL screens should appear if state machine logic calls these stubs.
    *   **Commit Point.**

**Phase 3: State-by-State UI Migration to LVGL & Touch Integration**
*   **Goal:** Iteratively rewrite the UI for each state using LVGL and integrate touch interactions.
1.  **[ ] Startup State & Splash Screen:**
    *   **Files:** `firmware/src/states/StartupState.cpp`, `DisplayController.cpp`.
    *   **Action (`DisplayController::drawSplashScreen`):** Implement the actual splash screen using LVGL objects (e.g., `lv_image`, `lv_label`).
    *   **Action (`StartupState::enter` or `update`):** Ensure it calls `displayController.drawSplashScreen()`.
    *   In `main.cpp -> setup()`, uncomment `stateMachine.changeState(&StateMachine::startupState);` and `projectManager.begin();` (if `StartupState` depends on it). Also uncomment `networkController.begin();` if needed by early states.
    *   In `main.cpp -> loop()`, uncomment `stateMachine.update();`.
    *   **Build & Test:** Verify the new LVGL splash screen appears.
    *   **Commit Point.**
2.  **[ ] Idle State UI & Basic "Touch" Interaction:**
    *   **Files:** `firmware/src/states/IdleState.cpp`, `DisplayController.cpp`.
    *   **Action (`DisplayController::drawIdleScreen`):** Design and implement the idle screen using LVGL. Include a touchable area/widget (e.g., full-screen `lv_obj` or `lv_button`) for interaction.
    *   **Action (`IdleState::enter`):**
        *   Remove `inputController.onPressHandler(...)` for the physical button.
        *   In `DisplayController::drawIdleScreen` (or called from `IdleState::enter`), create the touchable LVGL object and assign an LVGL event callback. This callback will perform actions previously done by the physical button press (e.g., `stateMachine.changeState(&StateMachine::projectSelectState);`).
    *   Encoder interactions (`onEncoderRotateHandler`) should remain functional.
    *   **Build & Test:** Verify LVGL idle screen. Test touch interaction transitions to `ProjectSelectState` (which will show its placeholder LVGL screen). Test encoder rotation.
    *   **Commit Point.**
3.  **[ ] Project Select State UI & Touch:**
    *   **Files:** `firmware/src/states/ProjectSelectState.cpp`, `DisplayController.cpp`.
    *   **Action (`DisplayController::drawProjectSelectionScreen`):** Implement using LVGL (e.g., `lv_roller` or list of `lv_button`s).
    *   **Action (`ProjectSelectState::enter` and `update`):**
        *   Populate LVGL list/roller from `projectManager.getProjects()`.
        *   LVGL event callbacks on list items/roller update selection.
        *   A "Confirm" LVGL button or similar interaction triggers state change.
    *   **Build & Test:** Verify project list display, selection, and transition.
    *   **Commit Point.**
4.  **[ ] Continue for other states (AdjustState, TimerState, PausedState, DoneState, etc.):**
    *   For each state:
        *   Implement its UI in `DisplayController::draw...Screen()` using LVGL.
        *   Adapt `State::enter()` for LVGL event callbacks for touch, replacing old physical button handlers.
    *   **Build & Test each state's UI and basic interaction.**
    *   **Commit frequently.**

**Phase 4: Refinements**
*   **Goal:** Finalize UI, animations, and conduct thorough testing.
1.  **[ ] Animations:** Adapt old bitmap animations or implement new LVGL animations. Test `displayController.updateAnimation()`.
2.  **[ ] Network State & Provisioning UI:** Update `ProvisionState` and `DisplayController::drawProvisionScreen()` for LVGL.
3.  **[ ] Error Handling and Edge Cases:** Thoroughly test all transitions and interactions.
4.  **[ ] Code Cleanup:** Remove dead code from the old UI system.

---
This detailed plan should serve as a good roadmap. We will tackle Phase 0 first.

## Notes & References
- Original Wiring Diagram (provided).
- Seeed Display Docs: [https://wiki.seeedstudio.com/get_start_round_display/](https://wiki.seeedstudio.com/get_start_round_display/)
- Seeed Studio XIAO ESP32S3 Plus: [https://www.seeedstudio.com/Seeed-Studio-XIAO-ESP32S3-Plus-p-6361.html](https://www.seeedstudio.com/Seeed-Studio-XIAO-ESP32S3-Plus-p-6361.html)
- XIAO ESP32S3 Plus Pinout and documentation.
- Seeed LVGL & TFT Guide for Round Display: [https://wiki.seeedstudio.com/using_lvgl_and_tft_on_round_display/](https://wiki.seeedstudio.com/using_lvgl_and_tft_on_round_display/)
- Required Libraries: `lvgl/lvgl`, `Seeed-Studio/Seeed_Arduino_RoundDisplay`, `lewisxhe/PCF8563_Library`, `Adafruit_NeoPixel`, `mathertel/RotaryEncoder`, (potentially `bodmer/TFT_eSPI` as dependency).
- The display controller is GC9A01.
- **Touch Interrupt:** Confirmed to require D7 (GPIO44).
- **Encoder:** Requires interrupt handling on D6 (GPIO43) and D9 (GPIO8) for reliable operation.
- Need to map and assign pins for NeoPixels, display SPI, I2C for touch/RTC on the XIAO ESP32S3 Plus.
- Using 16 LED NeoPixel for visual indicators.
- Bourns PER35 35mm rotary encoder will replace the previous encoder but has the same connection points. 

### NVS (Non-Volatile Storage) Stabilization in Main Firmware
- **Context:** The main firmware utilizes the Arduino `Preferences` library for NVS operations. Issues with NVS reliability were encountered when migrating to new hardware (XIAO ESP32S3).
- **Key Fixes Implemented:**
    1.  **Robust Initialization in `main.cpp`:** A robust `nvs_flash_init()` call, including `nvs_flash_erase()` on `ESP_ERR_NVS_NO_FREE_PAGES` or `ESP_ERR_NVS_NEW_VERSION_FOUND` errors, was added at the beginning of `setup()` in `firmware/src/main.cpp`. This ensures the NVS partition is healthy before any `Preferences.begin()` calls are made by other modules.
        ```cpp
        // Example from main.cpp setup()
        esp_err_t nvs_err = nvs_flash_init();
        if (nvs_err == ESP_ERR_NVS_NO_FREE_PAGES || nvs_err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            Serial.println("NVS: Erasing and re-initializing flash...");
            ESP_ERROR_CHECK(nvs_flash_erase());
            nvs_err = nvs_flash_init();
        }
        ESP_ERROR_CHECK(nvs_err);
        Serial.println("NVS: Flash initialized successfully by main.cpp.");
        ```
    2.  **Removed Redundant `nvs_flash_init()`:** The direct call to `nvs_flash_init()` in `firmware/src/states/IdleState.cpp` was removed to centralize initialization in `main.cpp`.
    3.  **Corrected `Preferences` Usage:** Ensured `Preferences.begin()` is called with the correct `readOnly` flag (e.g., `false` for write operations, as corrected in `IdleState::setTimer`).
- **Impact:** These changes make the existing `Preferences`-based NVS system in the main firmware more resilient, particularly for initial deployment on new or different hardware where the NVS partition state might be uncertain.
- **Further Testing:** While the NVS foundation is now more stable, full verification of all NVS-dependent application features (project saving/loading, settings persistence) will occur as other hardware components are integrated and the application logic is fully testable on the new platform.

## Development Workflow & Commands

Use the following PlatformIO commands in your terminal from the project's root directory:

*   **Clean Build Artifacts:**
    ```bash
    pio run -t clean
    ```
    *Removes previous build files. Useful before a fresh build or if encountering strange build errors.*

*   **Build Firmware:**
    ```bash
    pio run
    ```
    *Compiles the code and links the firmware binary.*

*   **Build and Upload Firmware:**
    ```bash
    pio run -t upload
    ```
    *Builds the firmware (if needed) and uploads it to the connected XIAO ESP32S3.*

*   **Open Serial Monitor:**
    ```bash
    pio device monitor
    ```
    *Connects to the device's serial port to view `Serial.print()` output. Use `Ctrl+C` to exit.*

*   **Upload and Monitor:**
    ```bash
    pio run -t upload -t monitor
    ```
    *Uploads the firmware and immediately opens the serial monitor.*

**Note:** Ensure the XIAO ESP32S3 is connected via USB and the correct port is selected (PlatformIO usually auto-detects, but may need manual configuration in `platformio.ini` if issues arise). 

**DONE - Partially (Button handlers in States)** Temporarily comment out or adapt any direct usage of `DisplayController` methods that rely on the old OLED, `Animation` class usages, and `InputController` methods that rely on the physical button (e.g., `onPressHandler`, `onDoublePressHandler`, `onLongPressHandler`) within each state file (`AdjustState.cpp`, `DoneState.cpp`, `IdleState.cpp`, `PausedState.cpp`, `ProjectSelectState.cpp`, `ProvisionState.cpp`, `ResetState.cpp`, `SleepState.cpp`, `StartupState.cpp`, `TimerState.cpp`).
    *   **DONE** Temporarily comment out or adapt any direct usage of `DisplayController` methods that rely on the old OLED, `Animation` class usages, and `InputController` methods that rely on the physical button (e.g., `onPressHandler`, `onDoublePressHandler`, `onLongPressHandler`) within each state file (`AdjustState.cpp`, `DoneState.cpp`, `IdleState.cpp`, `PausedState.cpp`, `ProjectSelectState.cpp`, `ProvisionState.cpp`, `ResetState.cpp`, `SleepState.cpp`, `StartupState.cpp`, `TimerState.cpp`).
        *   **DONE:** `AdjustState.cpp` - `onPressHandler` commented.
        *   **DONE:** `DoneState.cpp` - `onPressHandler` commented.
        *   **DONE:** `IdleState.cpp` - `onPressHandler`, `onLongPressHandler` commented.
        *   **DONE:** `PausedState.cpp` - `onPressHandler`, `onDoublePressHandler` commented.
        *   **DONE:** `ProjectSelectState.cpp` - `onPressHandler`, `onDoublePressHandler` commented.
        *   **DONE:** `ProvisionState.cpp` - No button handlers found.
        *   **DONE:** `ResetState.cpp` - `onPressHandler` commented.
        *   **DONE:** `SleepState.cpp` - `onPressHandler`, `onLongPressHandler` commented.
        *   **DONE:** `StartupState.cpp` - No button handlers found.
        *   **DONE:** `TimerState.cpp` - `onPressHandler`, `onDoublePressHandler` commented. 