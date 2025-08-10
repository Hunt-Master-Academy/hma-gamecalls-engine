#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>
#include <vector>

#include "dr_wav.h"
#include "huntmaster/core/DebugConfig.h"
#include "huntmaster/core/DebugLogger.h"

using huntmaster::DebugConfig;
using huntmaster::DebugLogger;

// Debug options structure
struct DebugOptions {
    bool enableDebug = false;
    bool enableTrace = false;
    bool enableVerbose = false;
    bool enablePerformanceMetrics = false;
    bool enableVADDebug = false;
    bool enableTrimDebug = false;
    bool enableVisualizationDebug = false;
    bool enableBatchDebug = false;
    bool printHelp = false;

    void parseArgs(int argc, char* argv[]) {
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "--debug" || arg == "-d") {
                enableDebug = true;
            } else if (arg == "--trace") {
                enableTrace = true;
            } else if (arg == "--verbose") {
                enableVerbose = true;
            } else if (arg == "--performance" || arg == "-p") {
                enablePerformanceMetrics = true;
            } else if (arg == "--vad-debug") {
                enableVADDebug = true;
            } else if (arg == "--trim-debug") {
                enableTrimDebug = true;
            } else if (arg == "--viz-debug") {
                enableVisualizationDebug = true;
            } else if (arg == "--batch-debug") {
                enableBatchDebug = true;
            } else if (arg == "--help" || arg == "-h") {
                printHelp = true;
            }
        }
    }

    void printUsage(const char* programName) {
        std::cout << "=== Audio Trimming Tool ===" << std::endl;
        std::cout << "Usage: " << programName << " <input.wav> [output.wav] [options]" << std::endl;
        std::cout << std::endl;
        std::cout << "Arguments:" << std::endl;
        std::cout << "  input.wav        Input audio file to trim" << std::endl;
        std::cout << "  output.wav       Output trimmed audio file (optional)" << std::endl;
        std::cout << std::endl;
        std::cout << "Options:" << std::endl;
        std::cout << "  -v               Visualize waveforms" << std::endl;
        std::cout << "  -t <threshold>   Set silence threshold (default: 0.01)" << std::endl;
        std::cout << "  -batch           Process all WAV files in recordings directory"
                  << std::endl;
        std::cout << std::endl;
        std::cout << "Debug Options:" << std::endl;
        std::cout << "  --debug, -d      Enable debug logging" << std::endl;
        std::cout << "  --trace          Enable trace logging" << std::endl;
        std::cout << "  --verbose        Enable verbose output" << std::endl;
        std::cout << "  --performance, -p Enable performance metrics" << std::endl;
        std::cout << "  --vad-debug      Enable Voice Activity Detection debugging" << std::endl;
        std::cout << "  --trim-debug     Enable trimming process debugging" << std::endl;
        std::cout << "  --viz-debug      Enable visualization debugging" << std::endl;
        std::cout << "  --batch-debug    Enable batch processing debugging" << std::endl;
        std::cout << "  --help, -h       Show this help message" << std::endl;
        std::cout << std::endl;
        std::cout << "Examples:" << std::endl;
        std::cout << "  " << programName << " recording.wav trimmed.wav -v --debug" << std::endl;
        std::cout << "  " << programName << " -batch --performance --trim-debug" << std::endl;
    }
};

// Performance monitoring class
class PerformanceMonitor {
  private:
    std::chrono::high_resolution_clock::time_point startTime;
    std::string operationName;
    bool enabled;

