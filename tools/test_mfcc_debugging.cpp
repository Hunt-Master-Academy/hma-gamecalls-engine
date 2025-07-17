/**
 * @file test_mfcc_debugging.cpp
 * @brief Diagnostic tool to debug MFCC "0 features" issue
 *
 * This tool loads test audio and analyzes the MFCC feature extraction process
 * step by step to identify why we might be getting 0 features extracted.
 */

#define _USE_MATH_DEFINES
#include <huntmaster/core/DebugConfig.h>
#include <huntmaster/core/DebugLogger.h>
#include <huntmaster/core/MFCCProcessor.h>

#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>

using huntmaster::DebugConfig;
using huntmaster::DebugLogger;

// Debug options structure
struct DebugOptions {
    bool enableDebug = false;
    bool enableTrace = false;
    bool enableVerbose = false;
    bool enablePerformanceMetrics = false;
    bool enableMFCCDebug = false;
    bool enableWAVDebug = false;
    bool enableSynthDebug = false;
    bool enableFrameDebug = false;
    bool printHelp = false;

    void parseArgs(int argc, char* argv[]) {
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "--debug" || arg == "-d") {
                enableDebug = true;
            } else if (arg == "--trace" || arg == "-t") {
                enableTrace = true;
            } else if (arg == "--verbose" || arg == "-v") {
                enableVerbose = true;
            } else if (arg == "--performance" || arg == "-p") {
                enablePerformanceMetrics = true;
            } else if (arg == "--mfcc-debug") {
                enableMFCCDebug = true;
            } else if (arg == "--wav-debug") {
                enableWAVDebug = true;
            } else if (arg == "--synth-debug") {
                enableSynthDebug = true;
            } else if (arg == "--frame-debug") {
                enableFrameDebug = true;
            } else if (arg == "--help" || arg == "-h") {
                printHelp = true;
            }
        }
    }

    void printUsage(const char* programName) {
        std::cout << "=== MFCC Debugging Tool ===" << std::endl;
        std::cout << "Usage: " << programName << " [options] [wav_files...]" << std::endl;
        std::cout << std::endl;
        std::cout << "Arguments:" << std::endl;
        std::cout << "  wav_files        Specific WAV files to test (optional)" << std::endl;
        std::cout << std::endl;
        std::cout << "Options:" << std::endl;
        std::cout << "  --debug, -d      Enable debug logging" << std::endl;
        std::cout << "  --trace, -t      Enable trace logging" << std::endl;
        std::cout << "  --verbose, -v    Enable verbose output" << std::endl;
        std::cout << "  --performance, -p Enable performance metrics" << std::endl;
        std::cout << "  --mfcc-debug     Enable MFCC processing debugging" << std::endl;
        std::cout << "  --wav-debug      Enable WAV file loading debugging" << std::endl;
        std::cout << "  --synth-debug    Enable synthetic audio debugging" << std::endl;
        std::cout << "  --frame-debug    Enable frame-by-frame debugging" << std::endl;
        std::cout << "  --help, -h       Show this help message" << std::endl;
        std::cout << std::endl;
        std::cout << "Examples:" << std::endl;
        std::cout << "  " << programName << "                           # Test default files"
                  << std::endl;
        std::cout << "  " << programName << " --debug --mfcc-debug      # Debug MFCC processing"
                  << std::endl;
        std::cout << "  " << programName << " --frame-debug test.wav    # Debug specific file"
                  << std::endl;
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

            DebugLogger::getInstance().log(
                huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::INFO,
                operationName + " completed in " + std::to_string(duration.count()) + "ms");
        }
    }

    void checkpoint(const std::string& message) {
        if (enabled) {
            auto currentTime = std::chrono::high_resolution_clock::now();
            auto duration =
                std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime);

            DebugLogger::getInstance().log(
                huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::DEBUG,
                operationName + " - " + message + " (+" + std::to_string(duration.count()) + "ms)");
        }
    }
};

// Simple WAV reader for testing (minimal implementation)
struct WAVData {
    std::vector<float> samples;
    uint32_t sampleRate;
    uint16_t channels;
    bool enableDebug;

    WAVData(bool debug = false) : enableDebug(debug) {}

    bool loadFromFile(const std::string& filename) {
        PerformanceMonitor monitor("WAV file loading: " + filename, enableDebug);

        if (enableDebug) {
            DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                           huntmaster::DebugLevel::DEBUG,
                                           "Loading WAV file: " + filename);
        }

