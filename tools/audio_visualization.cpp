#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

#include "dr_wav.h"
#include "huntmaster/core/DebugConfig.h"
#include "huntmaster/core/DebugLogger.h"
#include "huntmaster/core/HuntmasterAudioEngine.h"

using huntmaster::DebugConfig;
using huntmaster::DebugLogger;
using huntmaster::HuntmasterAudioEngine;

// Debug options structure
struct DebugOptions {
    bool enableDebug = false;
    bool enableTrace = false;
    bool enableVerbose = false;
    bool enablePerformanceMetrics = false;
    bool enableVisualizationDebug = false;
    bool enableAudioAnalysis = false;
    bool enableComparisonDebug = false;
    bool enableExportDebug = false;
    bool printHelp = false;

    void parseArgs(int argc, char *argv[]) {
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
            } else if (arg == "--viz-debug") {
                enableVisualizationDebug = true;
            } else if (arg == "--audio-debug") {
                enableAudioAnalysis = true;
            } else if (arg == "--comparison-debug") {
                enableComparisonDebug = true;
            } else if (arg == "--export-debug") {
                enableExportDebug = true;
            } else if (arg == "--help" || arg == "-h") {
                printHelp = true;
            }
        }
    }

    void printUsage(const char *programName) {
        std::cout << "=== Huntmaster Audio Visualization Tool ===" << std::endl;
        std::cout << "Usage: " << programName
                  << " <master_call_name> <user_recording.wav> [options]" << std::endl;
        std::cout << std::endl;
        std::cout << "Arguments:" << std::endl;
        std::cout << "  master_call_name     Name of the master call (without .wav extension)"
                  << std::endl;
        std::cout << "  user_recording.wav   Path to user recording file" << std::endl;
        std::cout << std::endl;
        std::cout << "Debug Options:" << std::endl;
        std::cout << "  --debug, -d          Enable debug logging" << std::endl;
        std::cout << "  --trace, -t          Enable trace logging" << std::endl;
        std::cout << "  --verbose, -v        Enable verbose output" << std::endl;
        std::cout << "  --performance, -p    Enable performance metrics" << std::endl;
        std::cout << "  --viz-debug          Enable visualization debugging" << std::endl;
        std::cout << "  --audio-debug        Enable audio analysis debugging" << std::endl;
        std::cout << "  --comparison-debug   Enable comparison debugging" << std::endl;
        std::cout << "  --export-debug       Enable export debugging" << std::endl;
        std::cout << "  --help, -h           Show this help message" << std::endl;
        std::cout << std::endl;
        std::cout << "Example: " << programName
                  << " buck_grunt ../data/recordings/user_attempt.wav --debug --performance"
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
    PerformanceMonitor(const std::string &name, bool enable = true)
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

    void checkpoint(const std::string &message) {
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

std::vector<float> loadAudioFile(const std::string &filePath, unsigned int &channels,
                                 unsigned int &sampleRate) {
    PerformanceMonitor monitor("Audio file loading", true);

    DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::INFO,
                                   "Loading audio file: " + filePath);

    drwav_uint64 totalPCMFrameCount = 0;
    float *pSampleData = drwav_open_file_and_read_pcm_frames_f32(
        filePath.c_str(), &channels, &sampleRate, &totalPCMFrameCount, nullptr);

    if (pSampleData == nullptr) {
        DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                       huntmaster::DebugLevel::ERROR,
                                       "Failed to load audio file: " + filePath);
        return {};
    }

    DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::DEBUG,
                                   "Audio file loaded - Channels: " + std::to_string(channels) +
                                       ", Sample Rate: " + std::to_string(sampleRate) +
                                       ", Frames: " + std::to_string(totalPCMFrameCount));

    // Convert to mono if needed
    std::vector<float> monoSamples(totalPCMFrameCount);
    if (channels > 1) {
        DebugLogger::getInstance().log(
            huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::DEBUG,
            "Converting " + std::to_string(channels) + " channels to mono");
        monitor.checkpoint("Starting channel conversion");

        for (drwav_uint64 i = 0; i < totalPCMFrameCount; ++i) {
            float monoSample = 0.0f;
            for (unsigned int j = 0; j < channels; ++j) {
                monoSample += pSampleData[i * channels + j];
            }
            monoSamples[i] = monoSample / channels;
        }

        monitor.checkpoint("Channel conversion completed");
    } else {
        DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                       huntmaster::DebugLevel::DEBUG,
                                       "Audio is already mono, copying samples");
        monoSamples.assign(pSampleData, pSampleData + totalPCMFrameCount);
    }

    drwav_free(pSampleData, nullptr);

    DebugLogger::getInstance().log(
        huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::INFO,
        "Audio file processing completed - " + std::to_string(monoSamples.size()) + " samples");

    return monoSamples;
}