  public:
    PerformanceMonitor(const std::string& name, bool enable = true)
        : operationName(name), enabled(enable) {
        if (enabled) {
            startTime = std::chrono::high_resolution_clock::now();
            DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                           huntmaster::DebugLevel::INFO,
                                           "Starting " + operationName);
        }
    }

    ~PerformanceMonitor() {
        if (enabled) {
            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration =
                std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

            DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                           huntmaster::DebugLevel::INFO,
                                           operationName + " completed in "
                                               + std::to_string(duration.count()) + "ms");
        }
    }

    void checkpoint(const std::string& message) {
        if (enabled) {
            auto currentTime = std::chrono::high_resolution_clock::now();
            auto duration =
                std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime);

            DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                           huntmaster::DebugLevel::DEBUG,
                                           operationName + " - " + message + " (+"
                                               + std::to_string(duration.count()) + "ms)");
        }
    }
};

// Configuration for sound detection
struct VADConfig {
    float silenceThreshold = 0.01f;   // Amplitude threshold for silence (adjustable)
    float energyThreshold = 0.0001f;  // Energy threshold (RMS squared)
    int minSilenceFrames = 2205;      // Minimum silence duration (50ms at 44.1kHz)
    int minSoundFrames = 4410;        // Minimum sound duration (100ms at 44.1kHz)
    float hangoverTime = 0.1f;        // Time to wait after sound ends (seconds)
    bool enableDebug = false;         // Enable debug output for VAD

    void printConfig() const {
        if (enableDebug) {
            DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                           huntmaster::DebugLevel::DEBUG,
                                           "VAD Configuration:");
            DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                           huntmaster::DebugLevel::DEBUG,
                                           "  Silence threshold: "
                                               + std::to_string(silenceThreshold));
            DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                           huntmaster::DebugLevel::DEBUG,
                                           "  Energy threshold: "
                                               + std::to_string(energyThreshold));
            DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                           huntmaster::DebugLevel::DEBUG,
                                           "  Min silence frames: "
                                               + std::to_string(minSilenceFrames));
            DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                           huntmaster::DebugLevel::DEBUG,
                                           "  Min sound frames: " + std::to_string(minSoundFrames));
            DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                           huntmaster::DebugLevel::DEBUG,
                                           "  Hangover time: " + std::to_string(hangoverTime)
                                               + "s");
        }
    }
};

// Calculate RMS energy for a window
float calculateEnergy(const std::vector<float>& samples, size_t start, size_t windowSize) {
    float sum = 0.0f;
    size_t end = std::min(start + windowSize, samples.size());

    for (size_t i = start; i < end; ++i) {
        sum += samples[i] * samples[i];
    }

    return sum / (end - start);  // Mean squared value
}

// Find the start of actual audio (first non-silence)
size_t
findAudioStart(const std::vector<float>& samples, float sampleRate, const VADConfig& config) {
    PerformanceMonitor monitor("Audio start detection", config.enableDebug);

    int windowSize = static_cast<int>(sampleRate * 0.01f);  // 10ms windows
    int consecutiveSoundFrames = 0;
    int requiredFrames = static_cast<int>(sampleRate * 0.02f);  // Need 20ms of sound

    if (config.enableDebug) {
        DebugLogger::getInstance().log(
            huntmaster::DebugComponent::TOOLS,
            huntmaster::DebugLevel::DEBUG,
            "Finding audio start - Window size: " + std::to_string(windowSize)
                + ", Required frames: " + std::to_string(requiredFrames));
    }

    int windowsProcessed = 0;
    for (size_t i = 0; i < samples.size(); i += windowSize / 2) {  // 50% overlap
        float energy = calculateEnergy(samples, i, windowSize);
        float peakInWindow = 0.0f;

        // Also check peak amplitude in window
        for (size_t j = i; j < static_cast<size_t>(std::min(static_cast<int>(i) + windowSize,
                                                            static_cast<int>(samples.size())));
             ++j) {
            peakInWindow = std::max(peakInWindow, std::abs(samples[j]));
        }

        bool isSound = (energy > config.energyThreshold || peakInWindow > config.silenceThreshold);

        if (config.enableDebug && windowsProcessed % 200 == 0) {
            DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                           huntmaster::DebugLevel::TRACE,
                                           "Window " + std::to_string(windowsProcessed)
                                               + " - Energy: " + std::to_string(energy)
                                               + ", Peak: " + std::to_string(peakInWindow)
                                               + ", Sound: " + (isSound ? "YES" : "NO"));
        }

        if (isSound) {
            consecutiveSoundFrames += windowSize / 2;
            if (consecutiveSoundFrames >= requiredFrames) {
                // Found start - backtrack a bit to not cut off attack
                size_t startIdx = std::max(0, static_cast<int>(i) - windowSize);

                if (config.enableDebug) {
                    DebugLogger::getInstance().log(
                        huntmaster::DebugComponent::TOOLS,
                        huntmaster::DebugLevel::INFO,
                        "Audio start found at sample " + std::to_string(startIdx) + " ("
                            + std::to_string(startIdx / sampleRate) + "s)");
                }

                return startIdx;
            }
        } else {
            consecutiveSoundFrames = 0;
        }

        windowsProcessed++;
    }

    if (config.enableDebug) {
        DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                       huntmaster::DebugLevel::WARN,
                                       "No audio start found after processing "
                                           + std::to_string(windowsProcessed) + " windows");
    }

    return 0;  // No sound found
}

