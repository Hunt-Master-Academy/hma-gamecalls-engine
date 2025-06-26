#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include "dr_wav.h"

// Configuration for sound detection
struct VADConfig
{
    float silenceThreshold = 0.01f;  // Amplitude threshold for silence (adjustable)
    float energyThreshold = 0.0001f; // Energy threshold (RMS squared)
    int minSilenceFrames = 2205;     // Minimum silence duration (50ms at 44.1kHz)
    int minSoundFrames = 4410;       // Minimum sound duration (100ms at 44.1kHz)
    float hangoverTime = 0.1f;       // Time to wait after sound ends (seconds)
};

// Calculate RMS energy for a window
float calculateEnergy(const std::vector<float> &samples, size_t start, size_t windowSize)
{
    float sum = 0.0f;
    size_t end = std::min(start + windowSize, samples.size());

    for (size_t i = start; i < end; ++i)
    {
        sum += samples[i] * samples[i];
    }

    return sum / (end - start); // Mean squared value
}

// Find the start of actual audio (first non-silence)
size_t findAudioStart(const std::vector<float> &samples, float sampleRate, const VADConfig &config)
{
    int windowSize = static_cast<int>(sampleRate * 0.01f); // 10ms windows
    int consecutiveSoundFrames = 0;
    int requiredFrames = static_cast<int>(sampleRate * 0.02f); // Need 20ms of sound

    for (size_t i = 0; i < samples.size(); i += windowSize / 2)
    { // 50% overlap
        float energy = calculateEnergy(samples, i, windowSize);
        float peakInWindow = 0.0f;

        // Also check peak amplitude in window
        for (size_t j = i; j < static_cast<size_t>(std::min(static_cast<int>(i) + windowSize, static_cast<int>(samples.size()))); ++j)
        {
            peakInWindow = std::max(peakInWindow, std::abs(samples[j]));
        }

        if (energy > config.energyThreshold || peakInWindow > config.silenceThreshold)
        {
            consecutiveSoundFrames += windowSize / 2;
            if (consecutiveSoundFrames >= requiredFrames)
            {
                // Found start - backtrack a bit to not cut off attack
                return std::max(0, static_cast<int>(i) - windowSize);
            }
        }
        else
        {
            consecutiveSoundFrames = 0;
        }
    }

    return 0; // No sound found
}

// Find the end of actual audio (last non-silence)
size_t findAudioEnd(const std::vector<float> &samples, float sampleRate, const VADConfig &config)
{
    int windowSize = static_cast<int>(sampleRate * 0.01f); // 10ms windows
    int hangoverSamples = static_cast<int>(sampleRate * config.hangoverTime);
    size_t lastSoundFrame = samples.size();

    // Search backwards
    for (int i = samples.size() - windowSize; i >= 0; i -= windowSize / 2)
    {
        float energy = calculateEnergy(samples, i, windowSize);
        float peakInWindow = 0.0f;

        for (size_t j = i; j < static_cast<size_t>(std::min(static_cast<int>(i) + windowSize, static_cast<int>(samples.size()))); ++j)
        {
            peakInWindow = std::max(peakInWindow, std::abs(samples[j]));
        }

        if (energy > config.energyThreshold || peakInWindow > config.silenceThreshold)
        {
            // Found sound - add hangover time
            lastSoundFrame = std::min(samples.size(), static_cast<size_t>(i) + windowSize + hangoverSamples);
            break;
        }
    }

    return lastSoundFrame;
}

// Trim silence from audio
std::vector<float> trimSilence(const std::vector<float> &samples, float sampleRate, const VADConfig &config)
{
    if (samples.empty())
        return samples;

    size_t start = findAudioStart(samples, sampleRate, config);
    size_t end = findAudioEnd(samples, sampleRate, config);

    // Ensure valid range
    if (start >= end || start >= samples.size())
    {
        std::cout << "Warning: No significant audio detected!" << std::endl;
        return samples; // Return original if no valid audio found
    }

    // Extract trimmed audio
    std::vector<float> trimmed(samples.begin() + start, samples.begin() + end);

    // Apply fade in/out to avoid clicks
    int fadeLength = static_cast<int>(sampleRate * 0.005f); // 5ms fade
    for (int i = 0; i < fadeLength && i < trimmed.size(); ++i)
    {
        float factor = static_cast<float>(i) / fadeLength;
        trimmed[i] *= factor;
    }
    for (int i = 0; i < fadeLength && i < trimmed.size(); ++i)
    {
        float factor = static_cast<float>(i) / fadeLength;
        trimmed[trimmed.size() - 1 - i] *= factor;
    }

    return trimmed;
}

