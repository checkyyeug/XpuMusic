#!/usr/bin/env python
"""
Test script for music-player.exe
"""

import subprocess
import sys
import time
import os

def test_music_player():
    # Create test WAV file
    print("Creating test WAV file...")
    try:
        import numpy as np
        import wave

        sample_rate = 44100
        duration = 3.0
        frequency = 440.0

        samples = int(sample_rate * duration)
        t = np.linspace(0, duration, samples, False)

        # Create a 2-channel stereo file with different frequencies
        waveform_left = np.sin(2 * np.pi * frequency * t)
        waveform_right = np.sin(2 * np.pi * (frequency * 1.5) * t)

        # Interleave for stereo
        stereo_waveform = np.empty(samples * 2, dtype=np.float32)
        stereo_waveform[0::2] = waveform_left
        stereo_waveform[1::2] = waveform_right

        # Convert to 16-bit
        waveform_int = (stereo_waveform * 32767).astype(np.int16)

        with wave.open('test_stereo.wav', 'w') as wav_file:
            wav_file.setnchannels(2)
            wav_file.setsampwidth(2)
            wav_file.setframerate(sample_rate)
            wav_file.writeframes(waveform_int.tobytes())

        print("Created test_stereo.wav (3 seconds, 440Hz + 660Hz)")
    except Exception as e:
        print(f"Failed to create test file: {e}")
        return False

    # Run music player
    print("\nRunning music-player.exe...")
    player_path = os.path.join(os.path.dirname(__file__), 'build', 'bin', 'Debug', 'music-player.exe')

    try:
        # Start the player
        process = subprocess.Popen([player_path, 'test_stereo.wav'],
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE,
                                   text=True,
                                   creationflags=subprocess.CREATE_NEW_CONSOLE if os.name == 'nt' else 0)

        # Monitor output
        print("Output from music-player:")
        while True:
            output = process.stdout.readline()
            if output == '' and process.poll() is not None:
                break
            if output:
                print(output.strip())

        # Get return code
        return_code = process.poll()
        print(f"\nProcess finished with return code: {return_code}")

        if return_code == 0:
            print("✅ Playback completed successfully!")
            return True
        else:
            print(f"❌ Player failed with code {return_code}")
            return False

    except Exception as e:
        print(f"Failed to run music-player: {e}")
        return False

if __name__ == "__main__":
    success = test_music_player()
    sys.exit(0 if success else 1)