// Find the end of actual audio (last non-silence)
size_t findAudioEnd(const std::vector<float>& samples, float sampleRate, const VADConfig& config) {
    PerformanceMonitor monitor("Audio end detection", config.enableDebug);

    int windowSize = static_cast<int>(sampleRate * 0.01f);  // 10ms windows
    int hangoverSamples = static_cast<int>(sampleRate * config.hangoverTime);
    size_t lastSoundFrame = samples.size();

    if (config.enableDebug) {
        DebugLogger::getInstance().log(
            huntmaster::DebugComponent::TOOLS,
            huntmaster::DebugLevel::DEBUG,
            "Finding audio end - Window size: " + std::to_string(windowSize)
                + ", Hangover samples: " + std::to_string(hangoverSamples));
    }

    int windowsProcessed = 0;

    // Search backwards
    for (int i = samples.size() - windowSize; i >= 0; i -= windowSize / 2) {
        float energy = calculateEnergy(samples, i, windowSize);
        float peakInWindow = 0.0f;

        for (size_t j = i; j < static_cast<size_t>(std::min(static_cast<int>(i) + windowSize,
                                                            static_cast<int>(samples.size())));
             ++j) {
            peakInWindow = std::max(peakInWindow, std::abs(samples[j]));
        }

        bool isSound = (energy > config.energyThreshold || peakInWindow > config.silenceThreshold);

        if (config.enableDebug && windowsProcessed % 200 == 0) {
            DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                           huntmaster::DebugLevel::TRACE,
                                           "Backward window " + std::to_string(windowsProcessed)
                                               + " - Energy: " + std::to_string(energy)
                                               + ", Peak: " + std::to_string(peakInWindow)
                                               + ", Sound: " + (isSound ? "YES" : "NO"));
        }

        if (isSound) {
            // Found sound - add hangover time
            lastSoundFrame =
                std::min(samples.size(), static_cast<size_t>(i) + windowSize + hangoverSamples);

            if (config.enableDebug) {
                DebugLogger::getInstance().log(
                    huntmaster::DebugComponent::TOOLS,
                    huntmaster::DebugLevel::INFO,
                    "Audio end found at sample " + std::to_string(lastSoundFrame) + " ("
                        + std::to_string(lastSoundFrame / sampleRate) + "s)");
            }

            break;
        }

        windowsProcessed++;
    }

    if (config.enableDebug) {
        DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                       huntmaster::DebugLevel::DEBUG,
                                       "End detection completed after processing "
                                           + std::to_string(windowsProcessed) + " windows");
    }

    return lastSoundFrame;
}