// Calculate RMS energy for a window
float calculateRMS(const std::vector<float> &samples, size_t start, size_t windowSize) {
    float sum = 0.0f;
    size_t end = std::min(start + windowSize, samples.size());

    for (size_t i = start; i < end; ++i) {
        sum += samples[i] * samples[i];
    }

    return std::sqrt(sum / (end - start));
}

// Generate ASCII waveform visualization
void visualizeWaveform(const std::vector<float> &samples, const std::string &label, int width = 80,
                       bool enableDebug = false) {
    if (samples.empty()) {
        DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                       huntmaster::DebugLevel::WARN,
                                       "Cannot visualize empty sample array for: " + label);
        return;
    }

    PerformanceMonitor monitor("Waveform visualization", enableDebug);

    if (enableDebug) {
        DebugLogger::getInstance().log(
            huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::DEBUG,
            "Visualizing waveform: " + label + " (" + std::to_string(samples.size()) +
                " samples, width=" + std::to_string(width) + ")");
    }

    std::cout << "\n" << label << " (" << samples.size() << " samples)" << std::endl;
    std::cout << std::string(width, '-') << std::endl;

    // Downsample to fit width
    int samplesPerColumn = samples.size() / width;
    if (samplesPerColumn < 1) samplesPerColumn = 1;

    if (enableDebug) {
        DebugLogger::getInstance().log(
            huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::DEBUG,
            "Downsampling: " + std::to_string(samplesPerColumn) + " samples per column");
    }

    // Find max amplitude for scaling
    float maxAmp = 0.0f;
    for (const auto &s : samples) {
        maxAmp = std::max(maxAmp, std::abs(s));
    }

    if (maxAmp == 0.0f) {
        maxAmp = 1.0f;
        if (enableDebug) {
            DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                           huntmaster::DebugLevel::WARN,
                                           "No amplitude found in samples, using default scale");
        }
    }

    if (enableDebug) {
        DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                       huntmaster::DebugLevel::DEBUG,
                                       "Maximum amplitude: " + std::to_string(maxAmp));
        monitor.checkpoint("Found max amplitude");
    }

    // Draw waveform
    const int height = 20;
    int drawnPixels = 0;

    for (int row = height / 2; row >= -height / 2; --row) {
        std::cout << "|";

        for (int col = 0; col < width; ++col) {
            size_t sampleIdx = col * samplesPerColumn;

            // Get average amplitude for this column
            float sum = 0.0f;
            int count = 0;
            for (int i = 0; i < samplesPerColumn && sampleIdx + i < samples.size(); ++i) {
                sum += samples[sampleIdx + i];
                count++;
            }
            float avgAmp = (count > 0) ? sum / count : 0.0f;

            // Scale to display height
            int ampHeight = static_cast<int>((avgAmp / maxAmp) * (height / 2));

            if (row == 0) {
                std::cout << "-";  // Center line
            } else if ((row > 0 && ampHeight >= row) || (row < 0 && ampHeight <= row)) {
                std::cout << "*";
                drawnPixels++;
            } else {
                std::cout << " ";
            }
        }

        std::cout << "|";

        // Add scale labels
        if (row == height / 2) std::cout << " +" << std::fixed << std::setprecision(2) << maxAmp;
        if (row == 0) std::cout << " 0.0";
        if (row == -height / 2) std::cout << " -" << std::fixed << std::setprecision(2) << maxAmp;

        std::cout << std::endl;
    }

    std::cout << std::string(width + 2, '-') << std::endl;

    if (enableDebug) {
        DebugLogger::getInstance().log(
            huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::DEBUG,
            "Visualization completed - Drew " + std::to_string(drawnPixels) + " pixels");
    }
}

