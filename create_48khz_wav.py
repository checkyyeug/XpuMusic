#!/usr/bin/env python3
"""
Create a 48kHz 32-bit WAV file for testing
"""
import numpy as np
import wave
import sys

def create_wav(filename, frequency=1000, duration=2, sample_rate=48000):
    """Create a WAV file with specified parameters"""

    # Generate samples
    t = np.linspace(0, duration, int(sample_rate * duration), False)

    # Generate sine wave
    samples = np.sin(frequency * 2 * np.pi * t)

    # Convert to 32-bit float
    samples = samples.astype(np.float32)

    # Write WAV file
    with wave.open(filename, 'w') as wav_file:
        wav_file.setnchannels(2)  # Stereo
        wav_file.setsampwidth(4)  # 32-bit = 4 bytes
        wav_file.setframerate(sample_rate)

        # Write stereo data (same in both channels)
        for sample in samples:
            # Convert float32 to bytes
            bytes_data = sample.tobytes()
            wav_file.writeframes(bytes_data + bytes_data)

    print(f"Created {filename}")
    print(f"  Frequency: {frequency} Hz")
    print(f"  Duration: {duration} seconds")
    print(f"  Sample Rate: {sample_rate} Hz")
    print(f"  Format: 32-bit float stereo")

if __name__ == "__main__":
    filename = "test_48khz.wav"
    create_wav(filename, frequency=1000, duration=3)
    print("\nPlay with:")
    print(f"  build\\bin\\Debug\\music-player.exe {filename}")