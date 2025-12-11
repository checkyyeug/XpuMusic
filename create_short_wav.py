#!/usr/bin/env python3
import wave
import numpy as np
import struct

def create_wav(filename, frequency=1000, duration=3, sample_rate=44100):
    """Create a short WAV file"""

    # Generate sine wave
    t = np.linspace(0, duration, int(sample_rate * duration), False)
    samples = np.sin(frequency * 2 * np.pi * t)

    # Convert to 16-bit PCM
    samples_int = (samples * 32767).astype(np.int16)

    # Create stereo
    stereo_samples = np.column_stack((samples_int, samples_int))

    # Write WAV file
    with wave.open(filename, 'w') as wav:
        wav.setnchannels(2)
        wav.setsampwidth(2)  # 16-bit
        wav.setframerate(sample_rate)
        wav.writeframes(stereo_samples.tobytes())

    print(f"Created {filename} - {duration} seconds")

# Create 3-second test file
create_wav("test_3sec.wav", frequency=1000, duration=3)
print("\nPlay with:")
print("  build/bin/Debug/music-player.exe test_3sec.wav")