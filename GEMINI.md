# Project Timer Device

## Project Overview

This repository contains the firmware for a custom-built timer device based on the ESP32-S3 microcontroller. The device features a round 1.75-inch AMOLED touchscreen display, a rotary encoder for input, and a 24-LED NeoPixel ring for visual feedback.

The firmware is written in C++ using the Arduino framework and the LVGL graphics library to create a sophisticated user interface. It includes a state machine to manage different modes of operation (idle, timer setting, active timer, etc.), a web server for remote configuration via a web interface, and a system for playing audio alarms.

The core functionality is centered around time management, allowing users to set timers for different projects, which are distinguished by color on the LED ring. The device also supports Wi-Fi connectivity to sync time via NTP and enables remote control through a web-based UI served from the device itself.

## Building and Running

The primary development environment is the Arduino IDE, as detailed in the `firmware/README.md` file.

### Key Dependencies

*   **LVGL:** For the user interface.
*   **Arduino_GFX_Library:** For display driver support.
*   **XPowersLib:** For power management.
*   **SensorPCF85063:** For the Real-Time Clock (RTC).
*   **ArduinoJson:** For handling JSON data for the web API.
*   **LittleFS:** For the onboard file system.

### Build Process

1.  **Copy Libraries:**
    The project requires specific libraries that are included in the `AMOLEDEXAMPLES` directory. A script is provided to copy them into the correct location. From the `firmware` directory, run:
    ```bash
    ./copy_libs.sh
    ```

2.  **Arduino IDE Configuration:**
    *   **Board:** ESP32-S3 Dev Module
    *   **Flash Size:** 4MB
    *   **Partition Scheme:** Default 4MB with spiffs
    *   **PSRAM:** OPI PSRAM
    *   **Flash Mode:** QIO 80MHz

3.  **Upload Firmware:**
    Open `firmware/firmware.ino` in the Arduino IDE and upload the code to the device.

### Web Interface

When the device is connected to Wi-Fi, it starts a web server. You can access the web interface by navigating to `http://thetimer.local` or the IP address of the device. The web interface allows you to:
*   Configure Wi-Fi credentials.
*   Manage projects (add, edit, delete).
*   Set the webhook URL for timer events.
*   Configure alarm sounds.

## Development Conventions

*   **State Machine:** The application logic is structured around a state machine, with different states for each mode of operation (e.g., `IdleState`, `TimerState`, `ProjectSelectState`). State transitions are managed by the `StateMachine` class.
*   **UI Management:** The UI is managed by the `ScreenManager` class, which is responsible for creating and updating the different screens of the application. LVGL is used for all UI components.
*   **Hardware Abstraction:** Pin configurations are defined in `firmware/pin_config.h`. The `firmware.ino` file contains the initialization and handling of hardware components like the display, touch controller, and power management IC.
*   **Web API:** The web server exposes a RESTful API for interacting with the device. API endpoints are defined in the `firmware.ino` file.
*   **File System:** The device uses the LittleFS file system to store configuration data, web files, and alarm sounds. The `data` directory in the `firmware` directory is uploaded to the device's file system.
