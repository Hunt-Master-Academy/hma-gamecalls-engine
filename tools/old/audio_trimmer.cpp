#include <iostream>
#include <vector>
#include <string>
#include <string_view>
#include <filesystem>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <span>

#include "dr_wav.h"

namespace AudioTrimmer
{
    struct VADConfig
    {
        float silenceThreshold = 0.01f;
        float energyThreshold = 0.0001f;
        int minSilenceFrames = 2205;
        int minSoundFrames = 4410;
        float hangoverTime = 0.1f;
    };

    float calculateEnergy(std::span<const float> samples)
    {
        if (samples.empty())
            return 0.0f;

        double sum = 0.0;
        for (float sample : samples)
        {
            sum += static_cast<double>(sample) * sample;
        }
        return static_cast<float>(sum / samples.size());
    }

    size_t findAudioStart(std::span<const float> samples, float sampleRate, const VADConfig &config)
    {
        const size_t windowSize = static_cast<size_t>(sampleRate * 0.01f);
        const size_t requiredSamples = static_cast<size_t>(sampleRate * 0.02f);
        if (windowSize == 0 || requiredSamples == 0)
            return 0;

        size_t consecutiveSoundSamples = 0;
        const size_t hopSize = windowSize / 2;

        for (size_t i = 0; i + windowSize <= samples.size(); i += hopSize)
        {
            auto window = samples.subspan(i, windowSize);
            float energy = calculateEnergy(window);

            float peakInWindow = 0.0f;
            for (float sample : window)
            {
                peakInWindow = std::max(peakInWindow, std::abs(sample));
            }

            if (energy > config.energyThreshold || peakInWindow > config.silenceThreshold)
            {
                consecutiveSoundSamples += hopSize;
                if (consecutiveSoundSamples >= requiredSamples)
                {
                    return (i > windowSize) ? (i - windowSize) : 0;
                }
            }
            else
            {
                consecutiveSoundSamples = 0;
            }
        }
        return samples.size();
    }

    size_t findAudioEnd(std::span<const float> samples, float sampleRate, const VADConfig &config)
    {
        const size_t windowSize = static_cast<size_t>(sampleRate * 0.01f);
        const size_t hangoverSamples = static_cast<size_t>(sampleRate * config.hangoverTime);
        if (windowSize == 0)
            return samples.size();

        const size_t hopSize = windowSize / 2;

        for (size_t i = samples.size() - windowSize; i > 0; i -= hopSize)
        {
            auto window = samples.subspan(i, windowSize);
            float energy = calculateEnergy(window);

            float peakInWindow = 0.0f;
            for (float sample : window)
            {
                peakInWindow = std::max(peakInWindow, std::abs(sample));
            }

            if (energy > config.energyThreshold || peakInWindow > config.silenceThreshold)
            {
                return std::min(samples.size(), i + windowSize + hangoverSamples);
            }
        }
        return 0;
    }

    std::vector<float> trimSilence(std::span<const float> samples, float sampleRate, const VADConfig &config)
    {
        if (samples.empty())
            return {};

        size_t start = findAudioStart(samples, sampleRate, config);
        size_t end = findAudioEnd(samples, sampleRate, config);

        if (start >= end)
        {
            std::cout << "Warning: No significant audio detected! Returning original." << std::endl;
            return {samples.begin(), samples.end()};
        }

        std::vector<float> trimmed(samples.begin() + start, samples.begin() + end);

        const size_t fadeLength = static_cast<size_t>(sampleRate * 0.005f);
        for (size_t i = 0; i < fadeLength && i < trimmed.size(); ++i)
        {
            float factor = static_cast<float>(i) / fadeLength;
            trimmed[i] *= factor;
            trimmed[trimmed.size() - 1 - i] *= factor;
        }
        return trimmed;
    }

    void visualizeWithSilence(
        std::span<const float> samples,
        float sampleRate,
        std::string_view label,
        int width = 80,
        int height = 10)
    {
        if (samples.empty())
            return;

        std::cout << "\n"
                  << label << "\n"
                  << std::string(width + 12, '=') << std::endl;

        VADConfig config;
        // POLISHED: Removed redundant "AudioTrimmer::" prefix
        size_t audioStart = findAudioStart(samples, sampleRate, config);
        size_t audioEnd = findAudioEnd(samples, sampleRate, config);
        float duration = samples.size() / sampleRate;

        std::cout << "Duration: " << std::fixed << std::setprecision(2) << duration << "s | "
                  << "Audio Region: " << (audioStart / sampleRate) << "s - " << (audioEnd / sampleRate) << "s\n";
        std::cout << std::string(width + 12, '-') << std::endl;

        struct Column
        {
            int ampHeight;
            bool isSilence;
        };
        std::vector<Column> columns;
        columns.reserve(width);

        const size_t samplesPerColumn = samples.size() / width;
        if (samplesPerColumn == 0)
            return;

        float maxRms = 0.0f;
        std::vector<float> columnRms;
        columnRms.reserve(width);

        for (int i = 0; i < width; ++i)
        {
            auto window = samples.subspan(i * samplesPerColumn, samplesPerColumn);
            // POLISHED: Removed redundant "AudioTrimmer::" prefix
            float rms_sq = calculateEnergy(window);
            columnRms.push_back(std::sqrt(rms_sq));
            if (columnRms.back() > maxRms)
            {
                maxRms = columnRms.back();
            }
        }
        if (maxRms == 0.0f)
            maxRms = 1.0f;

        for (int i = 0; i < width; ++i)
        {
            size_t sampleIdx = i * samplesPerColumn;
            columns.push_back({.ampHeight = static_cast<int>((columnRms[i] / maxRms) * height),
                               .isSilence = (sampleIdx < audioStart || sampleIdx >= audioEnd)});
        }

        for (int row = height; row >= -height; --row)
        {
            if (row > 0 && row < 1)
                continue;

            if (row == 0)
                std::cout << "Center --|";
            else
                std::cout << "        |";

            for (const auto &col : columns)
            {
                char c = ' ';
                if (row == 0)
                {
                    c = '-';
                }
                else if (row > 0 && col.ampHeight >= row)
                {
                    c = col.isSilence ? '.' : '*';
                }
                else if (row < 0 && -col.ampHeight <= row)
                {
                    c = col.isSilence ? '.' : '*';
                }
                std::cout << c;
            }
            std::cout << "|\n";
        }
        std::cout << std::string(width + 12, '-') << std::endl;
    }

