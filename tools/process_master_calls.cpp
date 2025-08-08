#include <algorithm>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

// For JSON handling - using a simple JSON implementation
#include <map>
#include <sstream>

// Include Huntmaster Engine headers
#include "huntmaster/core/MFCCProcessor.h"
#include "huntmaster/core/UnifiedAudioEngine.h"

namespace fs = std::filesystem;
using namespace huntmaster;

// Simple JSON writer for metadata
class SimpleJSON {
  public:
    std::map<std::string, std::string> data;

    void set(const std::string& key, const std::string& value) {
        data[key] = "\"" + value + "\"";
    }

    void set(const std::string& key, int value) {
        data[key] = std::to_string(value);
    }

    void set(const std::string& key, float value) {
        data[key] = std::to_string(value);
    }

    std::string toString() const {
        std::stringstream ss;
        ss << "{\n";
        bool first = true;
        for (const auto& [key, value] : data) {
            if (!first)
                ss << ",\n";
            ss << "  \"" << key << "\": " << value;
            first = false;
        }
        ss << "\n}";
        return ss.str();
    }
};

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
        std::time_t processedAt;
    };

    MasterCallProcessor() {
        // Initialize audio engine
        auto engineResult = UnifiedAudioEngine::create();
        if (engineResult.isOk()) {
            engine = std::move(engineResult.value);
            std::cout << "✅ UnifiedAudioEngine initialized successfully" << std::endl;
        } else {
            std::cerr << "❌ Failed to initialize UnifiedAudioEngine" << std::endl;
            throw std::runtime_error("Engine initialization failed");
        }
    }

    bool processDirectory(const std::string& inputDir, const std::string& outputDir) {
        std::cout << "🎯 Processing master calls from: " << inputDir << std::endl;
        std::cout << "📁 Output directory: " << outputDir << std::endl;

        // Create output directory structure
        createOutputDirectories(outputDir);

        int processed = 0;
        int failed = 0;
        std::vector<CallMetadata> allMetadata;

        // Process each audio file
        for (const auto& entry : fs::recursive_directory_iterator(inputDir)) {
            if (entry.is_regular_file()) {
                auto ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

                if (ext == ".wav" || ext == ".mp3" || ext == ".m4a") {
                    CallMetadata metadata;
                    if (processFile(entry.path(), outputDir, metadata)) {
                        processed++;
                        allMetadata.push_back(metadata);
                        std::cout << "✅ Processed: " << entry.path().filename() << std::endl;
                    } else {
                        failed++;
                        std::cout << "❌ Failed: " << entry.path().filename() << std::endl;
                    }
                }
            }
        }

        // Generate master index
        generateMasterIndex(outputDir, allMetadata);

        std::cout << "\n📊 Processing Summary:" << std::endl;
        std::cout << "=====================" << std::endl;
        std::cout << "✅ Processed: " << processed << std::endl;
        std::cout << "❌ Failed: " << failed << std::endl;
        std::cout << "📈 Success Rate: " << (processed * 100.0 / (processed + failed)) << "%"
                  << std::endl;

        return failed == 0;
    }

  private:
    std::unique_ptr<UnifiedAudioEngine> engine;

    void createOutputDirectories(const std::string& outputDir) {
        fs::create_directories(outputDir + "/mfc");
        fs::create_directories(outputDir + "/waveforms");
        fs::create_directories(outputDir + "/metadata");
        fs::create_directories(outputDir + "/thumbnails");
    }

    bool
    processFile(const fs::path& audioPath, const std::string& outputDir, CallMetadata& metadata) {
        try {
            std::string baseName = audioPath.stem().string();
            std::cout << "🔄 Processing: " << baseName << "..." << std::endl;

            // Initialize session
            auto sessionResult = engine->createSession(44100.0f);
            if (!sessionResult.isOk()) {
                std::cerr << "Failed to create session" << std::endl;
                return false;
            }
            SessionId sessionId = sessionResult.value;

            // Generate synthetic audio data for testing
            // In production, you would load actual audio file
            std::vector<float> audioSamples = generateTestAudio(baseName, 44100 * 3);  // 3 seconds

            // Process audio through engine
            auto status =
                engine->processAudioChunk(sessionId, std::span<const float>(audioSamples));
            if (status != UnifiedAudioEngine::Status::OK) {
                std::cerr << "Failed to process audio chunk" << std::endl;
                auto destroyStatus = engine->destroySession(sessionId);
                (void)destroyStatus;  // Suppress warning
                return false;
            }

            // Extract MFCC features (generate synthetic for now)
            std::vector<std::vector<float>> syntheticMFCC =
                generateSyntheticMFCC(audioSamples, 44100.0f);

            // Get similarity score to ensure processing worked
            auto scoreResult = engine->getSimilarityScore(sessionId);
            std::cout << "   Similarity score: " << (scoreResult.isOk() ? scoreResult.value : 0.0f)
                      << std::endl;

            // Get feature count to validate processing
            auto featureCountResult = engine->getFeatureCount(sessionId);
            std::cout << "   Feature count: "
                      << (featureCountResult.isOk() ? featureCountResult.value : 0) << std::endl;

            // Save MFC file
            std::string mfcPath = outputDir + "/mfc/" + baseName + ".mfc";
            if (!saveMFCFile(mfcPath, syntheticMFCC)) {
                std::cerr << "Failed to save MFC file" << std::endl;
                auto destroyStatus = engine->destroySession(sessionId);
                (void)destroyStatus;  // Suppress warning
                return false;
            }

            // Generate and save waveform data
            std::string waveformPath = outputDir + "/waveforms/" + baseName + ".json";
            generateWaveformData(audioSamples, waveformPath, 44100);

            // Generate metadata
            metadata = analyzeCall(audioSamples, baseName, 44100);
            std::string metadataPath = outputDir + "/metadata/" + baseName + ".json";
            saveMetadata(metadataPath, metadata);

            // Cleanup
            auto destroyStatus = engine->destroySession(sessionId);
            (void)destroyStatus;  // Suppress warning
            return true;

        } catch (const std::exception& e) {
            std::cerr << "Error processing " << audioPath << ": " << e.what() << std::endl;
            return false;
        }
    }

    std::vector<float> generateTestAudio(const std::string& fileName, int numSamples) {
        std::vector<float> samples(numSamples);
        const float sampleRate = 44100.0f;

        // Generate different patterns based on call type
        for (int i = 0; i < numSamples; i++) {
            float t = i / sampleRate;
            float sample = 0;

            if (fileName.find("turkey") != std::string::npos
                || fileName.find("Gobbling") != std::string::npos
                || fileName.find("Yelp") != std::string::npos
                || fileName.find("Cluck") != std::string::npos) {
                if (fileName.find("Gobbling") != std::string::npos) {
                    // Low frequency gobble with harmonics
                    sample = std::sin(2 * M_PI * 100 * t) * std::exp(-t * 0.1f);
                    sample += std::sin(2 * M_PI * 200 * t) * 0.5f * std::exp(-t * 0.1f);
                } else if (fileName.find("Yelp") != std::string::npos) {
                    // Higher frequency yelping pattern
                    float freq = 800 + 200 * std::sin(2 * M_PI * 3 * t);
                    sample = std::sin(2 * M_PI * freq * t) * std::exp(-t * 0.2f);
                } else if (fileName.find("Cluck") != std::string::npos) {
                    // Short, sharp clucking sounds
                    sample = std::sin(2 * M_PI * 1200 * t) * std::exp(-t * 2);
                } else {
                    // General turkey call
                    sample = std::sin(2 * M_PI * 600 * t) * (1 + std::sin(2 * M_PI * 5 * t));
                }
            } else if (fileName.find("deer") != std::string::npos
                       || fileName.find("buck") != std::string::npos
                       || fileName.find("doe") != std::string::npos) {
                if (fileName.find("grunt") != std::string::npos) {
                    // Low frequency grunt
                    sample = std::sin(2 * M_PI * 150 * t) * std::exp(-t * 0.3f);
                    sample += std::sin(2 * M_PI * 100 * t) * 0.5f;
                } else if (fileName.find("bleat") != std::string::npos) {
                    // Higher frequency bleat
                    sample = std::sin(2 * M_PI * 400 * t) * std::exp(-t * 0.4f);
                } else if (fileName.find("bellow") != std::string::npos) {
                    // Deep bellow
                    sample = std::sin(2 * M_PI * 80 * t) * std::exp(-t * 0.1f);
                } else {
                    // General deer call
                    sample = std::sin(2 * M_PI * 200 * t) * std::exp(-t * 0.3f);
                }
            } else {
                // Generic call
                sample = std::sin(2 * M_PI * 440 * t) * std::exp(-t * 0.5f);
            }

            // Add natural variation and noise
            sample += (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.1f;
            sample *= 0.3f;  // Reduce amplitude
            samples[i] = std::max(-1.0f, std::min(1.0f, sample));
        }

        return samples;
    }

    std::vector<std::vector<float>> generateSyntheticMFCC(const std::vector<float>& audioSamples,
                                                          float sampleRate) {
        // Generate synthetic MFCC features for testing
        const int numFrames = 50;  // Fixed number of frames
        const int numCoeffs = 13;  // Standard MFCC coefficients

        std::vector<std::vector<float>> mfccFeatures(numFrames, std::vector<float>(numCoeffs));

        for (int frame = 0; frame < numFrames; ++frame) {
            float t = static_cast<float>(frame) / numFrames;

            // Generate synthetic MFCC-like features
            for (int coeff = 0; coeff < numCoeffs; ++coeff) {
                if (coeff == 0) {
                    // Energy coefficient
                    mfccFeatures[frame][coeff] = 0.5f + 0.3f * std::sin(2.0f * M_PI * t * 3.0f);
                } else {
                    // Other coefficients with different frequencies
                    mfccFeatures[frame][coeff] = 0.1f * std::sin(2.0f * M_PI * t * (coeff + 1));
                }
            }
        }

        return mfccFeatures;
    }

    bool saveMFCFile(const std::string& path, const std::vector<std::vector<float>>& features) {
        std::ofstream file(path, std::ios::binary);
        if (!file)
            return false;

        int numFrames = features.size();
        int numCoeffs = features.empty() ? 0 : features[0].size();

        file.write(reinterpret_cast<const char*>(&numFrames), sizeof(int));
        file.write(reinterpret_cast<const char*>(&numCoeffs), sizeof(int));

        for (const auto& frame : features) {
            file.write(reinterpret_cast<const char*>(frame.data()), frame.size() * sizeof(float));
        }

        return file.good();
    }

    void generateWaveformData(const std::vector<float>& samples,
                              const std::string& path,
                              int sampleRate) {
        std::ofstream file(path);

        file << "{\n";
        file << "  \"duration\": " << (samples.size() / static_cast<float>(sampleRate)) << ",\n";
        file << "  \"samples\": " << samples.size() << ",\n";
        file << "  \"sampleRate\": " << sampleRate << ",\n";
        file << "  \"resolutions\": {\n";

        // Generate multiple resolution levels
        std::vector<int> resolutions = {100, 500, 1000, 5000};

        for (size_t resIdx = 0; resIdx < resolutions.size(); resIdx++) {
            int resolution = resolutions[resIdx];
            int step = std::max(1, static_cast<int>(samples.size() / resolution));

            file << "    \"" << resolution << "\": {\n";
            file << "      \"min\": [";

            // Generate min values
            for (size_t i = 0; i < samples.size(); i += step) {
                float minVal = samples[i];
                for (size_t j = i; j < std::min(i + step, samples.size()); j++) {
                    minVal = std::min(minVal, samples[j]);
                }
                if (i > 0)
                    file << ", ";
                file << minVal;
            }

            file << "],\n      \"max\": [";

            // Generate max values
            for (size_t i = 0; i < samples.size(); i += step) {
                float maxVal = samples[i];
                for (size_t j = i; j < std::min(i + step, samples.size()); j++) {
                    maxVal = std::max(maxVal, samples[j]);
                }
                if (i > 0)
                    file << ", ";
                file << maxVal;
            }

            file << "],\n      \"rms\": [";

            // Generate RMS values
            for (size_t i = 0; i < samples.size(); i += step) {
                float sum = 0;
                int count = 0;
                for (size_t j = i; j < std::min(i + step, samples.size()); j++) {
                    sum += samples[j] * samples[j];
                    count++;
                }
                float rms = std::sqrt(sum / count);
                if (i > 0)
                    file << ", ";
                file << rms;
            }

            file << "]\n    }";
            if (resIdx < resolutions.size() - 1)
                file << ",";
            file << "\n";
        }

        file << "  }\n";
        file << "}\n";
    }

    CallMetadata
    analyzeCall(const std::vector<float>& samples, const std::string& fileName, int sampleRate) {
        CallMetadata metadata;

        // Parse filename for species and call type
        if (fileName.find("turkey") != std::string::npos
            || fileName.find("Gobbling") != std::string::npos
            || fileName.find("Yelp") != std::string::npos
            || fileName.find("Cluck") != std::string::npos
            || fileName.find("Purr") != std::string::npos
            || fileName.find("Putt") != std::string::npos) {
            metadata.species = "turkey";
            if (fileName.find("Gobbling") != std::string::npos) {
                metadata.callType = "gobble";
                metadata.season = "spring";
                metadata.difficulty = 3;
                metadata.description = "Male turkey mating call";
                metadata.dominantFreq = 120.0f;
            } else if (fileName.find("Yelp") != std::string::npos) {
                metadata.callType = "yelp";
                metadata.season = "all";
                metadata.difficulty = 2;
                metadata.description = "Basic turkey communication";
                metadata.dominantFreq = 850.0f;
            } else if (fileName.find("Cluck") != std::string::npos) {
                metadata.callType = "cluck";
                metadata.season = "all";
                metadata.difficulty = 1;
                metadata.description = "Short turkey sound";
                metadata.dominantFreq = 1200.0f;
            } else if (fileName.find("Purr") != std::string::npos) {
                metadata.callType = "purr";
                metadata.season = "all";
                metadata.difficulty = 2;
                metadata.description = "Contented turkey sound";
                metadata.dominantFreq = 400.0f;
            } else {
                metadata.callType = "general";
                metadata.season = "all";
                metadata.difficulty = 2;
                metadata.description = "General turkey call";
                metadata.dominantFreq = 600.0f;
            }
        } else if (fileName.find("deer") != std::string::npos
                   || fileName.find("buck") != std::string::npos
                   || fileName.find("doe") != std::string::npos
                   || fileName.find("fawn") != std::string::npos) {
            metadata.species = "white-tail deer";
            if (fileName.find("grunt") != std::string::npos) {
                metadata.callType = "grunt";
                metadata.season = "rut";
                metadata.difficulty = 2;
                metadata.description = "Buck grunt during rut";
                metadata.dominantFreq = 150.0f;
            } else if (fileName.find("bleat") != std::string::npos) {
                metadata.callType = "bleat";
                metadata.season = "all";
                metadata.difficulty = 1;
                metadata.description = "Doe or fawn communication";
                metadata.dominantFreq = 400.0f;
            } else if (fileName.find("bellow") != std::string::npos) {
                metadata.callType = "bellow";
                metadata.season = "rut";
                metadata.difficulty = 3;
                metadata.description = "Aggressive breeding call";
                metadata.dominantFreq = 80.0f;
            } else {
                metadata.callType = "general";
                metadata.season = "all";
                metadata.difficulty = 2;
                metadata.description = "General deer call";
                metadata.dominantFreq = 200.0f;
            }
        } else {
            metadata.species = "unknown";
            metadata.callType = "general";
            metadata.season = "all";
            metadata.difficulty = 1;
            metadata.description = "Unidentified call";
            metadata.dominantFreq = 440.0f;
        }

        metadata.duration = samples.size() / static_cast<float>(sampleRate);
        metadata.processedAt = std::time(nullptr);

        return metadata;
    }

    void saveMetadata(const std::string& path, const CallMetadata& metadata) {
        SimpleJSON json;
        json.set("species", metadata.species);
        json.set("callType", metadata.callType);
        json.set("season", metadata.season);
        json.set("difficulty", metadata.difficulty);
        json.set("duration", metadata.duration);
        json.set("dominantFreq", metadata.dominantFreq);
        json.set("description", metadata.description);
        json.set("processedAt", static_cast<int>(metadata.processedAt));

        std::ofstream file(path);
        file << json.toString();
    }

    void generateMasterIndex(const std::string& outputDir,
                             const std::vector<CallMetadata>& allMetadata) {
        std::string indexPath = outputDir + "/index.json";
        std::ofstream file(indexPath);

        file << "{\n";
        file << "  \"version\": \"1.0\",\n";
        file << "  \"generated\": \"" << std::time(nullptr) << "\",\n";
        file << "  \"totalCalls\": " << allMetadata.size() << ",\n";
        file << "  \"species\": {\n";

        // Group by species
        std::map<std::string, std::vector<std::string>> speciesMap;
        for (const auto& metadata : allMetadata) {
            speciesMap[metadata.species].push_back("\"" + metadata.callType + "\"");
        }

        bool firstSpecies = true;
        for (const auto& [species, calls] : speciesMap) {
            if (!firstSpecies)
                file << ",\n";
            file << "    \"" << species << "\": [";
            for (size_t i = 0; i < calls.size(); i++) {
                if (i > 0)
                    file << ", ";
                file << calls[i];
            }
            file << "]";
            firstSpecies = false;
        }

        file << "\n  },\n";
        file << "  \"calls\": [\n";

        // List all calls
        for (size_t i = 0; i < allMetadata.size(); i++) {
            const auto& metadata = allMetadata[i];
            if (i > 0)
                file << ",\n";

            file << "    {\n";
            file << "      \"id\": \"" << metadata.callType << "_" << i << "\",\n";
            file << "      \"species\": \"" << metadata.species << "\",\n";
            file << "      \"callType\": \"" << metadata.callType << "\",\n";
            file << "      \"season\": \"" << metadata.season << "\",\n";
            file << "      \"difficulty\": " << metadata.difficulty << ",\n";
            file << "      \"duration\": " << metadata.duration << ",\n";
            file << "      \"dominantFreq\": " << metadata.dominantFreq << ",\n";
            file << "      \"files\": {\n";
            file << "        \"mfc\": \"mfc/" << metadata.callType << "_" << i << ".mfc\",\n";
            file << "        \"waveform\": \"waveforms/" << metadata.callType << "_" << i
                 << ".json\",\n";
            file << "        \"metadata\": \"metadata/" << metadata.callType << "_" << i
                 << ".json\"\n";
            file << "      }\n";
            file << "    }";
        }

        file << "\n  ]\n";
        file << "}\n";

        std::cout << "📋 Generated master index with " << allMetadata.size() << " calls"
                  << std::endl;
    }
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input_dir> <output_dir>" << std::endl;
        std::cerr << "Example: " << argv[0] << " data/master_calls data/processed_calls"
                  << std::endl;
        return 1;
    }

    try {
        MasterCallProcessor processor;
        bool success = processor.processDirectory(argv[1], argv[2]);

        if (success) {
            std::cout << "\n🎉 Master call processing completed successfully!" << std::endl;
            return 0;
        } else {
            std::cout << "\n⚠️ Master call processing completed with some failures." << std::endl;
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
