# Production Testing Guide for Timer Device

This guide outlines the quality control process for manufacturing the ESP32-S3-Touch-AMOLED-1.75 timer devices.

## Overview

The firmware includes a built-in test framework that can be enabled during compilation. This allows you to thoroughly test each device during production without needing separate test firmware.

## Production Workflow

### Step 1: Assemble Hardware
- Complete physical assembly of the timer device
- Ensure all components are properly connected

### Step 2: Compile and Upload Test Firmware

```bash
# Navigate to firmware directory
cd /Users/Travis/Developer/ProjectTimerDevice/firmware

# Compile firmware with TEST_MODE enabled
arduino-cli compile --fqbn esp32:esp32:esp32s3:USBMode=hwcdc,FlashSize=16M,PartitionScheme=huge_app --build-property "build.psram_type=opi" --build-property "compiler.cpp.extra_flags=-DTEST_MODE" --clean

# Upload to device (adjust port as needed)
arduino-cli upload -p /dev/cu.usbmodem32301 --fqbn esp32:esp32:esp32s3:USBMode=hwcdc,FlashSize=16M,PartitionScheme=huge_app
```

### Step 3: Run Quality Control Tests

1. **Open Serial Monitor**:
   ```bash
   # Reset device and monitor
   esptool.py --port /dev/cu.usbmodem32301 read_mac >/dev/null 2>&1 && sleep 0.5 && timeout 30 cat /dev/cu.usbmodem32301
   ```

2. **Run Test Suite**:
   - Wait for device to boot (you'll see "TEST MODE ENABLED")
   - Type `test` and press Enter
   - Tests will run automatically

3. **Verify Results**:
   - All tests should show ✓ PASS
   - Note any failures for troubleshooting
   - Basic test suite takes ~5 minutes

4. **Optional: Run Extended Tests**:
   - Type `stress` for 4-hour continuous operation test
   - Only needed for first device in batch or after design changes

### Step 4: Flash Production Firmware

```bash
# Compile firmware WITHOUT TEST_MODE (production version)
arduino-cli compile --fqbn esp32:esp32:esp32s3:USBMode=hwcdc,FlashSize=16M,PartitionScheme=huge_app --build-property "build.psram_type=opi" --clean

# Upload production firmware
arduino-cli upload -p /dev/cu.usbmodem32301 --fqbn esp32:esp32:esp32s3:USBMode=hwcdc,FlashSize=16M,PartitionScheme=huge_app

# Optional: Upload web interface
./mklittlefs/mklittlefs -c data -b 4096 -p 256 -s 0x91E000 littlefs_current.bin
esptool.py --chip esp32s3 --port /dev/cu.usbmodem32301 --baud 921600 write_flash 0x6E2000 littlefs_current.bin
```

### Step 5: Final Verification

```bash
# Monitor startup to ensure production firmware works
esptool.py --port /dev/cu.usbmodem32301 read_mac >/dev/null 2>&1 && sleep 0.5 && timeout 20 cat /dev/cu.usbmodem32301
```

- Verify device starts normally
- Check "Integration Testing Available" message (NOT "TEST MODE ENABLED")
- Device is ready for packaging

### Step 6: Package for Shipping
- Device now has production firmware
- No test code included in shipped product
- Ready for customer use

## Test Coverage

### Basic Tests (Always Run)
- Math operations and memory allocation
- String handling
- Basic functionality verification

### Hardware Tests (When Enabled)
- **Display**: Pattern rendering
- **Touch**: Touch detection and response
- **Encoder**: Rotation and button press
- **LED**: NeoPixel color cycling
- **RTC**: Time keeping accuracy
- **WiFi**: Connection capability
- **Power**: Battery and charging status
- **Memory**: PSRAM availability

### Continuous Operation Test (Optional)
- 4+ hour stress test
- Random state transitions
- Memory leak detection
- Stability verification

## Common Issues

### Port Not Found
```bash
# List available ports
ls /dev/cu.* | grep -E "(usbmodem|wchusbserial)"
```

### Device Won't Wake from Sleep
```bash
# Force wake with esptool
esptool.py --port /dev/cu.usbmodem32301 --before default_reset --after hard_reset chip_id
```

### Compilation Errors
- Ensure you're in the firmware directory
- Check that all libraries are installed
- Use `--clean` flag to force fresh compilation

## Important Notes

1. **Flash Size**: This device has 16MB flash (not default 4MB) - the commands above include the correct `FlashSize=16M` parameter
2. **Partition Scheme**: Uses `huge_app` partition for the 2MB firmware
3. **Test Mode**: Only enabled during production testing, never shipped to customers
4. **Web Interface**: The `littlefs_current.bin` contains the web interface accessible at http://thetimer.local

## Quick Reference Card

```bash
# Test Mode Compile & Upload
arduino-cli compile --fqbn esp32:esp32:esp32s3:USBMode=hwcdc,FlashSize=16M,PartitionScheme=huge_app --build-property "build.psram_type=opi" --build-property "compiler.cpp.extra_flags=-DTEST_MODE" --clean && arduino-cli upload -p /dev/cu.usbmodem32301 --fqbn esp32:esp32:esp32s3:USBMode=hwcdc,FlashSize=16M,PartitionScheme=huge_app

# Production Compile & Upload  
arduino-cli compile --fqbn esp32:esp32:esp32s3:USBMode=hwcdc,FlashSize=16M,PartitionScheme=huge_app --build-property "build.psram_type=opi" --clean && arduino-cli upload -p /dev/cu.usbmodem32301 --fqbn esp32:esp32:esp32s3:USBMode=hwcdc,FlashSize=16M,PartitionScheme=huge_app

# Monitor Serial Output
esptool.py --port /dev/cu.usbmodem32301 read_mac >/dev/null 2>&1 && sleep 0.5 && timeout 30 cat /dev/cu.usbmodem32301
```

## Production Checklist

- [ ] Hardware assembled correctly
- [ ] Test firmware uploaded
- [ ] All tests PASS
- [ ] Production firmware uploaded  
- [ ] Final boot verification
- [ ] Web interface uploaded (if needed)
- [ ] Device packaged for shipping