// Trim silence from audio
std::vector<float>
trimSilence(const std::vector<float>& samples, float sampleRate, const VADConfig& config) {
    if (samples.empty()) {
        DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                       huntmaster::DebugLevel::WARN,
                                       "Cannot trim silence from empty sample array");
        return samples;
    }

    PerformanceMonitor monitor("Silence trimming", config.enableDebug);

    if (config.enableDebug) {
        DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                       huntmaster::DebugLevel::DEBUG,
                                       "Trimming silence from " + std::to_string(samples.size())
                                           + " samples");
        config.printConfig();
    }

    size_t start = findAudioStart(samples, sampleRate, config);
    size_t end = findAudioEnd(samples, sampleRate, config);

    // Ensure valid range
    if (start >= end || start >= samples.size()) {
        DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                       huntmaster::DebugLevel::WARN,
                                       "No significant audio detected! Start: "
                                           + std::to_string(start)
                                           + ", End: " + std::to_string(end));
        std::cout << "Warning: No significant audio detected!" << std::endl;
        return samples;  // Return original if no valid audio found
    }

    if (config.enableDebug) {
        DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                       huntmaster::DebugLevel::DEBUG,
                                       "Trimming audio from sample " + std::to_string(start)
                                           + " to " + std::to_string(end) + " ("
                                           + std::to_string((end - start) / sampleRate) + "s)");
        monitor.checkpoint("Audio boundaries identified");
    }

    // Extract trimmed audio
    std::vector<float> trimmed(samples.begin() + start, samples.begin() + end);

    // Apply fade in/out to avoid clicks
    int fadeLength = static_cast<int>(sampleRate * 0.005f);  // 5ms fade

    if (config.enableDebug) {
        DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                       huntmaster::DebugLevel::DEBUG,
                                       "Applying fade in/out with " + std::to_string(fadeLength)
                                           + " samples");
    }

    for (int i = 0; i < fadeLength && static_cast<size_t>(i) < trimmed.size(); ++i) {
        float factor = static_cast<float>(i) / fadeLength;
        trimmed[i] *= factor;
    }
    for (int i = 0; i < fadeLength && static_cast<size_t>(i) < trimmed.size(); ++i) {
        float factor = static_cast<float>(i) / fadeLength;
        trimmed[trimmed.size() - 1 - static_cast<size_t>(i)] *= factor;
    }

    if (config.enableDebug) {
        DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                       huntmaster::DebugLevel::INFO,
                                       "Trimming completed - Removed "
                                           + std::to_string(samples.size() - trimmed.size())
                                           + " samples");
        monitor.checkpoint("Fade applied");
    }

    return trimmed;
}

