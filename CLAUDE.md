# Task Master AI - Claude Code Integration Guide

## CRITICAL: DEVICE SLEEP BEHAVIOR - MUST READ FIRST

**The device has a 3-minute inactivity timeout and will almost always be asleep when you try to interact with it.**

### Sleep States:
1. **Light Sleep** (after 3 minutes of inactivity on idle screen):
   - Can be woken by: Touch screen, encoder rotation, BOOT button, or **UART data**
   - WiFi is disabled but device resumes quickly
   
2. **Deep Sleep** (when power button is pressed):
   - Can ONLY be woken by: BOOT button (GPIO 0)
   - Device fully restarts on wake

### ALWAYS WAKE THE DEVICE BEFORE ANY OPERATION:
```bash
# Wake the device first (required before EVERY operation)
./wake_device.sh

# Or manually:
echo -e "\n" > /dev/cu.usbmodem32301
sleep 0.5
```

## device wiki. always reference this. 
https://www.waveshare.com/wiki/ESP32-S3-Touch-AMOLED-1.75

## CRITICAL: POWER MANAGEMENT REQUIREMENTS - MUST BE MET WITHOUT EXCEPTION

### Power Button Sleep/Wake:
1. When the power button is pressed, the device enters sleep mode and can only be awakened by pressing the power button again.

### Inactivity Sleep/Wake:
2. On the idle screen, after three minutes of inactivity, the device enters sleep mode. The device can be awakened by either:
   - Touching the screen
   - Rotating the encoder

**These requirements must be met consistently and without exception.**

**Note: As of the latest firmware update, the 3-minute inactivity timeout is currently DISABLED in the code (commented out). The device will only sleep when the power button is pressed. The wake mechanisms above still apply for power button sleep.**

## CRITICAL: WEB SERVER REBOOT ISSUE

**KNOWN BUG**: Accessing the web interface after waking the device from sleep can cause a complete reboot. This is a known stability issue documented in FAILED_ATTEMPTS_COMPREHENSIVE.md.

### Root Cause:
- WiFi reconnection after wake triggers system instability
- Sleep/wake cycle causes memory corruption or stack issues
- Multiple ESP32-S3 sleep mechanisms conflict with web server

### Workarounds:
1. **Access web interface BEFORE device goes to sleep** - Most reliable
2. **Restart device if web access needed after wake** - `clearwifi` command restarts
3. **Use serial commands instead** - `test`, `sleep`, `stayawake` work reliably
4. **If reboot occurs** - Wait 30 seconds for full boot and reconnection

### Commands that work reliably after wake:
- Serial monitor communication
- `test` command for hardware validation  
- `sleep` command to enter deep sleep
- `stayawake` command (placeholder)

---

## CRITICAL: Arduino Compilation and Upload Commands

### ALWAYS USE THESE EXACT COMMANDS - THIS DEVICE HAS 16MB FLASH (NOT 4MB!)

```bash
# COMPILE ONLY (Normal Firmware)
cd /Users/Travis/Developer/ProjectTimerDevice/firmware && \
arduino-cli compile --fqbn esp32:esp32:esp32s3:USBMode=hwcdc,FlashSize=16M,PartitionScheme=huge_app --build-property "build.psram_type=opi" --clean

# COMPILE WITH TEST MODE
cd /Users/Travis/Developer/ProjectTimerDevice/firmware && \
arduino-cli compile --fqbn esp32:esp32:esp32s3:USBMode=hwcdc,FlashSize=16M,PartitionScheme=huge_app --build-property "build.psram_type=opi" --build-property "compiler.cpp.extra_flags=-DTEST_MODE" --clean

# UPLOAD (after compilation) - ALWAYS WAKE FIRST
./wake_device.sh && \
arduino-cli upload -p /dev/cu.usbmodem32301 --fqbn esp32:esp32:esp32s3:USBMode=hwcdc,FlashSize=16M,PartitionScheme=huge_app

# WAKE + COMPILE + UPLOAD + MONITOR (All in one - RECOMMENDED)
cd /Users/Travis/Developer/ProjectTimerDevice/firmware && \
./wake_device.sh && \
arduino-cli compile --fqbn esp32:esp32:esp32s3:USBMode=hwcdc,FlashSize=16M,PartitionScheme=huge_app --build-property "build.psram_type=opi" --clean && \
./wake_device.sh && \
arduino-cli upload -p /dev/cu.usbmodem32301 --fqbn esp32:esp32:esp32s3:USBMode=hwcdc,FlashSize=16M,PartitionScheme=huge_app && \
esptool.py --port /dev/cu.usbmodem32301 read_mac >/dev/null 2>&1 && \
sleep 0.5 && \
timeout 30 cat /dev/cu.usbmodem32301
```

