# Timer Arduino - Smooth Scrolling Demo

This is a high-performance smooth scrolling demo with three black screens for the ESP32-S3-Touch-AMOLED-1.75 device.

## Features
- Three scrollable screens with centered text
- Smooth horizontal scrolling with swipe gestures
- 16-bit RGB565 color (no color conversion)
- Optimized for 466x466 AMOLED display

## Hardware
- ESP32-S3-Touch-AMOLED-1.75 (WaveShare)
- QSPI display interface
- Capacitive touch

## Setup Instructions

1. **Copy Libraries**
   ```bash
   # From this directory, copy the required libraries:
   cp -r ../AMOLEDEXAMPLES/Arduino-v3.1.0/libraries/* ./libraries/
   ```

2. **Open in Arduino IDE**
   - Open `TheTimerArduino.ino` in Arduino IDE
   - Select Board: ESP32-S3 Dev Module
   - Select Port: /dev/cu.usbmodem2101 (or your device port)

3. **Configure Arduino IDE**
   - Board Settings:
     - Flash Size: 4MB
     - Partition Scheme: Default 4MB with spiffs
     - PSRAM: OPI PSRAM
     - Flash Mode: QIO 80MHz

4. **Upload**
   - Click Upload button in Arduino IDE
   - The device will show three scrollable black screens with text

## Troubleshooting
- If libraries are not found, ensure you've copied them from the AMOLEDEXAMPLES folder
- If touch doesn't work, check the I2C connections (SDA:15, SCL:14)
- If display doesn't work, verify QSPI connections match pin_config.h