// Visualize audio with silence regions marked
void visualizeWithSilence(const std::vector<float>& samples,
                          float sampleRate,
                          const std::string& label,
                          bool enableDebug = false) {
    if (samples.empty()) {
        DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                       huntmaster::DebugLevel::WARN,
                                       "Cannot visualize empty sample array for: " + label);
        return;
    }

    PerformanceMonitor monitor("Silence visualization", enableDebug);

    const int width = 80;
    const int height = 10;

    if (enableDebug) {
        DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                       huntmaster::DebugLevel::DEBUG,
                                       "Visualizing " + label + " with "
                                           + std::to_string(samples.size()) + " samples");
    }

    std::cout << "\n" << label << std::endl;
    std::cout << std::string(width, '=') << std::endl;

    VADConfig config;
    config.enableDebug = enableDebug;

    size_t audioStart = findAudioStart(samples, sampleRate, config);
    size_t audioEnd = findAudioEnd(samples, sampleRate, config);

    if (enableDebug) {
        monitor.checkpoint("Audio boundaries found");
    }

    // Show timeline
    float duration = samples.size() / sampleRate;
    std::cout << "Duration: " << std::fixed << std::setprecision(3) << duration << "s" << std::endl;
    std::cout << "Audio region: " << (audioStart / sampleRate) << "s - " << (audioEnd / sampleRate)
              << "s" << std::endl;

    // Calculate trimmed duration
    float trimmedDuration = (audioEnd - audioStart) / sampleRate;
    float removedDuration = duration - trimmedDuration;

    if (enableDebug) {
        DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                       huntmaster::DebugLevel::DEBUG,
                                       "Duration analysis - Original: " + std::to_string(duration)
                                           + "s, Trimmed: " + std::to_string(trimmedDuration)
                                           + "s, Removed: " + std::to_string(removedDuration)
                                           + "s");
    }

    // Draw waveform with regions
    int samplesPerColumn = samples.size() / width;
    if (samplesPerColumn < 1)
        samplesPerColumn = 1;

    if (enableDebug) {
        DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                       huntmaster::DebugLevel::DEBUG,
                                       "Visualization parameters - Samples per column: "
                                           + std::to_string(samplesPerColumn));
    }

    // Find max amplitude
    float maxAmp = 0.0f;
    for (const auto& s : samples) {
        maxAmp = std::max(maxAmp, std::abs(s));
    }
    if (maxAmp == 0.0f)
        maxAmp = 1.0f;

    if (enableDebug) {
        DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                       huntmaster::DebugLevel::DEBUG,
                                       "Maximum amplitude: " + std::to_string(maxAmp));
        monitor.checkpoint("Amplitude analysis completed");
    }

    // Draw silence indicators
    std::cout << "Silence: ";
    int silenceColumns = 0;
    for (int col = 0; col < width; ++col) {
        size_t sampleIdx = col * samplesPerColumn;
        if (sampleIdx < audioStart || sampleIdx >= audioEnd) {
            std::cout << "S";
            silenceColumns++;
        } else {
            std::cout << " ";
        }
    }
    std::cout << std::endl;

    if (enableDebug) {
        DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                       huntmaster::DebugLevel::DEBUG,
                                       "Silence visualization - " + std::to_string(silenceColumns)
                                           + " out of " + std::to_string(width)
                                           + " columns are silence");
    }

    // Draw waveform
    for (int row = height; row >= -height; --row) {
        std::cout << "        |";

        for (int col = 0; col < width; ++col) {
            size_t sampleIdx = col * samplesPerColumn;

            // RMS for this column
            float rms = 0.0f;
            int count = 0;
            for (int i_inner = 0;
                 i_inner < samplesPerColumn && sampleIdx + i_inner < samples.size();
                 ++i_inner) {
                rms += samples[sampleIdx + i_inner] * samples[sampleIdx + i_inner];
                count++;
            }
            rms = (count > 0) ? std::sqrt(rms / count) : 0.0f;

            int ampHeight = static_cast<int>((rms / maxAmp) * height);

            // Color code: silence vs audio
            bool isSilence = (sampleIdx < audioStart || sampleIdx >= audioEnd);

            if (row == 0) {
                std::cout << "-";
            } else if (row > 0 && ampHeight >= row) {
                std::cout << (isSilence ? "." : "*");
            } else if (row < 0 && -ampHeight >= -row) {
                std::cout << (isSilence ? "." : "*");
            } else {
                std::cout << " ";
            }
        }

        std::cout << "|" << std::endl;
    }
    std::cout << std::string(width + 10, '-') << std::endl;

    if (enableDebug) {
        DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                       huntmaster::DebugLevel::DEBUG,
                                       "Visualization completed");
    }
}

