/**
 * @file test_mfcc_debugging.cpp
 * @brief Diagnostic tool to debug MFCC "0 features" issue
 *
 * This tool loads test audio and analyzes the MFCC feature extraction process
 * step by step to identify why we might be getting 0 features extracted.
 */

#define _USE_MATH_DEFINES
#include <huntmaster/core/MFCCProcessor.h>

#include <cmath>
#include <fstream>
#include <iostream>
#include <vector>

// Simple WAV reader for testing (minimal implementation)
struct WAVData {
    std::vector<float> samples;
    uint32_t sampleRate;
    uint16_t channels;

    bool loadFromFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file) {
            std::cerr << "Could not open file: " << filename << std::endl;
            return false;
        }

        // Read WAV header (simplified - assumes PCM format)
        char header[44];
        file.read(header, 44);

        if (std::string(header, 4) != "RIFF" || std::string(header + 8, 4) != "WAVE") {
            std::cerr << "Not a valid WAV file" << std::endl;
            return false;
        }

        // Extract sample rate and channels from header
        sampleRate = *reinterpret_cast<uint32_t*>(header + 24);
        channels = *reinterpret_cast<uint16_t*>(header + 22);
        uint16_t bitsPerSample = *reinterpret_cast<uint16_t*>(header + 34);

        std::cout << "WAV Info: " << sampleRate << " Hz, " << channels << " channels, "
                  << bitsPerSample << " bits" << std::endl;

        // Read sample data
        file.seekg(0, std::ios::end);
        size_t fileSize = file.tellg();
        size_t dataSize = fileSize - 44;  // Subtract header size

        if (bitsPerSample == 16) {
            std::vector<int16_t> rawSamples(dataSize / 2);
            file.seekg(44);
            file.read(reinterpret_cast<char*>(rawSamples.data()), dataSize);

            // Convert to float and take only first channel if stereo
            samples.reserve(rawSamples.size() / channels);
            for (size_t i = 0; i < rawSamples.size(); i += channels) {
                samples.push_back(static_cast<float>(rawSamples[i]) / 32768.0f);
            }
        } else {
            std::cerr << "Unsupported bit depth: " << bitsPerSample << std::endl;
            return false;
        }

        std::cout << "Loaded " << samples.size() << " samples" << std::endl;
        return true;
    }
};