// Analyze and display audio characteristics
void analyzeAudioCharacteristics(const std::vector<float> &samples, float sampleRate,
                                 const std::string &label, bool enableDebug = false) {
    if (samples.empty()) {
        DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                       huntmaster::DebugLevel::WARN,
                                       "Cannot analyze empty sample array for: " + label);
        return;
    }

    PerformanceMonitor monitor("Audio characteristics analysis", enableDebug);

    if (enableDebug) {
        DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                       huntmaster::DebugLevel::DEBUG,
                                       "Analyzing audio characteristics for: " + label);
    }

    std::cout << "\n=== " << label << " Analysis ===" << std::endl;

    // Duration
    float duration = samples.size() / sampleRate;
    std::cout << "Duration: " << std::fixed << std::setprecision(3) << duration << " seconds"
              << std::endl;

    if (enableDebug) {
        DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                       huntmaster::DebugLevel::DEBUG,
                                       "Duration calculated: " + std::to_string(duration) + "s");
        monitor.checkpoint("Duration calculation");
    }

    // Find peaks and calculate statistics
    float maxAmp = 0.0f;
    float avgAmp = 0.0f;
    int zeroCrossings = 0;

    for (size_t i = 0; i < samples.size(); ++i) {
        float absAmp = std::abs(samples[i]);
        maxAmp = std::max(maxAmp, absAmp);
        avgAmp += absAmp;

        if (i > 0 &&
            ((samples[i - 1] < 0 && samples[i] >= 0) || (samples[i - 1] >= 0 && samples[i] < 0))) {
            zeroCrossings++;
        }
    }
    avgAmp /= samples.size();

    if (enableDebug) {
        DebugLogger::getInstance().log(
            huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::DEBUG,
            "Statistics - Max: " + std::to_string(maxAmp) + ", Avg: " + std::to_string(avgAmp) +
                ", Zero crossings: " + std::to_string(zeroCrossings));
        monitor.checkpoint("Statistics calculation");
    }

    std::cout << "Peak amplitude: " << maxAmp << std::endl;
    std::cout << "Average amplitude: " << avgAmp << std::endl;

    float estimatedPitch = zeroCrossings / 2.0f / duration;
    std::cout << "Estimated pitch: ~" << estimatedPitch << " Hz" << std::endl;

    if (enableDebug) {
        if (estimatedPitch < 50) {
            DebugLogger::getInstance().log(
                huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::WARN,
                "Estimated pitch very low (" + std::to_string(estimatedPitch) +
                    " Hz) - possible noise or very low frequency content");
        } else if (estimatedPitch > 2000) {
            DebugLogger::getInstance().log(
                huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::WARN,
                "Estimated pitch very high (" + std::to_string(estimatedPitch) +
                    " Hz) - possible noise or artifacts");
        }
    }

    // Energy envelope (RMS over time)
    int windowSize = sampleRate * 0.01f;  // 10ms windows
    int numWindows = 50;                  // Show 50 time points
    int hopSize = samples.size() / numWindows;

    if (enableDebug) {
        DebugLogger::getInstance().log(
            huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::DEBUG,
            "Energy envelope analysis - Window size: " + std::to_string(windowSize) +
                ", Hop size: " + std::to_string(hopSize));
        monitor.checkpoint("Starting energy envelope");
    }

    std::cout << "\nEnergy envelope:" << std::endl;
    std::cout << "Time:  ";
    for (int i = 0; i < numWindows; i += 10) {
        std::cout << std::setw(6) << std::fixed << std::setprecision(1)
                  << (i * hopSize / sampleRate) << "s   ";
    }
    std::cout << std::endl;

    std::cout << "Level: ";
    float totalEnergy = 0.0f;
    for (int i = 0; i < numWindows; ++i) {
        float rms = calculateRMS(samples, i * hopSize, windowSize);
        totalEnergy += rms;

        int barHeight = static_cast<int>(rms * 10 / maxAmp);

        if (barHeight >= 9)
            std::cout << "█";
        else if (barHeight >= 7)
            std::cout << "▓";
        else if (barHeight >= 5)
            std::cout << "▒";
        else if (barHeight >= 3)
            std::cout << "░";
        else if (barHeight >= 1)
            std::cout << "·";
        else
            std::cout << " ";
    }
    std::cout << std::endl;

    if (enableDebug) {
        float avgEnergy = totalEnergy / numWindows;
        DebugLogger::getInstance().log(
            huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::DEBUG,
            "Energy envelope completed - Average energy: " + std::to_string(avgEnergy));

        if (avgEnergy < 0.01f) {
            DebugLogger::getInstance().log(
                huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::WARN,
                "Very low average energy detected - possible silence or very quiet audio");
        }
    }
}