// Load and process audio file
bool processAudioFile(const std::string& inputPath,
                      const std::string& outputPath,
                      VADConfig& config,
                      bool visualize = false,
                      bool enableDebug = false) {
    PerformanceMonitor monitor("Audio file processing", enableDebug);

    if (enableDebug) {
        DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                       huntmaster::DebugLevel::INFO,
                                       "Processing audio file: " + inputPath + " -> " + outputPath);
    }

    // Load audio
    unsigned int channels, sampleRate;
    drwav_uint64 totalFrames;
    float* pSampleData = drwav_open_file_and_read_pcm_frames_f32(
        inputPath.c_str(), &channels, &sampleRate, &totalFrames, nullptr);

    if (!pSampleData) {
        std::cerr << "Failed to load: " << inputPath << std::endl;
        DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                       huntmaster::DebugLevel::ERROR,
                                       "Failed to load audio file: " + inputPath);
        return false;
    }

    if (enableDebug) {
        DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                       huntmaster::DebugLevel::DEBUG,
                                       "Audio loaded - Channels: " + std::to_string(channels)
                                           + ", Sample Rate: " + std::to_string(sampleRate)
                                           + ", Total Frames: " + std::to_string(totalFrames));
        monitor.checkpoint("Audio file loaded");
    }

    // Convert to mono
    std::vector<float> samples(totalFrames);
    if (channels > 1) {
        if (enableDebug) {
            DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                           huntmaster::DebugLevel::DEBUG,
                                           "Converting " + std::to_string(channels)
                                               + " channels to mono");
        }

        for (drwav_uint64 i = 0; i < totalFrames; ++i) {
            float sum = 0.0f;
            for (unsigned int ch = 0; ch < channels; ++ch) {
                sum += pSampleData[i * channels + ch];
            }
            samples[i] = sum / channels;
        }

        if (enableDebug) {
            monitor.checkpoint("Channel conversion completed");
        }
    } else {
        if (enableDebug) {
            DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                           huntmaster::DebugLevel::DEBUG,
                                           "Audio is already mono");
        }

        samples.assign(pSampleData, pSampleData + totalFrames);
    }
    drwav_free(pSampleData, nullptr);

    // Visualize original
    if (visualize) {
        visualizeWithSilence(samples, sampleRate, "Original: " + inputPath, enableDebug);
    }

    // Configure VAD debug
    config.enableDebug = enableDebug;

    // Trim silence
    std::vector<float> trimmed = trimSilence(samples, sampleRate, config);

    if (enableDebug) {
        monitor.checkpoint("Silence trimming completed");
    }

    // Show results
    float originalDuration = samples.size() / static_cast<float>(sampleRate);
    float trimmedDuration = trimmed.size() / static_cast<float>(sampleRate);
    float removedDuration = originalDuration - trimmedDuration;
    float compressionRatio = (originalDuration > 0) ? (trimmedDuration / originalDuration) : 0.0f;

    std::cout << "\nTrimming results:" << std::endl;
    std::cout << "Original: " << std::fixed << std::setprecision(3) << originalDuration << "s ("
              << samples.size() << " samples)" << std::endl;
    std::cout << "Trimmed:  " << std::fixed << std::setprecision(3) << trimmedDuration << "s ("
              << trimmed.size() << " samples)" << std::endl;
    std::cout << "Removed:  " << std::fixed << std::setprecision(3) << removedDuration << "s ("
              << std::fixed << std::setprecision(1) << (100.0f * (1.0f - compressionRatio)) << "%)"
              << std::endl;

    if (enableDebug) {
        DebugLogger::getInstance().log(
            huntmaster::DebugComponent::TOOLS,
            huntmaster::DebugLevel::INFO,
            "Trimming analysis - Compression ratio: " + std::to_string(compressionRatio)
                + ", Size reduction: " + std::to_string(100.0f * (1.0f - compressionRatio)) + "%");
    }

    // Visualize trimmed
    if (visualize) {
        visualizeWithSilence(trimmed, sampleRate, "Trimmed: " + outputPath, enableDebug);
    }

    // Save trimmed audio
    drwav wav;
    drwav_data_format format;
    format.container = drwav_container_riff;
    format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
    format.channels = 1;
    format.sampleRate = sampleRate;
    format.bitsPerSample = 32;

    if (!drwav_init_file_write(&wav, outputPath.c_str(), &format, nullptr)) {
        std::cerr << "Failed to create output file: " << outputPath << std::endl;
        DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                       huntmaster::DebugLevel::ERROR,
                                       "Failed to create output file: " + outputPath);
        return false;
    }

    drwav_uint64 framesWritten = drwav_write_pcm_frames(&wav, trimmed.size(), trimmed.data());
    drwav_uninit(&wav);

    if (enableDebug) {
        DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                       huntmaster::DebugLevel::DEBUG,
                                       "Wrote " + std::to_string(framesWritten)
                                           + " frames to output file");
        monitor.checkpoint("Audio file saved");
    }

    std::cout << "Saved trimmed audio to: " << outputPath << std::endl;

    bool success = framesWritten > 0;

    if (enableDebug) {
        DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                       huntmaster::DebugLevel::INFO,
                                       std::string("Audio processing ")
                                           + (success ? "completed successfully" : "failed"));
    }

    return success;
}

