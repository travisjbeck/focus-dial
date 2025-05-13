# Project: Replace OLED with Seeed Studio Round Touch Display
- **Created**: 2024-07-26
- **Status**: Planned
- **Last Updated**: 2024-07-26

## Context & Requirements
The current square monochrome OLED display needs to be replaced with a Seeed Studio 1.28" Round Color Touch Display. This requires hardware wiring changes and significant firmware updates. The existing rotary encoder will be replaced with a smaller Bourns PER35 35mm rotary encoder that has the same connection points and functionality. The separate physical button will be replaced by the touch screen interaction. The project will now use a 16 LED NeoPixel for visual indicators. The target board is the Seeed Studio XIAO ESP32S3 Plus, which features a dual-core ESP32-S3 processor, 20 GPIOs, WiFi, BLE 5.0, 8MB PSRAM, and 16MB Flash.

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

## Development Plan
### Phase 1: Hardware Wiring & Basic Display Test
- [x] Determine correct wiring between Seeed Studio XIAO ESP32S3 Plus and the Seeed Round Display (Power, SPI for Display, I2C for RTC/Touch).
- [x] Document the new wiring scheme, noting any pin conflicts and necessary reassignments (e.g., 16 LED NeoPixel, Button).
- [x] Install necessary libraries (`bodmer/TFT_eSPI`, `lvgl/lvgl`, `Seeed-Studio/Seeed_Arduino_RoundDisplay`, `lewisxhe/PCF8563_Library`).
- [x] Configure `TFT_eSPI` library (`User_Setup.h` or build flags) for the ESP32-S3, GC9A01 controller, and chosen pins.
- [x] Create a minimal test sketch (`DisplayTest.cpp`) to initialize the display, fill the screen with color, display text, and read the RTC time via Serial Monitor.
- [x] Add code to test the 16 LED NeoPixel and verify the Bourns PER35 encoder works correctly.
- [x] Implement and verify Non-Volatile Storage (NVS) for persistent data (e.g., run counter test in `MinimalDisplayTest.cpp`).
- [x] Update `platformio.ini` to include libraries and build the test sketch.
- [x] Flash and verify basic display, RTC, NeoPixel, NVS, and encoder functionality.

**Note:** Successful initial tests for the Seeed Round Display (including touch), Bourns PER35 encoder (interrupt-driven), WS2812 NeoPixel ring (16 LEDs), and Non-Volatile Storage (NVS) were implemented and verified in `firmware/src/MinimalDisplayTest.cpp`. This sketch serves as a working reference for integrating these components, including basic NVS read/write operations.

### Phase 2: Firmware Integration - Core Functionality
- [ ] Integrate the new display driver into the main firmware (`main.cpp`, display controller) using `lv_xiao_disp_init()` and `lv_xiao_touch_init()`.
- [ ] Integrate LVGL tick handling (`lv_timer_handler()`) into the main loop.
- [ ] Replace existing display drawing logic with **LVGL** calls for basic timer/state display.
- [ ] Implement basic touch detection via LVGL input drivers to replace the physical button press for state transitions.
- [ ] Update pin definitions in `Config.h` or equivalent, ensuring correct assignment for Display SPI/I2C, Touch INT (D7/GPIO44), Encoder (D6/GPIO43, D9/GPIO8), and NeoPixel (D0/GPIO1).
- [ ] Integrate **interrupt-driven encoder reading** using `attachInterrupt` on D6 and D9 to call `encoder.tick()`.
- [ ] Refactor state machine or input handling logic to use touch input and the reliable encoder readings.
- [ ] Integrate and test the **PCF8563 Real-Time Clock (RTC)** using `lewisxhe/PCF8563_Library`.
- [ ] Integrate NVS for storing application-specific settings (e.g., last selected project, timer states) leveraging the tested NVS framework.
- [ ] Test core timer functionality with the new display, touch input, encoder, RTC, and NVS.

### Phase 3: Firmware Enhancement - Rich UI & LVGL
- [ ] Design a new UI layout leveraging the round color display using LVGL.
- [ ] Implement project selection via touch interface (list or carousel).
- [ ] Display project-specific colors on the screen.
- [ ] Configure 16 LED NeoPixel to enhance visual feedback.
- [ ] Utilize LVGL widgets (labels, arcs, buttons, etc.) for displaying time, status, and project information.
- [ ] Potentially add graphical elements (e.g., progress arc).
- [ ] Optimize drawing routines for performance and leverage ESP32-S3's dual-core capabilities.
- [ ] Test all features thoroughly.

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

### NVS (Non-Volatile Storage) Implementation
- **Tested in:** `firmware/src/MinimalDisplayTest.cpp`
- **Key Steps:**
    1. Include headers: `#include "nvs_flash.h"` and `#include "nvs.h"`.
    2. Initialize NVS flash in `setup()`:
       ```cpp
       esp_err_t err; // Declare error variable
       err = nvs_flash_init();
       if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
           // NVS partition was truncated and needs to be erased
           // Retry nvs_flash_init
           ESP_ERROR_CHECK(nvs_flash_erase());
           err = nvs_flash_init();
       }
       ESP_ERROR_CHECK(err);
       ```
    3. Declare an `nvs_handle_t` variable globally or in scope (e.g., `nvs_handle_t my_nvs_handle;`).
    4. Open NVS with a namespace and mode (e.g., `err = nvs_open("storage", NVS_READWRITE, &my_nvs_handle);`). Handle errors.
    5. Use `nvs_get_i32()`, `nvs_get_str()`, etc., to read values and `nvs_set_i32()`, `nvs_set_str()`, etc., to write. Handle errors, especially `ESP_ERR_NVS_NOT_FOUND` on first read.
    6. Commit changes using `err = nvs_commit(my_nvs_handle);`. Handle errors.
    7. Optionally close the handle with `nvs_close(my_nvs_handle);` when NVS operations for that handle are complete.
- **Integration into Main Firmware:** The same principles apply. Initialize NVS early in `setup()`. It's good practice to create wrapper functions or a dedicated class to manage NVS read/write operations for specific application settings (e.g., saving current timer state, user preferences, last selected project). This encapsulates NVS logic and makes the main code cleaner. Ensure robust error handling for all NVS operations. The NVS handle can be kept open for the application's lifetime if frequently accessed, or opened/closed as needed.

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