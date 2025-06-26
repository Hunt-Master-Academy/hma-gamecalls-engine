import numpy as np
import wave
import os

def create_sine_wave(filename, frequency, duration, sample_rate=44100, amplitude=0.5):
    """Generates a simple sine wave and saves it as a WAV file."""
    print(f"Generating sine wave: {filename} ({frequency} Hz)")
    t = np.linspace(0, duration, int(sample_rate * duration), False)
    # Generate sine wave
    samples = np.sin(2 * np.pi * frequency * t)
    # Normalize and convert to 16-bit PCM
    samples = (samples * amplitude * 32767).astype(np.int16)
    
    with wave.open(filename, 'w') as wav:
        wav.setnchannels(1)
        wav.setsampwidth(2)
        wav.setframerate(sample_rate)
        wav.writeframes(samples.tobytes())

def create_complex_wave(filename, frequencies, amplitudes, duration, sample_rate=44100):
    """Generates a complex waveform by summing multiple sine waves."""
    print(f"Generating complex wave: {filename}")
    if len(frequencies) != len(amplitudes):
        raise ValueError("Frequencies and amplitudes must have the same length.")

    t = np.linspace(0, duration, int(sample_rate * duration), False)
    
    # Create an empty array to store the combined waveform
    combined_samples = np.zeros(len(t))

    # Add each sine wave component
    for freq, amp in zip(frequencies, amplitudes):
        combined_samples += amp * np.sin(2 * np.pi * freq * t)
    
    # Normalize the combined waveform to prevent clipping
    max_amp = np.max(np.abs(combined_samples))
    if max_amp > 1.0:
        combined_samples /= max_amp
        
    # Convert to 16-bit PCM
    samples = (combined_samples * 0.5 * 32767).astype(np.int16)

    with wave.open(filename, 'w') as wav:
        wav.setnchannels(1)
        wav.setsampwidth(2)
        wav.setframerate(sample_rate)
        wav.writeframes(samples.tobytes())


if __name__ == "__main__":
    # --- Configuration ---
    OUTPUT_DIR = os.path.join("..", "data", "test_audio")
    SAMPLE_RATE = 44100
    DURATION = 1.0 # 1 second for all test files

    # --- Setup ---
    # Create the output directory if it doesn't exist
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    print(f"Ensuring output directory exists: {os.path.abspath(OUTPUT_DIR)}")

    # --- Generate Simple Sine Waves ---
    create_sine_wave(
        os.path.join(OUTPUT_DIR, "sine_440.wav"),
        frequency=440,
        duration=DURATION,
        sample_rate=SAMPLE_RATE
    )
    create_sine_wave(
        os.path.join(OUTPUT_DIR, "sine_880.wav"),
        frequency=880,
        duration=DURATION,
        sample_rate=SAMPLE_RATE
    )

    # --- Generate Complex Waveform ---
    # A mix of a fundamental frequency and several harmonics
    complex_frequencies = [220, 440, 660, 880]  # A fundamental tone with 3 harmonics
    complex_amplitudes = [0.6, 0.2, 0.1, 0.05] # Harmonics have lower amplitude
    
    create_complex_wave(
        os.path.join(OUTPUT_DIR, "complex.wav"),
        frequencies=complex_frequencies,
        amplitudes=complex_amplitudes,
        duration=DURATION,
        sample_rate=SAMPLE_RATE
    )

    print("\nTest audio files generated successfully.")