int main(int argc, char* argv[]) {
    // Parse debug options
    DebugOptions debugOptions;
    debugOptions.parseArgs(argc, argv);

    if (debugOptions.printHelp) {
        debugOptions.printUsage(argv[0]);
        return 0;
    }

    // Set up debugging based on options
    if (debugOptions.enableTrace) {
        DebugConfig::setupFullDebug();
    } else if (debugOptions.enableDebug) {
        DebugConfig::setupToolsDebug();
    }

    // Configure component-specific debug levels
    auto& logger = DebugLogger::getInstance();
    if (debugOptions.enableVADDebug) {
        logger.setComponentLogLevel(huntmaster::DebugComponent::AUDIO_ENGINE,
                                    huntmaster::DebugLevel::DEBUG);
    }
    if (debugOptions.enableTrimDebug) {
        logger.setComponentLogLevel(huntmaster::DebugComponent::TOOLS,
                                    huntmaster::DebugLevel::TRACE);
    }
    if (debugOptions.enablePerformanceMetrics) {
        logger.setComponentLogLevel(huntmaster::DebugComponent::PERFORMANCE,
                                    huntmaster::DebugLevel::DEBUG);
    }

    DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                   huntmaster::DebugLevel::INFO,
                                   "=== Audio Trimming Tool Started ===");

    PerformanceMonitor totalMonitor("Total execution", debugOptions.enablePerformanceMetrics);

    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <input.wav> [output.wav] [options]" << std::endl;
        std::cout << "Use --help for detailed usage information." << std::endl;
        return 1;
    }

    VADConfig config;
    bool visualize = false;
    bool batchMode = false;

    // Parse arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-v") {
            visualize = true;
        } else if (arg == "-t" && i + 1 < argc) {
            config.silenceThreshold = std::stof(argv[++i]);

            if (debugOptions.enableDebug) {
                DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                               huntmaster::DebugLevel::DEBUG,
                                               "Silence threshold set to: "
                                                   + std::to_string(config.silenceThreshold));
            }
        } else if (arg == "-batch") {
            batchMode = true;
        }
    }

    if (debugOptions.enableDebug) {
        DebugLogger::getInstance().log(
            huntmaster::DebugComponent::TOOLS,
            huntmaster::DebugLevel::DEBUG,
            "Configuration - Visualize: " + std::string(visualize ? "true" : "false")
                + ", Batch mode: " + std::string(batchMode ? "true" : "false"));
    }

    bool allSuccessful = true;

    if (batchMode) {
        // Process all recordings
        std::cout << "\nBatch processing recordings..." << std::endl;

        if (debugOptions.enableBatchDebug) {
            DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                           huntmaster::DebugLevel::INFO,
                                           "Starting batch processing mode");
        }

        PerformanceMonitor batchMonitor("Batch processing", debugOptions.enableBatchDebug);

        std::vector<std::pair<std::string, std::string>> filesToProcess = {
            {"../data/recordings/user_attempt_buck_grunt.wav",
             "../data/recordings/user_attempt_buck_grunt_trimmed.wav"},
            {"../data/recordings/test_grunt.wav", "../data/recordings/test_grunt_trimmed.wav"}};

        // Process test recordings
        for (const auto& filePair : filesToProcess) {
            if (debugOptions.enableBatchDebug) {
                DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                               huntmaster::DebugLevel::DEBUG,
                                               "Processing: " + filePair.first + " -> "
                                                   + filePair.second);
            }

            if (std::filesystem::exists(filePair.first)) {
                bool success = processAudioFile(
                    filePair.first, filePair.second, config, visualize, debugOptions.enableDebug);
                if (!success) {
                    allSuccessful = false;
                    if (debugOptions.enableBatchDebug) {
                        DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                                       huntmaster::DebugLevel::ERROR,
                                                       "Failed to process: " + filePair.first);
                    }
                }
            } else {
                if (debugOptions.enableBatchDebug) {
                    DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                                   huntmaster::DebugLevel::WARN,
                                                   "File not found: " + filePair.first);
                }
            }
        }

        batchMonitor.checkpoint("Test recordings processed");

        // Process master calls
        std::vector<std::string> masters = {"buck_grunt", "doe-grunt", "buck-bawl"};
        for (const auto& master : masters) {
            std::string input = "../data/master_calls/" + master + ".wav";
            std::string output = "../data/master_calls/" + master + "_trimmed.wav";

            if (debugOptions.enableBatchDebug) {
                DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                               huntmaster::DebugLevel::DEBUG,
                                               "Processing master call: " + input + " -> "
                                                   + output);
            }

            if (std::filesystem::exists(input)) {
                bool success =
                    processAudioFile(input, output, config, visualize, debugOptions.enableDebug);
                if (!success) {
                    allSuccessful = false;
                    if (debugOptions.enableBatchDebug) {
                        DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                                       huntmaster::DebugLevel::ERROR,
                                                       "Failed to process master call: " + input);
                    }
                }
            } else {
                if (debugOptions.enableBatchDebug) {
                    DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                                   huntmaster::DebugLevel::WARN,
                                                   "Master call file not found: " + input);
                }
            }
        }

        batchMonitor.checkpoint("Master calls processed");

        if (debugOptions.enableBatchDebug) {
            DebugLogger::getInstance().log(
                huntmaster::DebugComponent::TOOLS,
                huntmaster::DebugLevel::INFO,
                "Batch processing completed - "
                    + std::string(allSuccessful ? "All successful" : "Some files failed"));
        }
    } else {
        // Single file mode
        std::string inputPath = argv[1];
        std::string outputPath = "trimmed_output.wav";

        // Find output path from arguments
        for (int i = 2; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg[0] != '-' && arg.find('=') == std::string::npos) {
                outputPath = arg;
                break;
            }
        }

        if (debugOptions.enableDebug) {
            DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                           huntmaster::DebugLevel::DEBUG,
                                           "Single file mode - Input: " + inputPath
                                               + ", Output: " + outputPath);
        }

        if (std::filesystem::exists(inputPath)) {
            allSuccessful = processAudioFile(
                inputPath, outputPath, config, visualize, debugOptions.enableDebug);
        } else {
            std::cerr << "Input file not found: " << inputPath << std::endl;
            DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                           huntmaster::DebugLevel::ERROR,
                                           "Input file not found: " + inputPath);
            allSuccessful = false;
        }
    }

    totalMonitor.checkpoint("Processing completed");

    DebugLogger::getInstance().log(
        huntmaster::DebugComponent::TOOLS,
        huntmaster::DebugLevel::INFO,
        "=== Audio Trimming Tool "
            + std::string(allSuccessful ? "Completed Successfully" : "Completed with Errors")
            + " ===");

    return allSuccessful ? 0 : 1;
}
