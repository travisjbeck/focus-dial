#!/bin/bash
# Wake ESP32-S3 Timer Device from sleep mode
# The device sleeps after 3 minutes of inactivity

PORT=${1:-/dev/cu.usbmodem32301}

echo "Waking device on port $PORT..."

# Send a newline character to wake the device via UART
echo -e "\n" > "$PORT" 2>/dev/null || true

# Also try with printf as a fallback
printf "\n" > "$PORT" 2>/dev/null || true

# Give the device time to wake up
sleep 0.5

echo "Device should now be awake"