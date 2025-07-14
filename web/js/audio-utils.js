/**
 * Audio Utilities for Huntmaster Engine Testing
 * Handles audio format conversions, validation, and utility functions
 */

class AudioUtils {
  /**
   * Convert audio buffer to the format expected by the engine
   */
  static convertAudioBuffer(audioBuffer) {
    // Engine expects mono 32-bit float at 44.1kHz
    const targetSampleRate = 44100;
    const channelData = audioBuffer.getChannelData(0); // Use first channel

    if (audioBuffer.sampleRate === targetSampleRate) {
      return channelData;
    }

    // Simple resampling (for more accurate resampling, consider using a library)
    return this.resample(channelData, audioBuffer.sampleRate, targetSampleRate);
  }

  /**
   * Simple linear resampling
   */
  static resample(inputData, inputRate, outputRate) {
    const ratio = inputRate / outputRate;
    const outputLength = Math.round(inputData.length / ratio);
    const output = new Float32Array(outputLength);

    for (let i = 0; i < outputLength; i++) {
      const srcIndex = i * ratio;
      const srcIndexFloor = Math.floor(srcIndex);
      const srcIndexCeil = Math.min(srcIndexFloor + 1, inputData.length - 1);
      const t = srcIndex - srcIndexFloor;

      output[i] =
        inputData[srcIndexFloor] * (1 - t) + inputData[srcIndexCeil] * t;
    }

    return output;
  }

  /**
   * Calculate RMS level of audio data
   */
  static calculateRMS(audioData) {
    let sum = 0;
    for (let i = 0; i < audioData.length; i++) {
      sum += audioData[i] * audioData[i];
    }
    return Math.sqrt(sum / audioData.length);
  }

  /**
   * Calculate peak level of audio data
   */
  static calculatePeak(audioData) {
    let peak = 0;
    for (let i = 0; i < audioData.length; i++) {
      peak = Math.max(peak, Math.abs(audioData[i]));
    }
    return peak;
  }

  /**
   * Convert linear amplitude to dB
   */
  static linearToDb(linear, floor = -60) {
    if (linear <= 0) return floor;
    return Math.max(floor, 20 * Math.log10(linear));
  }

  /**
   * Convert dB to linear amplitude
   */
  static dbToLinear(db) {
    return Math.pow(10, db / 20);
  }

  /**
   * Apply simple high-pass filter to remove DC offset
   */
  static highPassFilter(audioData, cutoffFreq = 80, sampleRate = 44100) {
    const alpha = 1 / (1 + (2 * Math.PI * cutoffFreq) / sampleRate);
    const filtered = new Float32Array(audioData.length);

    let prevInput = 0;
    let prevOutput = 0;

    for (let i = 0; i < audioData.length; i++) {
      filtered[i] = alpha * (prevOutput + audioData[i] - prevInput);
      prevInput = audioData[i];
      prevOutput = filtered[i];
    }

    return filtered;
  }

  /**
   * Normalize audio data to specified peak level
   */
  static normalize(audioData, targetPeak = 0.9) {
    const currentPeak = this.calculatePeak(audioData);
    if (currentPeak === 0) return audioData;

    const gain = targetPeak / currentPeak;
    const normalized = new Float32Array(audioData.length);

    for (let i = 0; i < audioData.length; i++) {
      normalized[i] = audioData[i] * gain;
    }

    return normalized;
  }

  /**
   * Apply simple windowing function
   */
  static applyHannWindow(audioData) {
    const windowed = new Float32Array(audioData.length);
    const N = audioData.length;

    for (let i = 0; i < N; i++) {
      const window = 0.5 * (1 - Math.cos((2 * Math.PI * i) / (N - 1)));
      windowed[i] = audioData[i] * window;
    }

    return windowed;
  }

  /**
   * Detect silence in audio data
   */
  static detectSilence(audioData, threshold = -40) {
    const rms = this.calculateRMS(audioData);
    const db = this.linearToDb(rms);
    return db < threshold;
  }

