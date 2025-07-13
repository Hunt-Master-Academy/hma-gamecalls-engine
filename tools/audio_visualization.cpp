#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

#include "dr_wav.h"
#include "huntmaster/core/HuntmasterAudioEngine.h"

using huntmaster::HuntmasterAudioEngine;

std::vector<float> loadAudioFile(const std::string &filePath, unsigned int &channels,
                                 unsigned int &sampleRate) {
    drwav_uint64 totalPCMFrameCount = 0;
    float *pSampleData = drwav_open_file_and_read_pcm_frames_f32(
        filePath.c_str(), &channels, &sampleRate, &totalPCMFrameCount, nullptr);

    if (pSampleData == nullptr) {
        return {};
    }

    // Convert to mono if needed
    std::vector<float> monoSamples(totalPCMFrameCount);
    if (channels > 1) {
        for (drwav_uint64 i = 0; i < totalPCMFrameCount; ++i) {
            float monoSample = 0.0f;
            for (unsigned int j = 0; j < channels; ++j) {
                monoSample += pSampleData[i * channels + j];
            }
            monoSamples[i] = monoSample / channels;
        }
    } else {
        monoSamples.assign(pSampleData, pSampleData + totalPCMFrameCount);
    }

    drwav_free(pSampleData, nullptr);
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
void visualizeWaveform(const std::vector<float> &samples, const std::string &label,
                       int width = 80) {
    if (samples.empty()) return;

    std::cout << "\n" << label << " (" << samples.size() << " samples)" << std::endl;
    std::cout << std::string(width, '-') << std::endl;

    // Downsample to fit width
    int samplesPerColumn = samples.size() / width;
    if (samplesPerColumn < 1) samplesPerColumn = 1;

    // Find max amplitude for scaling
    float maxAmp = 0.0f;
    for (const auto &s : samples) {
        maxAmp = std::max(maxAmp, std::abs(s));
    }

    if (maxAmp == 0.0f) maxAmp = 1.0f;

    // Draw waveform
    const int height = 20;
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
}

// Analyze and display audio characteristics
void analyzeAudioCharacteristics(const std::vector<float> &samples, float sampleRate,
                                 const std::string &label) {
    if (samples.empty()) return;

    std::cout << "\n=== " << label << " Analysis ===" << std::endl;

    // Duration
    float duration = samples.size() / sampleRate;
    std::cout << "Duration: " << std::fixed << std::setprecision(3) << duration << " seconds"
              << std::endl;

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

    std::cout << "Peak amplitude: " << maxAmp << std::endl;
    std::cout << "Average amplitude: " << avgAmp << std::endl;
    std::cout << "Estimated pitch: ~" << (zeroCrossings / 2.0f / duration) << " Hz" << std::endl;

    // Energy envelope (RMS over time)
    int windowSize = sampleRate * 0.01f;  // 10ms windows
    int numWindows = 50;                  // Show 50 time points
    int hopSize = samples.size() / numWindows;

    std::cout << "\nEnergy envelope:" << std::endl;
    std::cout << "Time:  ";
    for (int i = 0; i < numWindows; i += 10) {
        std::cout << std::setw(6) << std::fixed << std::setprecision(1)
                  << (i * hopSize / sampleRate) << "s   ";
    }
    std::cout << std::endl;

    std::cout << "Level: ";
    for (int i = 0; i < numWindows; ++i) {
        float rms = calculateRMS(samples, i * hopSize, windowSize);
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
}

// Generate comparison report
void generateComparisonReport(const std::vector<float> &master, const std::vector<float> &user,
                              float masterSR, float userSR) {
    std::cout << "\n=== COMPARISON REPORT ===" << std::endl;

    float masterDuration = master.size() / masterSR;
    float userDuration = user.size() / userSR;

    std::cout << "Duration difference: " << std::abs(masterDuration - userDuration) << " seconds ("
              << ((userDuration / masterDuration) * 100.0f) << "% of master)" << std::endl;

    // Compare energy profiles
    float masterEnergy = 0.0f, userEnergy = 0.0f;
    for (const auto &s : master) masterEnergy += s * s;
    for (const auto &s : user) userEnergy += s * s;

    masterEnergy = std::sqrt(masterEnergy / master.size());
    userEnergy = std::sqrt(userEnergy / user.size());

    std::cout << "Energy ratio (user/master): " << (userEnergy / masterEnergy) << std::endl;

    // Coaching suggestions
    std::cout << "\n=== COACHING SUGGESTIONS ===" << std::endl;

    if (userDuration > masterDuration * 1.2f) {
        std::cout << "• Your call is too long. Try to make it shorter and more concise."
                  << std::endl;
    } else if (userDuration < masterDuration * 0.8f) {
        std::cout << "• Your call is too short. Try to sustain it longer." << std::endl;
    }

    if (userEnergy < masterEnergy * 0.5f) {
        std::cout << "• Your call is too quiet. Try to project more volume." << std::endl;
    } else if (userEnergy > masterEnergy * 1.5f) {
        std::cout << "• Your call might be too loud or distorted. Try a more controlled volume."
                  << std::endl;
    }
}

// Export visualization data to HTML
void exportToHTML(const std::vector<float> &master, const std::vector<float> &user, float masterSR,
                  float userSR, const std::string &masterName, const std::string &userFile) {
    std::ofstream html("audio_comparison.html");

    html << "<!DOCTYPE html><html><head><title>Audio Comparison</title>" << std::endl;
    html << "<script src='https://cdn.plot.ly/plotly-latest.min.js'></script></head>" << std::endl;
    html << "<body><h1>Audio Comparison: " << masterName << " vs " << userFile << "</h1>"
         << std::endl;

    // Generate time arrays
    html << "<div id='waveforms'></div><script>" << std::endl;
    html << "var masterTime = [";
    for (size_t i = 0; i < master.size(); i += 100) {
        html << (i / masterSR) << ",";
    }
    html << "];" << std::endl;

    html << "var masterData = [";
    for (size_t i = 0; i < master.size(); i += 100) {
        html << master[i] << ",";
    }
    html << "];" << std::endl;

    html << "var userTime = [";
    for (size_t i = 0; i < user.size(); i += 100) {
        html << (i / userSR) << ",";
    }
    html << "];" << std::endl;

    html << "var userData = [";
    for (size_t i = 0; i < user.size(); i += 100) {
        html << user[i] << ",";
    }
    html << "];" << std::endl;

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
    std::cout << "\nVisualization exported to: audio_comparison.html" << std::endl;
}

int main(int argc, char *argv[]) {
    std::cout << "=== Huntmaster Audio Visualization Tool ===" << std::endl;

    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <master_call_name> <user_recording.wav>"
                  << std::endl;
        std::cout << "Example: " << argv[0] << " buck_grunt ../data/recordings/user_attempt.wav"
                  << std::endl;
        return 1;
    }

    std::string masterCallName = argv[1];
    std::string userRecordingPath = argv[2];

    // Load master call
    std::string masterPath = "../data/master_calls/" + masterCallName + ".wav";
    unsigned int masterChannels, masterSR;
    std::vector<float> masterAudio = loadAudioFile(masterPath, masterChannels, masterSR);

    if (masterAudio.empty()) {
        std::cerr << "Failed to load master call: " << masterPath << std::endl;
        return 1;
    }

    // Load user recording
    unsigned int userChannels, userSR;
    std::vector<float> userAudio = loadAudioFile(userRecordingPath, userChannels, userSR);

    if (userAudio.empty()) {
        std::cerr << "Failed to load user recording: " << userRecordingPath << std::endl;
        return 1;
    }

    // Visual comparison
    std::cout << "\n=== WAVEFORM COMPARISON ===" << std::endl;
    visualizeWaveform(masterAudio, "Master Call: " + masterCallName);
    visualizeWaveform(userAudio, "Your Recording: " + userRecordingPath);

    // Detailed analysis
    analyzeAudioCharacteristics(masterAudio, masterSR, "Master Call");
    analyzeAudioCharacteristics(userAudio, userSR, "Your Recording");

    // Comparison report
    generateComparisonReport(masterAudio, userAudio, masterSR, userSR);

    // Export to HTML for better visualization
    exportToHTML(masterAudio, userAudio, masterSR, userSR, masterCallName, userRecordingPath);

    // Run similarity analysis
    std::cout << "\n=== SIMILARITY ANALYSIS ===" << std::endl;
    HuntmasterAudioEngine &engine = HuntmasterAudioEngine::getInstance();
    engine.initialize();

    engine.loadMasterCall(masterCallName);
    int sessionId = engine.startRealtimeSession(static_cast<float>(userSR), 1024);

    // Process in chunks
    const int chunkSize = 1024;
    for (size_t i = 0; i < userAudio.size(); i += chunkSize) {
        size_t remaining = userAudio.size() - i;
        size_t toProcess = (remaining < chunkSize) ? remaining : chunkSize;
        engine.processAudioChunk(sessionId, userAudio.data() + i, toProcess);
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

    engine.endRealtimeSession(sessionId);
    engine.shutdown();

    return 0;
}