// Generate comparison report
void generateComparisonReport(const std::vector<float> &master, const std::vector<float> &user,
                              float masterSR, float userSR, bool enableDebug = false) {
    PerformanceMonitor monitor("Comparison report generation", enableDebug);

    if (enableDebug) {
        DebugLogger::getInstance().log(
            huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::DEBUG,
            "Generating comparison report - Master: " + std::to_string(master.size()) +
                " samples, User: " + std::to_string(user.size()) + " samples");
    }

    std::cout << "\n=== COMPARISON REPORT ===" << std::endl;

    float masterDuration = master.size() / masterSR;
    float userDuration = user.size() / userSR;
    float durationDiff = std::abs(masterDuration - userDuration);
    float durationRatio = (userDuration / masterDuration) * 100.0f;

    std::cout << "Duration difference: " << durationDiff << " seconds (" << durationRatio
              << "% of master)" << std::endl;

    if (enableDebug) {
        DebugLogger::getInstance().log(
            huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::DEBUG,
            "Duration analysis - Master: " + std::to_string(masterDuration) + "s, User: " +
                std::to_string(userDuration) + "s, Ratio: " + std::to_string(durationRatio) + "%");
        monitor.checkpoint("Duration analysis");
    }

    // Compare energy profiles
    float masterEnergy = 0.0f, userEnergy = 0.0f;
    for (const auto &s : master) masterEnergy += s * s;
    for (const auto &s : user) userEnergy += s * s;

    masterEnergy = std::sqrt(masterEnergy / master.size());
    userEnergy = std::sqrt(userEnergy / user.size());

    float energyRatio = userEnergy / masterEnergy;
    std::cout << "Energy ratio (user/master): " << energyRatio << std::endl;

    if (enableDebug) {
        DebugLogger::getInstance().log(
            huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::DEBUG,
            "Energy analysis - Master: " + std::to_string(masterEnergy) + ", User: " +
                std::to_string(userEnergy) + ", Ratio: " + std::to_string(energyRatio));
        monitor.checkpoint("Energy analysis");
    }

    // Coaching suggestions
    std::cout << "\n=== COACHING SUGGESTIONS ===" << std::endl;
    int suggestionCount = 0;

    if (userDuration > masterDuration * 1.2f) {
        std::cout << "• Your call is too long. Try to make it shorter and more concise."
                  << std::endl;
        suggestionCount++;

        if (enableDebug) {
            DebugLogger::getInstance().log(
                huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::INFO,
                "Coaching: User call too long (" + std::to_string(userDuration) + "s vs " +
                    std::to_string(masterDuration) + "s)");
        }
    } else if (userDuration < masterDuration * 0.8f) {
        std::cout << "• Your call is too short. Try to sustain it longer." << std::endl;
        suggestionCount++;

        if (enableDebug) {
            DebugLogger::getInstance().log(
                huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::INFO,
                "Coaching: User call too short (" + std::to_string(userDuration) + "s vs " +
                    std::to_string(masterDuration) + "s)");
        }
    }

    if (userEnergy < masterEnergy * 0.5f) {
        std::cout << "• Your call is too quiet. Try to project more volume." << std::endl;
        suggestionCount++;

        if (enableDebug) {
            DebugLogger::getInstance().log(
                huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::INFO,
                "Coaching: User call too quiet (energy ratio: " + std::to_string(energyRatio) +
                    ")");
        }
    } else if (userEnergy > masterEnergy * 1.5f) {
        std::cout << "• Your call might be too loud or distorted. Try a more controlled volume."
                  << std::endl;
        suggestionCount++;

        if (enableDebug) {
            DebugLogger::getInstance().log(
                huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::INFO,
                "Coaching: User call too loud (energy ratio: " + std::to_string(energyRatio) + ")");
        }
    }

    if (suggestionCount == 0) {
        std::cout << "• Great job! Your call timing and volume are well matched to the master call."
                  << std::endl;

        if (enableDebug) {
            DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                           huntmaster::DebugLevel::INFO,
                                           "Coaching: No major issues found with user call");
        }
    }

    if (enableDebug) {
        DebugLogger::getInstance().log(
            huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::DEBUG,
            "Comparison report completed - " + std::to_string(suggestionCount) +
                " suggestions generated");
    }
}