int main() {
    std::cout << "=== MFCC Debugging Tool ===" << std::endl;

    try {
        // Test with available audio files
        std::vector<std::string> testFiles = {"data/test_audio/test_sine_440.wav",
                                              "data/test_audio/test_complex.wav"};

        for (const auto& filename : testFiles) {
            std::cout << "\n--- Testing with: " << filename << " ---" << std::endl;

            WAVData audio;
            if (!audio.loadFromFile(filename)) {
                std::cout << "❌ Failed to load " << filename << std::endl;
                continue;
            }

            // Create MFCC processor with standard config
            huntmaster::MFCCProcessor::Config config;
            config.sample_rate = static_cast<float>(audio.sampleRate);
            config.frame_size = 512;       // Standard frame size
            config.num_filters = 26;       // Standard mel filters
            config.num_coefficients = 13;  // Standard MFCC coefficients
            config.low_freq = 0.0f;
            config.high_freq = 0.0f;  // Will be set to Nyquist by processor

            std::cout << "MFCC Config:" << std::endl;
            std::cout << "  Sample Rate: " << config.sample_rate << " Hz" << std::endl;
            std::cout << "  Frame Size: " << config.frame_size << " samples" << std::endl;
            std::cout << "  Num Filters: " << config.num_filters << std::endl;
            std::cout << "  Num Coefficients: " << config.num_coefficients << std::endl;

            huntmaster::MFCCProcessor processor(config);

            // Test single frame extraction
            if (audio.samples.size() >= config.frame_size) {
                std::span<const float> frame(audio.samples.data(), config.frame_size);

                std::cout << "Testing single frame extraction..." << std::endl;
                auto result = processor.extractFeatures(frame);

                if (result) {
                    const auto& features = *result;
                    std::cout << "✅ Single frame: " << features.size() << " features extracted"
                              << std::endl;

                    // Print first few coefficients
                    std::cout << "  First 5 coefficients: ";
                    for (size_t i = 0; i < std::min(5UL, features.size()); ++i) {
                        std::cout << features[i] << " ";
                    }
                    std::cout << std::endl;
                } else {
                    std::cout << "❌ Single frame extraction failed" << std::endl;
                }
            }

            // Test buffer extraction with various hop sizes
            std::vector<size_t> hopSizes = {256, 128, 64};

            for (size_t hopSize : hopSizes) {
                std::cout << "Testing buffer extraction with hop size " << hopSize << "..."
                          << std::endl;

                auto bufferResult = processor.extractFeaturesFromBuffer(audio.samples, hopSize);

                if (bufferResult) {
                    const auto& features = *bufferResult;
                    std::cout << "✅ Buffer extraction: " << features.size() << " frames extracted"
                              << std::endl;

                    if (!features.empty()) {
                        std::cout << "  Each frame has " << features[0].size() << " coefficients"
                                  << std::endl;

                        // Calculate total features
                        size_t totalFeatures =
                            features.size() * (features.empty() ? 0 : features[0].size());
                        std::cout << "  Total feature count: " << totalFeatures << std::endl;

                        if (totalFeatures == 0) {
                            std::cout << "❌ FOUND THE ISSUE: 0 total features extracted!"
                                      << std::endl;
                        }
                    } else {
                        std::cout << "❌ FOUND THE ISSUE: No frames extracted from buffer!"
                                  << std::endl;

                        // Debug why no frames were extracted
                        std::cout << "  Audio buffer size: " << audio.samples.size() << " samples"
                                  << std::endl;
                        std::cout << "  Frame size: " << config.frame_size << " samples"
                                  << std::endl;
                        std::cout << "  Expected frames: " << audio.samples.size() / hopSize
                                  << std::endl;

                        if (audio.samples.size() < config.frame_size) {
                            std::cout << "  ❌ Audio buffer too short for even one frame!"
                                      << std::endl;
                        }
                    }
                } else {
                    std::cout << "❌ Buffer extraction failed with error" << std::endl;
                }
            }
        }

        // Test with synthetic data to isolate the issue
        std::cout << "\n--- Testing with synthetic sine wave ---" << std::endl;

        // Generate a 1-second 440 Hz sine wave at 44.1 kHz
        size_t sampleRate = 44100;
        size_t duration = 1;  // seconds
        std::vector<float> sineWave(sampleRate * duration);

        for (size_t i = 0; i < sineWave.size(); ++i) {
            float t = static_cast<float>(i) / sampleRate;
            sineWave[i] = 0.5f * sin(2.0f * M_PI * 440.0f * t);
        }

        std::cout << "Generated " << sineWave.size() << " samples of 440 Hz sine wave" << std::endl;

        huntmaster::MFCCProcessor::Config synthConfig;
        synthConfig.sample_rate = static_cast<float>(sampleRate);
        synthConfig.frame_size = 512;
        synthConfig.num_filters = 26;
        synthConfig.num_coefficients = 13;
        synthConfig.low_freq = 0.0f;
        synthConfig.high_freq = 0.0f;

        huntmaster::MFCCProcessor synthProcessor(synthConfig);

        auto synthResult = synthProcessor.extractFeaturesFromBuffer(sineWave, 256);

        if (synthResult) {
            const auto& features = *synthResult;
            std::cout << "✅ Synthetic audio: " << features.size() << " frames extracted"
                      << std::endl;

            if (!features.empty()) {
                std::cout << "  Each frame has " << features[0].size() << " coefficients"
                          << std::endl;
                size_t totalFeatures = features.size() * features[0].size();
                std::cout << "  Total feature count: " << totalFeatures << std::endl;

                // Show energy in first few frames
                std::cout << "  Energy in first 3 frames:" << std::endl;
                for (size_t i = 0; i < std::min(3UL, features.size()); ++i) {
                    float energy = 0.0f;
                    for (float coeff : features[i]) {
                        energy += coeff * coeff;
                    }
                    std::cout << "    Frame " << i << ": " << sqrt(energy) << std::endl;
                }
            }
        } else {
            std::cout << "❌ Synthetic audio extraction failed" << std::endl;
        }

        std::cout << "\n=== MFCC Debugging Complete ===" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "❌ Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