        std::ifstream file(filename, std::ios::binary);
        if (!file) {
            std::cerr << "Could not open file: " << filename << std::endl;
            DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                           huntmaster::DebugLevel::ERROR,
                                           "Could not open file: " + filename);
            return false;
        }

        // Read WAV header (simplified - assumes PCM format)
        char header[44];
        file.read(header, 44);

        if (std::string(header, 4) != "RIFF" || std::string(header + 8, 4) != "WAVE") {
            std::cerr << "Not a valid WAV file" << std::endl;
            DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                           huntmaster::DebugLevel::ERROR,
                                           "Not a valid WAV file: " + filename);
            return false;
        }

        // Extract sample rate and channels from header
        sampleRate = *reinterpret_cast<uint32_t*>(header + 24);
        channels = *reinterpret_cast<uint16_t*>(header + 22);
        uint16_t bitsPerSample = *reinterpret_cast<uint16_t*>(header + 34);

        if (enableDebug) {
            DebugLogger::getInstance().log(
                huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::DEBUG,
                "WAV properties - Sample Rate: " + std::to_string(sampleRate) +
                    " Hz, Channels: " + std::to_string(channels) +
                    ", Bits per sample: " + std::to_string(bitsPerSample));
            monitor.checkpoint("WAV header parsed");
        }

        std::cout << "WAV Info: " << sampleRate << " Hz, " << channels << " channels, "
                  << bitsPerSample << " bits" << std::endl;

        // Read sample data
        file.seekg(0, std::ios::end);
        size_t fileSize = file.tellg();
        size_t dataSize = fileSize - 44;  // Subtract header size

        if (enableDebug) {
            DebugLogger::getInstance().log(
                huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::DEBUG,
                "File size: " + std::to_string(fileSize) +
                    " bytes, Data size: " + std::to_string(dataSize) + " bytes");
        }

        if (bitsPerSample == 16) {
            std::vector<int16_t> rawSamples(dataSize / 2);
            file.seekg(44);
            file.read(reinterpret_cast<char*>(rawSamples.data()), dataSize);

            if (enableDebug) {
                DebugLogger::getInstance().log(
                    huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::DEBUG,
                    "Read " + std::to_string(rawSamples.size()) + " raw samples");
                monitor.checkpoint("Raw samples read");
            }

            // Convert to float and take only first channel if stereo
            samples.reserve(rawSamples.size() / channels);
            for (size_t i = 0; i < rawSamples.size(); i += channels) {
                samples.push_back(static_cast<float>(rawSamples[i]) / 32768.0f);
            }

            if (enableDebug) {
                DebugLogger::getInstance().log(
                    huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::DEBUG,
                    "Converted to " + std::to_string(samples.size()) + " float samples");
                monitor.checkpoint("Sample conversion completed");
            }
        } else {
            std::cerr << "Unsupported bit depth: " << bitsPerSample << std::endl;
            DebugLogger::getInstance().log(
                huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::ERROR,
                "Unsupported bit depth: " + std::to_string(bitsPerSample));
            return false;
        }

        // Calculate some basic statistics
        if (enableDebug && !samples.empty()) {
            float minVal = *std::min_element(samples.begin(), samples.end());
            float maxVal = *std::max_element(samples.begin(), samples.end());
            float avgVal = 0.0f;
            for (float sample : samples) {
                avgVal += std::abs(sample);
            }
            avgVal /= samples.size();

            DebugLogger::getInstance().log(
                huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::DEBUG,
                "Sample statistics - Min: " + std::to_string(minVal) + ", Max: " +
                    std::to_string(maxVal) + ", Avg magnitude: " + std::to_string(avgVal));
        }

        std::cout << "Loaded " << samples.size() << " samples" << std::endl;
        return true;
    }
};

// MFCC debugging class
class MFCCDebugger {
   private:
    DebugOptions& options;

   public:
    MFCCDebugger(DebugOptions& opts) : options(opts) {}

    void testAudioFile(const std::string& filename) {
        PerformanceMonitor monitor("Testing audio file: " + filename,
                                   options.enablePerformanceMetrics);

        std::cout << "\n--- Testing with: " << filename << " ---" << std::endl;

        if (options.enableMFCCDebug) {
            DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                           huntmaster::DebugLevel::DEBUG,
                                           "Starting MFCC test for: " + filename);
        }