### WHY THESE EXACT PARAMETERS ARE CRITICAL:
- **FlashSize=16M** - Device has 16MB flash, NOT the default 4MB
- **PartitionScheme=huge_app** - Required for 2MB firmware (default partition too small)
- **build.psram_type=opi** - Required for 8MB PSRAM to work
- **--clean** - Ensures fresh build (use when switching between TEST_MODE and normal)

### CRITICAL: FAILED ATTEMPTS REFERENCE
**BEFORE ANY CHANGES**: Always check `/firmware/FAILED_ATTEMPTS_COMPREHENSIVE.md` 
- Documents 4+ days of failed attempts and root causes
- Contains solutions that DO NOT WORK and must never be repeated
- Updated with latest failed dual-mode sleep attempts (ext0+ext1 conflict)
- Reference this file before implementing ANY sleep, LED, or wake functionality

## Arduino Compilation and Upload

### CRITICAL: ESP32-S3-Touch-AMOLED-1.75 Board Configuration

**Device Specifications:**
- **Flash Size: 16MB** (NOT the default 4MB!)
- **PSRAM: 8MB OPI PSRAM**
- **Required partition scheme: huge_app** (provides 3MB for app, firmware needs 1.57MB)

**ALWAYS USE THESE EXACT COMMANDS:**

```bash
# Compile (from firmware directory)
arduino-cli compile --fqbn esp32:esp32:esp32s3:USBMode=hwcdc,FlashSize=16M,PartitionScheme=huge_app --build-property "build.psram_type=opi" --clean

# Upload
arduino-cli upload -p /dev/cu.usbmodem32301 --fqbn esp32:esp32:esp32s3:USBMode=hwcdc,FlashSize=16M,PartitionScheme=huge_app
```

**Why this is critical:**
- Default settings assume 4MB flash with 1.2MB app partition
- This firmware is 1.57MB and WILL NOT FIT in default partition
- Missing `FlashSize=16M` causes "Sketch too big" error every time
- The `huge_app` partition provides 3MB for the application

### LittleFS Filesystem for Web Interface

**The device includes a web interface accessible at http://thetimer.local when connected to WiFi**

The `data/` directory contains:
- `index.html` - Main web interface
- `style.css` - Styling  
- `app.js` - JavaScript functionality

This must be uploaded separately to the SPIFFS partition (9.56MB at offset 0x6E2000).

**Existing LittleFS files in firmware directory:**
- `littlefs.bin` (1.6MB) - Older/minimal filesystem
- `littlefs_new.bin` (2.0MB) - Another version
- `littlefs_web.bin` (10MB) - Full size but may be outdated
- `littlefs_current.bin` - Created fresh from current data/ directory (recommended)

## Arduino Monitoring Workflow

### IMPORTANT: Reliable Serial Monitoring for ESP32-S3

**Problem**: arduino-cli monitor often misses startup messages, making debugging difficult.

**Solution**: Use esptool to reset the board and cat to capture output.

### Method 1: Wake, Reset and Monitor (RECOMMENDED)
```bash
# Find the current port
ls /dev/cu.* | grep -E "(usbmodem|wchusbserial)"

# Wake device first, then reset and monitor
./wake_device.sh && \
esptool.py --port /dev/cu.usbmodem32301 read_mac >/dev/null 2>&1 && sleep 0.5 && timeout 20 cat /dev/cu.usbmodem32301
```