// Export visualization data to HTML
void exportToHTML(const std::vector<float> &master, const std::vector<float> &user, float masterSR,
                  float userSR, const std::string &masterName, const std::string &userFile,
                  bool enableDebug = false) {
    PerformanceMonitor monitor("HTML export", enableDebug);

    if (enableDebug) {
        DebugLogger::getInstance().log(
            huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::DEBUG,
            "Exporting to HTML - Master: " + masterName + ", User: " + userFile);
    }

    std::ofstream html("audio_comparison.html");

    if (!html.is_open()) {
        DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                       huntmaster::DebugLevel::ERROR,
                                       "Failed to create HTML file: audio_comparison.html");
        return;
    }

    html << "<!DOCTYPE html><html><head><title>Audio Comparison</title>" << std::endl;
    html << "<script src='https://cdn.plot.ly/plotly-latest.min.js'></script></head>" << std::endl;
    html << "<body><h1>Audio Comparison: " << masterName << " vs " << userFile << "</h1>"
         << std::endl;

    if (enableDebug) {
        monitor.checkpoint("HTML header written");
    }

    // Generate time arrays with downsampling for performance
    int downsampleFactor = 100;
    html << "<div id='waveforms'></div><script>" << std::endl;

    html << "var masterTime = [";
    for (size_t i = 0; i < master.size(); i += downsampleFactor) {
        html << (i / masterSR) << ",";
    }
    html << "];" << std::endl;

    html << "var masterData = [";
    for (size_t i = 0; i < master.size(); i += downsampleFactor) {
        html << master[i] << ",";
    }
    html << "];" << std::endl;

    if (enableDebug) {
        monitor.checkpoint("Master data written");
    }

    html << "var userTime = [";
    for (size_t i = 0; i < user.size(); i += downsampleFactor) {
        html << (i / userSR) << ",";
    }
    html << "];" << std::endl;

    html << "var userData = [";
    for (size_t i = 0; i < user.size(); i += downsampleFactor) {
        html << user[i] << ",";
    }
    html << "];" << std::endl;

    if (enableDebug) {
        monitor.checkpoint("User data written");
    }

    html << "var trace1 = {x: masterTime, y: masterData, name: 'Master Call', type: 'scatter'};"
         << std::endl;
    html << "var trace2 = {x: userTime, y: userData, name: 'Your Recording', type: 'scatter'};"
         << std::endl;
    html << "var data = [trace1, trace2];" << std::endl;
    html << "var layout = {title: 'Waveform Comparison', xaxis: {title: 'Time (s)'}, yaxis: "
            "{title: 'Amplitude'}};"
         << std::endl;
    html << "Plotly.newPlot('waveforms', data, layout);" << std::endl;
    html << "</script></body></html>" << std::endl;

    html.close();

    if (enableDebug) {
        DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                       huntmaster::DebugLevel::DEBUG,
                                       "HTML export completed successfully");
    }

    std::cout << "\nVisualization exported to: audio_comparison.html" << std::endl;
}