        WAVData audio(options.enableWAVDebug);
        if (!audio.loadFromFile(filename)) {
            std::cout << "❌ Failed to load " << filename << std::endl;
            return;
        }

        monitor.checkpoint("Audio file loaded");

        // Create MFCC processor with standard config
        huntmaster::MFCCProcessor::Config config;
        config.sample_rate = static_cast<float>(audio.sampleRate);
        config.frame_size = 512;       // Standard frame size
        config.num_filters = 26;       // Standard mel filters
        config.num_coefficients = 13;  // Standard MFCC coefficients
        config.low_freq = 0.0f;
        config.high_freq = 0.0f;  // Will be set to Nyquist by processor

        if (options.enableMFCCDebug) {
            DebugLogger::getInstance().log(
                huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::DEBUG,
                "MFCC Config - Sample Rate: " + std::to_string(config.sample_rate) +
                    ", Frame Size: " + std::to_string(config.frame_size) +
                    ", Filters: " + std::to_string(config.num_filters) +
                    ", Coefficients: " + std::to_string(config.num_coefficients));
        }

        std::cout << "MFCC Config:" << std::endl;
        std::cout << "  Sample Rate: " << config.sample_rate << " Hz" << std::endl;
        std::cout << "  Frame Size: " << config.frame_size << " samples" << std::endl;
        std::cout << "  Num Filters: " << config.num_filters << std::endl;
        std::cout << "  Num Coefficients: " << config.num_coefficients << std::endl;

        huntmaster::MFCCProcessor processor(config);
        monitor.checkpoint("MFCC processor created");

        // Test single frame extraction
        if (audio.samples.size() >= config.frame_size) {
            testSingleFrame(processor, audio, config);
        } else {
            std::cout << "❌ Audio too short for single frame test" << std::endl;
            if (options.enableMFCCDebug) {
                DebugLogger::getInstance().log(
                    huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::WARN,
                    "Audio too short - Size: " + std::to_string(audio.samples.size()) +
                        ", Required: " + std::to_string(config.frame_size));
            }
        }

