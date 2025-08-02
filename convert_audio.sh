#!/bin/bash

# Audio conversion script for ESP32 alarm sounds
# Converts M4A files to WAV format suitable for ESP32 playback

echo "ESP32 Audio Converter"
echo "===================="
echo "This script converts your M4A files to WAV format for ESP32 playback"
echo ""

# Check if ffmpeg is installed
if ! command -v ffmpeg &> /dev/null; then
    echo "Error: ffmpeg is not installed!"
    echo "Please install ffmpeg first:"
    echo "  brew install ffmpeg"
    exit 1
fi

# Create output directory
OUTPUT_DIR="converted_sounds"
mkdir -p "$OUTPUT_DIR"

# Convert each M4A file
for file in alarm-sounds/*.m4a; do
    if [ -f "$file" ]; then
        filename=$(basename "$file" .m4a)
        echo "Converting: $filename.m4a"
        
        # Convert to WAV with ESP32-friendly settings:
        # - 16-bit PCM
        # - 16kHz sample rate (good quality for alarms, saves memory)
        # - Stereo (I2S expects stereo data)
        ffmpeg -i "$file" \
               -acodec pcm_s16le \
               -ar 16000 \
               -ac 2 \
               "$OUTPUT_DIR/${filename}.wav" \
               -y -loglevel error
        
        if [ $? -eq 0 ]; then
            # Get file info
            size=$(ls -lh "$OUTPUT_DIR/${filename}.wav" | awk '{print $5}')
            duration=$(ffmpeg -i "$OUTPUT_DIR/${filename}.wav" 2>&1 | grep Duration | awk '{print $2}' | cut -d',' -f1)
            echo "  ✓ Converted: ${filename}.wav (${size}, ${duration})"
        else
            echo "  ✗ Failed to convert ${filename}.m4a"
        fi
    fi
done

echo ""
echo "Conversion complete!"
echo "Files saved to: $OUTPUT_DIR/"
echo ""
echo "To copy to SD card:"
echo "1. Insert SD card"
echo "2. Create 'sounds' folder on SD card root"
echo "3. Copy all .wav files from $OUTPUT_DIR/ to /sounds/ on SD card"
echo ""
echo "File sizes:"
ls -lh "$OUTPUT_DIR"/*.wav 2>/dev/null | awk '{print "  " $9 ": " $5}'