#!/usr/bin/env python3
"""
Setup script to create the required data directories and provide instructions
for adding master call audio files.
"""

import os
import sys

def create_data_structure():
    """Create the required directory structure for Huntmaster Engine"""
    
    # Define the directory structure
    directories = [
        "../data/master_calls",
        "../data/recordings",
        "../data/features"
    ]
    
    print("=== Setting up Huntmaster Engine Data Directories ===\n")
    
    # Get the current directory (should be run from build directory)
    current_dir = os.getcwd()
    print(f"Current directory: {current_dir}")
    
    # Create directories
    for dir_path in directories:
        full_path = os.path.abspath(dir_path)
        os.makedirs(full_path, exist_ok=True)
        print(f"Created/verified: {full_path}")
    
    print("\n=== Master Call Files Needed ===")
    print("\nPlace the following audio files (MP3 or WAV format) in:")
    print(os.path.abspath("../data/master_calls/"))
    print("\nRequired files:")
    
    calls = [
        "breeding_bellow", "buck_grunt", "buck_rage_grunts",
        "buck-bawl", "contact-bleatr", "doe-grunt", "doebleat",
        "estrus_bleat", "fawn-bleat", "sparring_bucks", "tending_grunts"
    ]
    
    for call in calls:
        wav_path = os.path.abspath(f"../data/master_calls/{call}.wav")
        
        if os.path.exists(wav_path):
            print(f"  ✓ {call}.wav - FOUND")
        else:
            print(f"  ✗ {call}.wav - MISSING")
    
    print("\n=== Test Audio File ===")
    print("\nFor testing, you can record your own deer call WAV files")
    print("Or use the recorded test files from the recording tests.")
    
    # Create a sample WAV file for testing if none exists
    test_wav = os.path.abspath("../data/master_calls/test_tone.wav")
    if not os.path.exists(test_wav):
        print("\nCreating a test tone file for immediate testing...")
        try:
            import numpy as np
            import wave
            
            # Generate a 1 second 440Hz tone
            sample_rate = 44100
            duration = 1.0
            frequency = 440.0
            
            t = np.linspace(0, duration, int(sample_rate * duration))
            samples = (0.5 * np.sin(2 * np.pi * frequency * t) * 32767).astype(np.int16)
            
            with wave.open(test_wav, 'w') as wav_file:
                wav_file.setnchannels(1)
                wav_file.setsampwidth(2)
                wav_file.setframerate(sample_rate)
                wav_file.writeframes(samples.tobytes())
            
            print(f"Created test tone: {test_wav}")
        except ImportError:
            print("Install numpy to auto-generate a test tone: pip install numpy")
    
    print("\n=== Next Steps ===")
    print("1. Add your master call audio files to the master_calls directory")
    print("2. Run: ./GenerateFeatures.exe to pre-compute MFCC features")
    print("3. Run: ./InteractiveRecorder.exe for the interactive test")
    print("4. Run: ./TestRecording.exe for the automated test")

if __name__ == "__main__":
    create_data_structure()