  /**
   * Find voice activity in audio data
   */
  static detectVoiceActivity(audioData, frameSize = 1024, threshold = -30) {
    const frames = Math.floor(audioData.length / frameSize);
    const activity = [];

    for (let i = 0; i < frames; i++) {
      const start = i * frameSize;
      const end = Math.min(start + frameSize, audioData.length);
      const frame = audioData.slice(start, end);

      const rms = this.calculateRMS(frame);
      const db = this.linearToDb(rms);

      activity.push({
        frameIndex: i,
        startTime: start / 44100, // Assuming 44.1kHz
        endTime: end / 44100,
        rmsDb: db,
        hasVoice: db > threshold,
      });
    }

    return activity;
  }

  /**
   * Create audio context with proper settings
   */
  static createAudioContext() {
    const AudioContext = window.AudioContext || window.webkitAudioContext;
    return new AudioContext({
      sampleRate: 44100,
      latencyHint: "interactive",
    });
  }

  /**
   * Get microphone constraints for optimal recording
   */
  static getMicrophoneConstraints() {
    return {
      audio: {
        sampleRate: 44100,
        channelCount: 1,
        echoCancellation: false,
        noiseSuppression: false,
        autoGainControl: false,
        googEchoCancellation: false,
        googNoiseSuppression: false,
        googAutoGainControl: false,
        googHighpassFilter: false,
        googTypingNoiseDetection: false,
      },
    };
  }

  /**
   * Format file size for display
   */
  static formatFileSize(bytes) {
    if (bytes === 0) return "0 B";

    const k = 1024;
    const sizes = ["B", "KB", "MB", "GB"];
    const i = Math.floor(Math.log(bytes) / Math.log(k));

    return parseFloat((bytes / Math.pow(k, i)).toFixed(1)) + " " + sizes[i];
  }

  /**
   * Format duration for display
   */
  static formatDuration(seconds) {
    const minutes = Math.floor(seconds / 60);
    const remainingSeconds = Math.floor(seconds % 60);
    return `${minutes}:${remainingSeconds.toString().padStart(2, "0")}`;
  }

  /**
   * Validate audio file type
   */
  static isValidAudioFile(file) {
    const validTypes = [
      "audio/wav",
      "audio/wave",
      "audio/x-wav",
      "audio/mpeg",
      "audio/mp3",
      "audio/ogg",
    ];
    const validExtensions = [".wav", ".wave", ".mp3", ".ogg"];

    const hasValidType = validTypes.includes(file.type);
    const hasValidExtension = validExtensions.some((ext) =>
      file.name.toLowerCase().endsWith(ext)
    );

    return hasValidType || hasValidExtension;
  }

  /**
   * Create download link for audio data
   */
  static createDownloadLink(
    audioData,
    filename = "recording.wav",
    sampleRate = 44100
  ) {
    const buffer = this.audioDataToWav(audioData, sampleRate);
    const blob = new Blob([buffer], { type: "audio/wav" });
    const url = URL.createObjectURL(blob);

    const a = document.createElement("a");
    a.href = url;
    a.download = filename;

    return { link: a, url: url };
  }

  /**
   * Convert audio data to WAV format
   */
  static audioDataToWav(audioData, sampleRate = 44100) {
    const buffer = new ArrayBuffer(44 + audioData.length * 2);
    const view = new DataView(buffer);

    // WAV header
    const writeString = (offset, string) => {
      for (let i = 0; i < string.length; i++) {
        view.setUint8(offset + i, string.charCodeAt(i));
      }
    };

    writeString(0, "RIFF");
    view.setUint32(4, 36 + audioData.length * 2, true);
    writeString(8, "WAVE");
    writeString(12, "fmt ");
    view.setUint32(16, 16, true);
    view.setUint16(20, 1, true);
    view.setUint16(22, 1, true);
    view.setUint32(24, sampleRate, true);
    view.setUint32(28, sampleRate * 2, true);
    view.setUint16(32, 2, true);
    view.setUint16(34, 16, true);
    writeString(36, "data");
    view.setUint32(40, audioData.length * 2, true);

    // Convert float samples to 16-bit PCM
    let offset = 44;
    for (let i = 0; i < audioData.length; i++) {
      const sample = Math.max(-1, Math.min(1, audioData[i]));
      view.setInt16(offset, sample * 0x7fff, true);
      offset += 2;
    }

    return buffer;
  }
}

// Export for use in other modules
if (typeof module !== "undefined" && module.exports) {
  module.exports = AudioUtils;
} else {
  window.AudioUtils = AudioUtils;
}
