#!/usr/bin/env python3
import serial
import sys
import time

port = '/dev/cu.usbmodem2101'
print(f"Monitoring {port} at 115200 baud...")

try:
    with serial.Serial(port, 115200, timeout=0.1) as ser:
        while True:
            if ser.in_waiting:
                data = ser.read(ser.in_waiting)
                print(data.decode('utf-8', errors='ignore'), end='')
except KeyboardInterrupt:
    print("\nStopped.")
except Exception as e:
    print(f"Error: {e}")