// Visualize audio with silence regions marked
void visualizeWithSilence(const std::vector<float> &samples, float sampleRate, const std::string &label)
{
    const int width = 80;
    const int height = 10;

    std::cout << "\n"
              << label << std::endl;
    std::cout << std::string(width, '=') << std::endl;

    VADConfig config;
    size_t audioStart = findAudioStart(samples, sampleRate, config);
    size_t audioEnd = findAudioEnd(samples, sampleRate, config);

    // Show timeline
    float duration = samples.size() / sampleRate;
    std::cout << "Duration: " << std::fixed << std::setprecision(3) << duration << "s" << std::endl;
    std::cout << "Audio region: " << (audioStart / sampleRate) << "s - " << (audioEnd / sampleRate) << "s" << std::endl;

    // Draw waveform with regions
    int samplesPerColumn = samples.size() / width;
    if (samplesPerColumn < 1)
        samplesPerColumn = 1;

    // Find max amplitude
    float maxAmp = 0.0f;
    for (const auto &s : samples)
    {
        maxAmp = std::max(maxAmp, std::abs(s));
    }
    if (maxAmp == 0.0f)
        maxAmp = 1.0f;

    // Draw silence indicators
    std::cout << "Silence: ";
    for (int col = 0; col < width; ++col)
    {
        size_t sampleIdx = col * samplesPerColumn;
        if (sampleIdx < audioStart || sampleIdx >= audioEnd)
        {
            std::cout << "S";
        }
        else
        {
            std::cout << " ";
        }
    }
    std::cout << std::endl;

    // Draw waveform
    for (int row = height; row >= -height; --row)
    {
        std::cout << "        |";

        for (int col = 0; col < width; ++col)
        {
            size_t sampleIdx = col * samplesPerColumn;

            // RMS for this column
            float rms = 0.0f;
            int count = 0;
            for (int i_inner = 0; i_inner < samplesPerColumn && sampleIdx + i_inner < samples.size(); ++i_inner) // Fix: Changed 'i' to 'i_inner'
            {
                rms += samples[sampleIdx + i_inner] * samples[sampleIdx + i_inner];
                count++;
            }
            rms = (count > 0) ? std::sqrt(rms / count) : 0.0f;

            int ampHeight = static_cast<int>((rms / maxAmp) * height);

            // Color code: silence vs audio
            bool isSilence = (sampleIdx < audioStart || sampleIdx >= audioEnd);

            if (row == 0)
            {
                std::cout << "-";
            }
            else if (row > 0 && ampHeight >= row)
            {
                std::cout << (isSilence ? "." : "*");
            }
            else if (row < 0 && -ampHeight >= -row)
            {
                std::cout << (isSilence ? "." : "*");
            }
            else
            {
                std::cout << " ";
            }
        }

        std::cout << "|" << std::endl;
    }
    std::cout << std::string(width + 10, '-') << std::endl;
}

