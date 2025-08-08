#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "huntmaster/core/MFCCProcessor.h"
#include "huntmaster/core/UnifiedAudioEngine.h"
#include "huntmaster/io/AudioLoader.h"

namespace fs = std::filesystem;
using json = nlohmann::json;

class MasterCallProcessor {
  public:
    struct CallMetadata {
        std::string species;
        std::string callType;
        std::string season;
        int difficulty;
        float duration;
        float dominantFreq;
        std::string description;
    };

    bool processDirectory(const std::string& inputDir, const std::string& outputDir) {
        std::cout << "Processing master calls from: " << inputDir << std::endl;

        // Create output directory structure
        fs::create_directories(outputDir + "/mfc");
        fs::create_directories(outputDir + "/waveforms");
        fs::create_directories(outputDir + "/metadata");

        int processed = 0;
        int failed = 0;

        // Process each audio file
        for (const auto& entry : fs::recursive_directory_iterator(inputDir)) {
            if (entry.is_regular_file()) {
                auto ext = entry.path().extension().string();
                if (ext == ".wav" || ext == ".mp3") {
                    if (processFile(entry.path(), outputDir)) {
                        processed++;
                    } else {
                        failed++;
                    }
                }
            }
        }

        std::cout << "Processed: " << processed << ", Failed: " << failed << std::endl;
        return failed == 0;
    }

  private:
    bool processFile(const fs::path& audioPath, const std::string& outputDir) {
        std::cout << "Processing: " << audioPath.filename() << std::endl;

        try {
            // Load audio
            huntmaster::AudioLoader loader;
            auto audioData = loader.load(audioPath.string());
            if (!audioData.has_value()) {
                std::cerr << "Failed to load audio: " << audioPath << std::endl;
                return false;
            }

            // Extract MFCC features
            huntmaster::MFCCProcessor::Config mfccConfig;
            mfccConfig.sample_rate = audioData.value().sampleRate;
            mfccConfig.frame_size = 512;
            mfccConfig.hop_size = 256;
            mfccConfig.num_filters = 40;
            mfccConfig.num_coefficients = 13;

            huntmaster::MFCCProcessor processor(mfccConfig);
            auto features = processor.extractFeatures(audioData.value().samples);

            if (!features.has_value()) {
                std::cerr << "Failed to extract features: " << audioPath << std::endl;
                return false;
            }

            // Save MFC file
            std::string baseName = audioPath.stem().string();
            std::string mfcPath = outputDir + "/mfc/" + baseName + ".mfc";
            saveMFCFile(mfcPath, features.value());

            // Generate and save waveform data
            std::string waveformPath = outputDir + "/waveforms/" + baseName + ".json";
            generateWaveformData(audioData.value().samples, waveformPath);

            // Generate metadata
            CallMetadata metadata = analyzeCall(audioData.value(), baseName);
            std::string metadataPath = outputDir + "/metadata/" + baseName + ".json";
            saveMetadata(metadataPath, metadata);

            std::cout << "✓ Completed: " << baseName << std::endl;
            return true;

        } catch (const std::exception& e) {
            std::cerr << "Error processing " << audioPath << ": " << e.what() << std::endl;
            return false;
        }
    }

    void saveMFCFile(const std::string& path, const std::vector<std::vector<float>>& features) {
        std::ofstream file(path, std::ios::binary);
        int numFrames = features.size();
        int numCoeffs = features.empty() ? 0 : features[0].size();

        file.write(reinterpret_cast<const char*>(&numFrames), sizeof(int));
        file.write(reinterpret_cast<const char*>(&numCoeffs), sizeof(int));

        for (const auto& frame : features) {
            file.write(reinterpret_cast<const char*>(frame.data()), frame.size() * sizeof(float));
        }
    }

    void generateWaveformData(const std::vector<float>& samples, const std::string& path) {
        json waveform;

        // Generate multiple resolution levels for efficient rendering
        std::vector<int> resolutions = {100, 500, 1000, 5000};

        for (int resolution : resolutions) {
            int step = std::max(1, static_cast<int>(samples.size() / resolution));
            std::vector<float> min_values, max_values, rms_values;

            for (size_t i = 0; i < samples.size(); i += step) {
                float min_val = samples[i], max_val = samples[i], sum = 0;
                int count = 0;

                for (size_t j = i; j < std::min(i + step, samples.size()); j++) {
                    min_val = std::min(min_val, samples[j]);
                    max_val = std::max(max_val, samples[j]);
                    sum += samples[j] * samples[j];
                    count++;
                }

                min_values.push_back(min_val);
                max_values.push_back(max_val);
                rms_values.push_back(std::sqrt(sum / count));
            }

            waveform["resolutions"][std::to_string(resolution)] = {
                {"min", min_values}, {"max", max_values}, {"rms", rms_values}};
        }

        waveform["duration"] = samples.size() / 44100.0;  // Assuming 44.1kHz
        waveform["samples"] = samples.size();

        std::ofstream file(path);
        file << waveform.dump(2);
    }

    CallMetadata analyzeCall(const huntmaster::AudioData& audio, const std::string& fileName) {
        CallMetadata metadata;

        // Parse filename for species and call type
        if (fileName.find("turkey") != std::string::npos) {
            metadata.species = "turkey";
            if (fileName.find("gobble") != std::string::npos) {
                metadata.callType = "gobble";
                metadata.season = "spring";
                metadata.difficulty = 3;
                metadata.description = "Male turkey mating call";
            } else if (fileName.find("yelp") != std::string::npos) {
                metadata.callType = "yelp";
                metadata.season = "all";
                metadata.difficulty = 2;
                metadata.description = "Basic turkey communication";
            } else if (fileName.find("cluck") != std::string::npos) {
                metadata.callType = "cluck";
                metadata.season = "all";
                metadata.difficulty = 1;
                metadata.description = "Short turkey sound";
            }
        } else if (fileName.find("deer") != std::string::npos
                   || fileName.find("buck") != std::string::npos
                   || fileName.find("doe") != std::string::npos) {
            metadata.species = "white-tail deer";
            if (fileName.find("grunt") != std::string::npos) {
                metadata.callType = "grunt";
                metadata.season = "rut";
                metadata.difficulty = 2;
                metadata.description = "Buck grunt during rut";
            } else if (fileName.find("bleat") != std::string::npos) {
                metadata.callType = "bleat";
                metadata.season = "all";
                metadata.difficulty = 1;
                metadata.description = "Doe or fawn communication";
            }
        }

        metadata.duration = audio.samples.size() / static_cast<float>(audio.sampleRate);

        // Simple dominant frequency estimation (placeholder)
        metadata.dominantFreq = 250.0;  // Would use FFT in production

        return metadata;
    }

    void saveMetadata(const std::string& path, const CallMetadata& metadata) {
        json j;
        j["species"] = metadata.species;
        j["callType"] = metadata.callType;
        j["season"] = metadata.season;
        j["difficulty"] = metadata.difficulty;
        j["duration"] = metadata.duration;
        j["dominantFreq"] = metadata.dominantFreq;
        j["description"] = metadata.description;
        j["processedAt"] = std::time(nullptr);

        std::ofstream file(path);
        file << j.dump(2);
    }
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input_dir> <output_dir>" << std::endl;
        return 1;
    }

    MasterCallProcessor processor;
    bool success = processor.processDirectory(argv[1], argv[2]);

    return success ? 0 : 1;
}
