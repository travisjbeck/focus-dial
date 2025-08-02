#!/usr/bin/env python3
"""
Convert mono WAV files to stereo by duplicating the channel
"""
import wave
import struct
import os

def convert_mono_to_stereo(input_file, output_file):
    """Convert a mono WAV file to stereo by duplicating the channel."""
    # Open the mono file
    with wave.open(input_file, 'rb') as mono:
        # Check if it's actually mono
        if mono.getnchannels() != 1:
            print(f"  {input_file} is already stereo, skipping...")
            return False
            
        # Get parameters
        params = mono.getparams()
        num_frames = params.nframes
        sample_width = params.sampwidth
        framerate = params.framerate
        
        # Read all frames
        frames = mono.readframes(num_frames)
    
    # Create stereo file
    with wave.open(output_file, 'wb') as stereo:
        # Set parameters for stereo (2 channels)
        stereo.setnchannels(2)
        stereo.setsampwidth(sample_width)
        stereo.setframerate(framerate)
        
        # Convert frames
        if sample_width == 2:  # 16-bit
            # Unpack mono samples
            mono_samples = struct.unpack(f'<{num_frames}h', frames)
            
            # Create stereo by duplicating each sample
            stereo_samples = []
            for sample in mono_samples:
                stereo_samples.extend([sample, sample])  # Left and right
            
            # Pack and write
            stereo_frames = struct.pack(f'<{num_frames * 2}h', *stereo_samples)
            stereo.writeframes(stereo_frames)
        else:
            raise ValueError(f"Unsupported sample width: {sample_width}")
    
    return True

def main():
    input_dir = "converted_sounds"
    output_dir = "firmware/data"
    
    # Create output directory if it doesn't exist
    os.makedirs(output_dir, exist_ok=True)
    
    print("Converting mono WAV files to stereo...")
    print("=====================================")
    
    # Process each WAV file
    for filename in os.listdir(input_dir):
        if filename.endswith('.wav'):
            input_path = os.path.join(input_dir, filename)
            output_path = os.path.join(output_dir, filename)
            
            print(f"\nConverting: {filename}")
            
            try:
                if convert_mono_to_stereo(input_path, output_path):
                    # Get file sizes
                    input_size = os.path.getsize(input_path) / 1024
                    output_size = os.path.getsize(output_path) / 1024
                    
                    print(f"  Input:  {input_size:.1f} KB (mono)")
                    print(f"  Output: {output_size:.1f} KB (stereo)")
                    print(f"  Saved to: {output_path}")
                
            except Exception as e:
                print(f"  Error: {e}")
    
    print("\nConversion complete!")
    
    # List all files in output directory
    print(f"\nFiles in {output_dir}:")
    for f in os.listdir(output_dir):
        path = os.path.join(output_dir, f)
        if os.path.isfile(path):
            size = os.path.getsize(path) / 1024
            print(f"  {f}: {size:.1f} KB")

if __name__ == "__main__":
    main()