// Load and process audio file
bool processAudioFile(const std::string &inputPath, const std::string &outputPath, VADConfig &config, bool visualize = false)
{
    // Load audio
    unsigned int channels, sampleRate;
    drwav_uint64 totalFrames;
    float *pSampleData = drwav_open_file_and_read_pcm_frames_f32(inputPath.c_str(), &channels, &sampleRate, &totalFrames, nullptr);

    if (!pSampleData)
    {
        std::cerr << "Failed to load: " << inputPath << std::endl;
        return false;
    }

    // Convert to mono
    std::vector<float> samples(totalFrames);
    if (channels > 1)
    {
        for (drwav_uint64 i = 0; i < totalFrames; ++i)
        {
            float sum = 0.0f;
            for (unsigned int ch = 0; ch < channels; ++ch)
            {
                sum += pSampleData[i * channels + ch];
            }
            samples[i] = sum / channels;
        }
    }
    else
    {
        samples.assign(pSampleData, pSampleData + totalFrames);
    }
    drwav_free(pSampleData, nullptr);

    // Visualize original
    if (visualize)
    {
        visualizeWithSilence(samples, sampleRate, "Original: " + inputPath);
    }

    // Trim silence
    std::vector<float> trimmed = trimSilence(samples, sampleRate, config);

    // Show results
    float originalDuration = samples.size() / static_cast<float>(sampleRate);
    float trimmedDuration = trimmed.size() / static_cast<float>(sampleRate);

    std::cout << "\nTrimming results:" << std::endl;
    std::cout << "Original: " << std::fixed << std::setprecision(3) << originalDuration << "s (" << samples.size() << " samples)" << std::endl;
    std::cout << "Trimmed:  " << std::fixed << std::setprecision(3) << trimmedDuration << "s (" << trimmed.size() << " samples)" << std::endl;
    std::cout << "Removed:  " << std::fixed << std::setprecision(3) << (originalDuration - trimmedDuration) << "s" << std::endl;

    // Visualize trimmed
    if (visualize)
    {
        visualizeWithSilence(trimmed, sampleRate, "Trimmed: " + outputPath);
    }

    // Save trimmed audio
    drwav wav;
    drwav_data_format format;
    format.container = drwav_container_riff;
    format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
    format.channels = 1;
    format.sampleRate = sampleRate;
    format.bitsPerSample = 32;

    if (!drwav_init_file_write(&wav, outputPath.c_str(), &format, nullptr))
    {
        std::cerr << "Failed to create output file: " << outputPath << std::endl;
        return false;
    }

    drwav_uint64 framesWritten = drwav_write_pcm_frames(&wav, trimmed.size(), trimmed.data());
    drwav_uninit(&wav);

    std::cout << "Saved trimmed audio to: " << outputPath << std::endl;

    return framesWritten > 0;
}

int main(int argc, char *argv[])
{
    std::cout << "=== Audio Trimming Tool ===" << std::endl;

    if (argc < 2)
    {
        std::cout << "\nUsage: " << argv[0] << " <input.wav> [output.wav] [options]" << std::endl;
        std::cout << "\nOptions:" << std::endl;
        std::cout << "  -v              Visualize waveforms" << std::endl;
        std::cout << "  -t <threshold>  Set silence threshold (default: 0.01)" << std::endl;
        std::cout << "  -batch          Process all WAV files in recordings directory" << std::endl;
        std::cout << "\nExamples:" << std::endl;
        std::cout << "  " << argv[0] << " recording.wav trimmed.wav -v" << std::endl;
        std::cout << "  " << argv[0] << " -batch" << std::endl;
        return 1;
    }

    VADConfig config;
    bool visualize = false;
    bool batchMode = false;

    // Parse arguments
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "-v")
        {
            visualize = true;
        }
        else if (arg == "-t" && i + 1 < argc)
        {
            config.silenceThreshold = std::stof(argv[++i]);
        }
        else if (arg == "-batch")
        {
            batchMode = true;
        }
    }

    if (batchMode)
    {
        // Process all recordings
        std::cout << "\nBatch processing recordings..." << std::endl;

        // Process test recordings
        processAudioFile("../data/recordings/user_attempt_buck_grunt.wav",
                         "../data/recordings/user_attempt_buck_grunt_trimmed.wav", config, visualize);

        processAudioFile("../data/recordings/test_grunt.wav",
                         "../data/recordings/test_grunt_trimmed.wav", config, visualize);

        // Process master calls
        std::vector<std::string> masters = {"buck_grunt", "doe-grunt", "buck-bawl"};
        for (const auto &master : masters)
        {
            std::string input = "../data/master_calls/" + master + ".wav";
            std::string output = "../data/master_calls/" + master + "_trimmed.wav";
            processAudioFile(input, output, config, visualize);
        }
    }
    else
    {
        // Single file mode
        std::string inputPath = argv[1];
        std::string outputPath = (argc > 2 && argv[2][0] != '-') ? argv[2] : "trimmed_output.wav";

        processAudioFile(inputPath, outputPath, config, visualize);
    }

    return 0;
}