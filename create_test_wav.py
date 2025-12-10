#!/usr/bin/env python3
"""
Create a simple test WAV file for testing purposes
"""

import struct
import math

def create_wav(filename, duration=1.0, frequency=440.0, sample_rate=44100):
    """Create a simple WAV file with a sine wave"""

    # WAV header
    sample_rate = int(sample_rate)
    num_channels = 2
    bits_per_sample = 16
    byte_rate = sample_rate * num_channels * bits_per_sample // 8
    block_align = num_channels * bits_per_sample // 8

    # Calculate data size
    num_samples = int(sample_rate * duration)
    data_size = num_samples * num_channels * bits_per_sample // 8
    file_size = 36 + data_size

    # Create header
    header = struct.pack('<4sL4s', b'RIFF', file_size, b'WAVE')
    fmt = struct.pack('<4sLHHLLHH', b'fmt ', 16, 1, num_channels,
                      sample_rate, byte_rate, block_align, bits_per_sample)
    data_header = struct.pack('<4sL', b'data', data_size)

    # Generate audio data (sine wave)
    audio_data = bytearray()
    for i in range(num_samples):
        t = i / sample_rate
        value = int(32767 * math.sin(2 * math.pi * frequency * t))
        # Stereo - same value for both channels
        audio_data.extend(struct.pack('<hh', value, value))

    # Write WAV file
    with open(filename, 'wb') as f:
        f.write(header)
        f.write(fmt)
        f.write(data_header)
        f.write(audio_data)

    print(f"Created {filename}")
    print(f"  Duration: {duration}s")
    print(f"  Frequency: {frequency}Hz")
    print(f"  Sample Rate: {sample_rate}Hz")
    print(f"  Channels: {num_channels}")
    print(f"  Bits: {bits_per_sample}")

if __name__ == "__main__":
    create_wav("test_440hz.wav", duration=2.0, frequency=440.0)