### Method 2: Full Build, Upload, Reset and Monitor (WITH WEB INTERFACE)
```bash
# CRITICAL: This device has 16MB flash - MUST specify FlashSize=16M and huge_app partition!
# Complete workflow with proper startup capture AND web interface
cd /Users/Travis/Developer/ProjectTimerDevice/firmware && \
./wake_device.sh && \
arduino-cli compile --fqbn esp32:esp32:esp32s3:USBMode=hwcdc,FlashSize=16M,PartitionScheme=huge_app --build-property "build.psram_type=opi" --clean && \
./wake_device.sh && \
arduino-cli upload -p /dev/cu.usbmodem32301 --fqbn esp32:esp32:esp32s3:USBMode=hwcdc,FlashSize=16M,PartitionScheme=huge_app && \
./mklittlefs/mklittlefs -c data -b 4096 -p 256 -s 0x91E000 littlefs_current.bin && \
./wake_device.sh && \
esptool.py --chip esp32s3 --port /dev/cu.usbmodem32301 --baud 921600 write_flash 0x6E2000 littlefs_current.bin && \
sleep 2 && \
esptool.py --port /dev/cu.usbmodem32301 read_mac >/dev/null 2>&1 && \
sleep 0.5 && \
timeout 30 cat /dev/cu.usbmodem32301
```

### LittleFS Web Interface Upload
```bash
# The device includes a web interface at http://thetimer.local when connected to WiFi
# To update just the web interface without recompiling:
cd /Users/Travis/Developer/ProjectTimerDevice/firmware && \
./mklittlefs/mklittlefs -c data -b 4096 -p 256 -s 0x91E000 littlefs_current.bin && \
esptool.py --chip esp32s3 --port /dev/cu.usbmodem32301 --baud 921600 write_flash 0x6E2000 littlefs_current.bin
```

### Method 3: State Machine Testing Procedure
```bash
# Reset and monitor startup to verify initialization
esptool.py --port /dev/cu.usbmodem32301 read_mac >/dev/null 2>&1 && sleep 0.5 && timeout 20 cat /dev/cu.usbmodem32301

# Send test command to run integrated state machine tests
echo "test" > /dev/cu.usbmodem32301 && timeout 15 cat /dev/cu.usbmodem32301

# Full reset, monitor, and test sequence
esptool.py --port /dev/cu.usbmodem32301 read_mac >/dev/null 2>&1 && sleep 0.5 && (timeout 5 cat /dev/cu.usbmodem32301 &) && sleep 2 && echo "test" > /dev/cu.usbmodem32301 && sleep 5
```

### Expected Test Output:
```
Timer Arduino - Starting
Setup complete
--- Integration Testing Available ---
Send 'test' via serial to run state machine tests
=== TEST MODE ACTIVATED ===
=== TEST MODE COMPLETED ===
Type 'test' again to run tests, or continue normal operation...
```

### Method 4: Wake from Sleep and Upload
```bash
# Wake ESP32-S3 from auto-sleep mode and upload firmware
esptool.py --port /dev/cu.usbmodem32301 --before default_reset --after hard_reset chip_id && sleep 1 && cd "/Users/Travis/Developer/ProjectTimerDevice/firmware" && arduino-cli upload --fqbn esp32:esp32:esp32s3:USBMode=hwcdc,FlashSize=16M,PartitionScheme=huge_app --port /dev/cu.usbmodem32301
```

### Key Points:
- **arduino-cli monitor misses startup** - It connects too slowly
- **esptool.py read_mac** - Reliably resets the board via RTS pin
- **cat** - Captures raw output more reliably than arduino-cli monitor
- **Timing is critical** - Small delays ensure proper sequencing
- **timeout prevents hanging** - Use 20-30 seconds typically
- **"test" command activates integrated testing** - Validates state machine implementation
- **Test completion confirms system stability** - Tests run and return to normal operation
- **ESP32-S3 auto-sleeps** - Use esptool with default_reset to wake from sleep before upload
- **Sleep mode blocks uploads** - Always reset first if upload fails with "No serial data received"

_This guide ensures Claude Code has immediate access to Task Master's essential functionality for agentic development workflows._

## Task Master AI Instructions
**Import Task Master's development workflow commands and guidelines, treat as if import is in the main CLAUDE.md file.**
@./.taskmaster/CLAUDE.md
