#!/bin/bash

# Copy required libraries from AMOLED examples
echo "Copying libraries..."

# Remove existing libraries directory if it exists
rm -rf libraries

# Copy the entire libraries folder
cp -r ../AMOLEDEXAMPLES/Arduino-v3.1.0/libraries .

echo "Libraries copied successfully!"
echo "You can now open TheTimerArduino.ino in Arduino IDE"