int main(int argc, char *argv[]) {
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
    auto &logger = DebugLogger::getInstance();
    if (debugOptions.enableVisualizationDebug) {
        logger.setComponentLevel(huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::TRACE);
    }
    if (debugOptions.enableAudioAnalysis) {
        logger.setComponentLevel(huntmaster::DebugComponent::AUDIO_PROCESSING,
                                 huntmaster::DebugLevel::DEBUG);
    }
    if (debugOptions.enablePerformanceMetrics) {
        logger.setComponentLevel(huntmaster::DebugComponent::PERFORMANCE,
                                 huntmaster::DebugLevel::DEBUG);
    }

    DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::INFO,
                                   "=== Huntmaster Audio Visualization Tool Started ===");

    PerformanceMonitor totalMonitor("Total execution", debugOptions.enablePerformanceMetrics);

    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <master_call_name> <user_recording.wav> [options]"
                  << std::endl;
        std::cout << "Use --help for detailed usage information." << std::endl;
        return 1;
    }

    std::string masterCallName = argv[1];
    std::string userRecordingPath = argv[2];

    if (debugOptions.enableDebug) {
        DebugLogger::getInstance().log(
            huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::DEBUG,
            "Processing files - Master: " + masterCallName + ", User: " + userRecordingPath);
    }

    // Load master call
    std::string masterPath = "../data/master_calls/" + masterCallName + ".wav";
    unsigned int masterChannels, masterSR;
    std::vector<float> masterAudio = loadAudioFile(masterPath, masterChannels, masterSR);

    if (masterAudio.empty()) {
        std::cerr << "Failed to load master call: " << masterPath << std::endl;
        DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                       huntmaster::DebugLevel::ERROR,
                                       "Failed to load master call: " + masterPath);
        return 1;
    }

    totalMonitor.checkpoint("Master call loaded");

    // Load user recording
    unsigned int userChannels, userSR;
    std::vector<float> userAudio = loadAudioFile(userRecordingPath, userChannels, userSR);

    if (userAudio.empty()) {
        std::cerr << "Failed to load user recording: " << userRecordingPath << std::endl;
        DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                       huntmaster::DebugLevel::ERROR,
                                       "Failed to load user recording: " + userRecordingPath);
        return 1;
    }

    totalMonitor.checkpoint("User recording loaded");

    // Visual comparison
    std::cout << "\n=== WAVEFORM COMPARISON ===" << std::endl;
    visualizeWaveform(masterAudio, "Master Call: " + masterCallName, 80,
                      debugOptions.enableVisualizationDebug);
    visualizeWaveform(userAudio, "Your Recording: " + userRecordingPath, 80,
                      debugOptions.enableVisualizationDebug);

    totalMonitor.checkpoint("Waveform visualization completed");

    // Detailed analysis
    analyzeAudioCharacteristics(masterAudio, masterSR, "Master Call",
                                debugOptions.enableAudioAnalysis);
    analyzeAudioCharacteristics(userAudio, userSR, "Your Recording",
                                debugOptions.enableAudioAnalysis);

    totalMonitor.checkpoint("Audio analysis completed");

    // Comparison report
    generateComparisonReport(masterAudio, userAudio, masterSR, userSR,
                             debugOptions.enableComparisonDebug);

    totalMonitor.checkpoint("Comparison report generated");

    // Export to HTML for better visualization
    exportToHTML(masterAudio, userAudio, masterSR, userSR, masterCallName, userRecordingPath,
                 debugOptions.enableExportDebug);

    totalMonitor.checkpoint("HTML export completed");

    // Run similarity analysis
    std::cout << "\n=== SIMILARITY ANALYSIS ===" << std::endl;

    PerformanceMonitor engineMonitor("Engine analysis", debugOptions.enablePerformanceMetrics);

    HuntmasterAudioEngine &engine = HuntmasterAudioEngine::getInstance();
    engine.initialize();

    if (debugOptions.enableDebug) {
        DebugLogger::getInstance().log(
            huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::DEBUG,
            "Engine initialized, loading master call: " + masterCallName);
    }

    engine.loadMasterCall(masterCallName);
    int sessionId = engine.startRealtimeSession(static_cast<float>(userSR), 1024);

    if (debugOptions.enableDebug) {
        DebugLogger::getInstance().log(
            huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::DEBUG,
            "Started realtime session with ID: " + std::to_string(sessionId));
    }

    engineMonitor.checkpoint("Engine setup completed");

    // Process in chunks
    const int chunkSize = 1024;
    int chunksProcessed = 0;

    for (size_t i = 0; i < userAudio.size(); i += chunkSize) {
        size_t remaining = userAudio.size() - i;
        size_t toProcess = (remaining < chunkSize) ? remaining : chunkSize;
        engine.processAudioChunk(sessionId, userAudio.data() + i, toProcess);
        chunksProcessed++;

        if (debugOptions.enableTrace && chunksProcessed % 100 == 0) {
            DebugLogger::getInstance().log(
                huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::TRACE,
                "Processed " + std::to_string(chunksProcessed) + " chunks");
        }
    }

    engineMonitor.checkpoint("Audio processing completed");

    if (debugOptions.enableDebug) {
        DebugLogger::getInstance().log(
            huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::DEBUG,
            "Processed " + std::to_string(chunksProcessed) + " audio chunks");
    }

    float score = engine.getSimilarityScore(sessionId);
    std::cout << "Similarity Score: " << score;

    if (score > 0.01) {
        std::cout << " [EXCELLENT MATCH]" << std::endl;
    } else if (score > 0.005) {
        std::cout << " [Good match]" << std::endl;
    } else if (score > 0.002) {
        std::cout << " [Fair match]" << std::endl;
    } else {
        std::cout << " [Needs improvement]" << std::endl;
    }

    if (debugOptions.enableDebug) {
        DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS,
                                       huntmaster::DebugLevel::DEBUG,
                                       "Similarity score: " + std::to_string(score));
    }

    engine.endRealtimeSession(sessionId);
    engine.shutdown();

    engineMonitor.checkpoint("Engine shutdown completed");

    DebugLogger::getInstance().log(huntmaster::DebugComponent::TOOLS, huntmaster::DebugLevel::INFO,
                                   "=== Audio Visualization Tool Completed Successfully ===");

    return 0;
}