        // Test buffer extraction with various hop sizes
        std::vector<size_t> hopSizes = {256, 128, 64};
        for (size_t hopSize : hopSizes) {
            testBufferExtraction(processor, audio, hopSize, config);
        }
    }

   private:
    void testSingleFrame(huntmaster::MFCCProcessor& processor, const WAVData& audio,
                         const huntmaster::MFCCProcessor::Config& config) {
        PerformanceMonitor monitor("Single frame test", options.enableFrameDebug);

        std::span<const float> frame(audio.samples.data(), config.frame_size);

        if (options.enableFrameDebug) {
            DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                           huntmaster::DebugLevel::DEBUG,
                                           "Testing single frame extraction with " +
                                               std::to_string(config.frame_size) + " samples");
        }

        std::cout << "Testing single frame extraction..." << std::endl;
        auto result = processor.extractFeatures(frame);

        if (result) {
            const auto& features = *result;
            std::cout << "✅ Single frame: " << features.size() << " features extracted"
                      << std::endl;

            if (options.enableFrameDebug) {
                DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                               huntmaster::DebugLevel::INFO,
                                               "Single frame extraction successful - " +
                                                   std::to_string(features.size()) + " features");
            }

            // Print first few coefficients
            std::cout << "  First 5 coefficients: ";
            for (size_t i = 0; i < std::min(5UL, features.size()); ++i) {
                std::cout << features[i] << " ";
            }
            std::cout << std::endl;

            // Calculate energy
            float energy = 0.0f;
            for (float coeff : features) {
                energy += coeff * coeff;
            }
            energy = std::sqrt(energy);

            if (options.enableFrameDebug) {
                DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                               huntmaster::DebugLevel::DEBUG,
                                               "Frame energy: " + std::to_string(energy));
            }

            std::cout << "  Frame energy: " << energy << std::endl;
        } else {
            std::cout << "❌ Single frame extraction failed" << std::endl;
            if (options.enableFrameDebug) {
                DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                               huntmaster::DebugLevel::ERROR,
                                               "Single frame extraction failed");
            }
        }
    }

    void testBufferExtraction(huntmaster::MFCCProcessor& processor, const WAVData& audio,
                              size_t hopSize, const huntmaster::MFCCProcessor::Config& config) {
        PerformanceMonitor monitor("Buffer extraction (hop=" + std::to_string(hopSize) + ")",
                                   options.enablePerformanceMetrics);

        std::cout << "Testing buffer extraction with hop size " << hopSize << "..." << std::endl;

        if (options.enableMFCCDebug) {
            DebugLogger::getInstance().log(
                huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::DEBUG,
                "Testing buffer extraction - Audio size: " + std::to_string(audio.samples.size()) +
                    ", Hop size: " + std::to_string(hopSize) +
                    ", Frame size: " + std::to_string(config.frame_size));
        }

        auto bufferResult = processor.extractFeaturesFromBuffer(audio.samples, hopSize);

        if (bufferResult) {
            const auto& features = *bufferResult;
            std::cout << "✅ Buffer extraction: " << features.size() << " frames extracted"
                      << std::endl;

            if (options.enableMFCCDebug) {
                DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                               huntmaster::DebugLevel::INFO,
                                               "Buffer extraction successful - " +
                                                   std::to_string(features.size()) + " frames");
            }

            if (!features.empty()) {
                std::cout << "  Each frame has " << features[0].size() << " coefficients"
                          << std::endl;

                // Calculate total features
                size_t totalFeatures =
                    features.size() * (features.empty() ? 0 : features[0].size());
                std::cout << "  Total feature count: " << totalFeatures << std::endl;

                if (options.enableMFCCDebug) {
                    DebugLogger::getInstance().log(
                        huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::DEBUG,
                        "Total features: " + std::to_string(totalFeatures));
                }

                if (totalFeatures == 0) {
                    std::cout << "❌ FOUND THE ISSUE: 0 total features extracted!" << std::endl;
                    if (options.enableMFCCDebug) {
                        DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                                       huntmaster::DebugLevel::ERROR,
                                                       "CRITICAL: 0 total features extracted");
                    }
                }

                // Show energy distribution
                if (options.enableFrameDebug && features.size() > 0) {
                    std::cout << "  Energy in first 3 frames:" << std::endl;
                    for (size_t i = 0; i < std::min(3UL, features.size()); ++i) {
                        float energy = 0.0f;
                        for (float coeff : features[i]) {
                            energy += coeff * coeff;
                        }
                        energy = std::sqrt(energy);
                        std::cout << "    Frame " << i << ": " << energy << std::endl;

                        if (options.enableFrameDebug) {
                            DebugLogger::getInstance().log(
                                huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::TRACE,
                                "Frame " + std::to_string(i) +
                                    " energy: " + std::to_string(energy));
                        }
                    }
                }
            } else {
                std::cout << "❌ FOUND THE ISSUE: No frames extracted from buffer!" << std::endl;

                if (options.enableMFCCDebug) {
                    DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                                   huntmaster::DebugLevel::ERROR,
                                                   "CRITICAL: No frames extracted from buffer");
                }

                // Debug why no frames were extracted
                std::cout << "  Audio buffer size: " << audio.samples.size() << " samples"
                          << std::endl;
                std::cout << "  Frame size: " << config.frame_size << " samples" << std::endl;
                std::cout << "  Expected frames: " << audio.samples.size() / hopSize << std::endl;

                if (audio.samples.size() < config.frame_size) {
                    std::cout << "  ❌ Audio buffer too short for even one frame!" << std::endl;
                    if (options.enableMFCCDebug) {
                        DebugLogger::getInstance().log(
                            huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::ERROR,
                            "Audio buffer too short - Size: " +
                                std::to_string(audio.samples.size()) +
                                ", Required: " + std::to_string(config.frame_size));
                    }
                }
            }
        } else {
            std::cout << "❌ Buffer extraction failed with error" << std::endl;
            if (options.enableMFCCDebug) {
                DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                               huntmaster::DebugLevel::ERROR,
                                               "Buffer extraction failed with error");
            }
        }
    }
};

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
    if (debugOptions.enableMFCCDebug) {
        logger.setComponentLogLevel(huntmaster::DebugComponent::FEATURE_EXTRACTION,
                                 huntmaster::DebugLevel::DEBUG);
    }
    if (debugOptions.enableFrameDebug) {
        logger.setComponentLogLevel(huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::TRACE);
    }
    if (debugOptions.enablePerformanceMetrics) {
        logger.setComponentLogLevel(huntmaster::DebugComponent::PERFORMANCE,
                                 huntmaster::DebugLevel::DEBUG);
    }

    DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::INFO,
                                   "=== MFCC Debugging Tool Started ===");

    PerformanceMonitor totalMonitor("Total execution", debugOptions.enablePerformanceMetrics);

    std::cout << "=== MFCC Debugging Tool ===" << std::endl;

    try {
        MFCCDebugger debugger(debugOptions);

        // Determine which files to test
        std::vector<std::string> testFiles;

        // Check if specific files were provided as arguments
        bool foundFiles = false;
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
            if (arg[0] != '-') {  // Not a debug option
                testFiles.push_back(arg);
                foundFiles = true;
            }
        }

        // If no specific files provided, use default list
        if (!foundFiles) {
            testFiles = {"data/test_audio/test_sine_440.wav", "data/test_audio/test_complex.wav"};
        }

        // Test with available audio files
        for (const auto& filename : testFiles) {
            debugger.testAudioFile(filename);
        }

        totalMonitor.checkpoint("Audio file tests completed");

        // Test with synthetic data to isolate the issue
        std::cout << "\n--- Testing with synthetic sine wave ---" << std::endl;

        if (debugOptions.enableSynthDebug) {
            DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                           huntmaster::DebugLevel::DEBUG,
                                           "Starting synthetic sine wave test");
        }

        PerformanceMonitor synthMonitor("Synthetic audio test", debugOptions.enableSynthDebug);

        // Generate a 1-second 440 Hz sine wave at 44.1 kHz
        size_t sampleRate = 44100;
        size_t duration = 1;  // seconds
        std::vector<float> sineWave(sampleRate * duration);

        for (size_t i = 0; i < sineWave.size(); ++i) {
            float t = static_cast<float>(i) / sampleRate;
            sineWave[i] = 0.5f * sin(2.0f * M_PI * 440.0f * t);
        }

        if (debugOptions.enableSynthDebug) {
            DebugLogger::getInstance().log(
                huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::DEBUG,
                "Generated sine wave - " + std::to_string(sineWave.size()) + " samples at " +
                    std::to_string(sampleRate) + " Hz");
            synthMonitor.checkpoint("Sine wave generated");
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
        synthMonitor.checkpoint("Synthetic MFCC processor created");

        auto synthResult = synthProcessor.extractFeaturesFromBuffer(sineWave, 256);

        if (synthResult) {
            const auto& features = *synthResult;
            std::cout << "✅ Synthetic audio: " << features.size() << " frames extracted"
                      << std::endl;

            if (debugOptions.enableSynthDebug) {
                DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                               huntmaster::DebugLevel::INFO,
                                               "Synthetic audio extraction successful - " +
                                                   std::to_string(features.size()) + " frames");
            }

            if (!features.empty()) {
                std::cout << "  Each frame has " << features[0].size() << " coefficients"
                          << std::endl;
                size_t totalFeatures = features.size() * features[0].size();
                std::cout << "  Total feature count: " << totalFeatures << std::endl;

                if (debugOptions.enableSynthDebug) {
                    DebugLogger::getInstance().log(
                        huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::DEBUG,
                        "Synthetic audio total features: " + std::to_string(totalFeatures));
                }

                // Show energy in first few frames
                std::cout << "  Energy in first 3 frames:" << std::endl;
                for (size_t i = 0; i < std::min(3UL, features.size()); ++i) {
                    float energy = 0.0f;
                    for (float coeff : features[i]) {
                        energy += coeff * coeff;
                    }
                    energy = std::sqrt(energy);
                    std::cout << "    Frame " << i << ": " << energy << std::endl;

                    if (debugOptions.enableSynthDebug) {
                        DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                                       huntmaster::DebugLevel::TRACE,
                                                       "Synthetic frame " + std::to_string(i) +
                                                           " energy: " + std::to_string(energy));
                    }
                }
            }
        } else {
            std::cout << "❌ Synthetic audio extraction failed" << std::endl;
            if (debugOptions.enableSynthDebug) {
                DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                               huntmaster::DebugLevel::ERROR,
                                               "Synthetic audio extraction failed");
            }
        }

        synthMonitor.checkpoint("Synthetic audio processing completed");

        std::cout << "\n=== MFCC Debugging Complete ===" << std::endl;

        DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                       huntmaster::DebugLevel::INFO,
                                       "=== MFCC Debugging Tool Completed Successfully ===");

    } catch (const std::exception& e) {
        std::cerr << "❌ Exception: " << e.what() << std::endl;
        DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                       huntmaster::DebugLevel::ERROR,
                                       "Exception occurred: " + std::string(e.what()));
        return 1;
    }

    return 0;
}