    bool processAudioFile(const std::filesystem::path &inputPath, const std::filesystem::path &outputPath, const VADConfig &config, bool visualize)
    {
        unsigned int channels, sampleRate;
        drwav_uint64 totalFrames;
        float *pSampleData = drwav_open_file_and_read_pcm_frames_f32(inputPath.string().c_str(), &channels, &sampleRate, &totalFrames, nullptr);

        if (!pSampleData)
        {
            std::cerr << "Failed to load: " << inputPath << std::endl;
            return false;
        }

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

        if (visualize)
        {
            // POLISHED: Re-enabled the visualization call
            visualizeWithSilence(samples, sampleRate, "Original: " + inputPath.string());
        }

        std::vector<float> trimmed = trimSilence(samples, sampleRate, config);

        float originalDuration = samples.size() / static_cast<float>(sampleRate);
        float trimmedDuration = trimmed.size() / static_cast<float>(sampleRate);

        std::cout << "\n---\nProcessed: " << inputPath.filename() << std::endl;
        std::cout << "  Original: " << std::fixed << std::setprecision(3) << originalDuration << "s\n";
        std::cout << "  Trimmed:  " << std::fixed << std::setprecision(3) << trimmedDuration << "s\n";
        std::cout << "  Removed:  " << std::fixed << std::setprecision(3) << (originalDuration - trimmedDuration) << "s" << std::endl;

        // Save trimmed audio
        drwav wav;
        drwav_data_format format;
        format.container = drwav_container_riff;
        format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
        format.channels = 1;
        format.sampleRate = sampleRate;
        format.bitsPerSample = 32;

        if (!drwav_init_file_write(&wav, outputPath.string().c_str(), &format, nullptr))
        {
            std::cerr << "Failed to create output file: " << outputPath << std::endl;
            return false;
        }

        drwav_write_pcm_frames(&wav, trimmed.size(), trimmed.data());
        drwav_uninit(&wav);

        std::cout << "  Saved to: " << outputPath << std::endl;
        return true;
    }
}

int main(int argc, char *argv[])
{
    std::cout << "=== Audio Trimming Tool ===" << std::endl;
    if (argc < 2)
    {
        std::cout << "\nUsage: " << argv[0] << " <input.wav> [output.wav] [options]\n"
                  << "\nOptions:\n"
                  << "  -v              Visualize waveforms\n"
                  << "  -t <threshold>  Set silence threshold (default: 0.01)\n"
                  << "  -batch          Process all WAV files in a directory\n"
                  << "\nExample:\n"
                  << "  " << argv[0] << " ../data/recordings/my_rec.wav -v\n"
                  << "  " << argv[0] << " -batch\n";
        return 1;
    }

    AudioTrimmer::VADConfig config;
    bool visualize = false;
    bool batchMode = false;
    std::vector<std::string> args(argv + 1, argv + argc);

    for (size_t i = 0; i < args.size(); ++i)
    {
        std::string_view arg = args[i];
        if (arg == "-v")
        {
            visualize = true;
        }
        else if (arg == "-t" && i + 1 < args.size())
        {
            try
            {
                config.silenceThreshold = std::stof(args[++i]);
            }
            catch (const std::invalid_argument &e)
            {
                std::cerr << "Error: Invalid number for threshold '" << args[i] << "'." << std::endl;
                return 1;
            }
        }
        else if (arg == "-batch")
        {
            batchMode = true;
        }
    }

    if (batchMode)
    {
        std::cout << "\nBatch processing all .wav files in ../data/master_calls/..." << std::endl;
        std::filesystem::path inputDir = "../data/master_calls";
        std::filesystem::path outputDir = inputDir / "trimmed";
        std::filesystem::create_directories(outputDir);

        for (const auto &entry : std::filesystem::directory_iterator(inputDir))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".wav")
            {
                std::filesystem::path outputPath = outputDir / entry.path().filename();
                AudioTrimmer::processAudioFile(entry.path(), outputPath, config, visualize);
            }
        }
    }
    else
    {
        std::filesystem::path inputPath = args[0];
        std::filesystem::path outputPath = "trimmed_output.wav";
        if (args.size() > 1 && args[1][0] != '-')
        {
            outputPath = args[1];
        }
        AudioTrimmer::processAudioFile(inputPath, outputPath, config, visualize);
    }

    return 0;
}