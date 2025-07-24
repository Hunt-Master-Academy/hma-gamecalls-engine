#include "TestUtils.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <random>

#include <gtest/gtest.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace huntmaster {
namespace test {

// Static member initialization
std::filesystem::path TestPaths::projectRoot_;
std::filesystem::path TestPaths::dataRoot_;
std::filesystem::path TestPaths::tempRoot_;
bool TestPaths::initialized_ = false;

// TestPaths implementation
void TestPaths::initialize() {
    if (initialized_)
        return;

    projectRoot_ = findProjectRoot();
    dataRoot_ = projectRoot_ / "data";
    tempRoot_ = getTempPath();

    // Ensure basic directory structure exists
    ensureDirectory(dataRoot_ / "master_calls");
    ensureDirectory(dataRoot_ / "test_audio");
    ensureDirectory(dataRoot_ / "test_vectors");
    ensureDirectory(tempRoot_);

    initialized_ = true;

    std::cout << "[TestPaths] Initialized:" << std::endl;
    std::cout << "  Project root: " << projectRoot_ << std::endl;
    std::cout << "  Data root: " << dataRoot_ << std::endl;
    std::cout << "  Temp root: " << tempRoot_ << std::endl;
}

std::filesystem::path TestPaths::getMasterCallsPath() {
    if (!initialized_)
        initialize();
    return dataRoot_ / "master_calls";
}

std::filesystem::path TestPaths::getTestAudioPath() {
    if (!initialized_)
        initialize();
    return dataRoot_ / "test_audio";
}

std::filesystem::path TestPaths::getTestVectorsPath() {
    if (!initialized_)
        initialize();
    return dataRoot_ / "test_vectors";
}

std::filesystem::path TestPaths::getTempPath() {
    if (!tempRoot_.empty())
        return tempRoot_;

    // Create unique temp directory for this test run
    auto baseTempDir = CrossPlatformUtils::getTempDirectory();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::system_clock::now().time_since_epoch())
                         .count();

    tempRoot_ = baseTempDir / ("huntmaster_test_" + std::to_string(timestamp));
    ensureDirectory(tempRoot_);

    return tempRoot_;
}

std::filesystem::path TestPaths::getMasterCallFile(const std::string& callId,
                                                   const std::string& extension) {
    return getMasterCallsPath() / (callId + extension);
}

bool TestPaths::hasTestData() {
    if (!initialized_)
        initialize();

    // Check if data directory structure exists
    return std::filesystem::exists(dataRoot_) && std::filesystem::exists(getMasterCallsPath());
}

std::vector<std::string> TestPaths::getAvailableMasterCalls(const std::string& extension) {
    std::vector<std::string> calls;

    if (!hasTestData())
        return calls;

    try {
        for (const auto& entry : std::filesystem::directory_iterator(getMasterCallsPath())) {
            if (entry.is_regular_file()) {
                auto filepath = entry.path();
                auto fileExtension = filepath.extension().string();

                if (extension.empty() || fileExtension == extension) {
                    calls.push_back(filepath.stem().string());
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[TestPaths] Error scanning master calls: " << e.what() << std::endl;
    }

    return calls;
}

void TestPaths::cleanup() {
    if (!tempRoot_.empty() && std::filesystem::exists(tempRoot_)) {
        try {
            std::filesystem::remove_all(tempRoot_);
            std::cout << "[TestPaths] Cleaned up temp directory: " << tempRoot_ << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[TestPaths] Error cleaning temp directory: " << e.what() << std::endl;
        }
    }
}

std::filesystem::path TestPaths::findProjectRoot() {
    std::filesystem::path current = std::filesystem::current_path();

    // Look for characteristic files that indicate project root
    const std::vector<std::string> markers = {
        "CMakeLists.txt", "huntmaster-engine.code-workspace", "README.md"};

    // Search up to 10 levels up
    for (int i = 0; i < 10; ++i) {
        for (const auto& marker : markers) {
            if (std::filesystem::exists(current / marker)) {
                // Additional validation - check for expected structure
                auto dataDir = current / "data";
                auto srcDir = current / "src";
                auto testsDir = current / "tests";

                if (std::filesystem::exists(dataDir) || std::filesystem::exists(srcDir)
                    || std::filesystem::exists(testsDir)) {
                    return current;
                }
            }
        }

        auto parent = current.parent_path();
        if (parent == current)
            break;  // Reached filesystem root
        current = parent;
    }

    // Fallback to current directory
    std::cout << "[TestPaths] Warning: Could not find project root, using current directory"
              << std::endl;
    return std::filesystem::current_path();
}

void TestPaths::ensureDirectory(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        try {
            std::filesystem::create_directories(path);
        } catch (const std::exception& e) {
            std::cerr << "[TestPaths] Error creating directory " << path << ": " << e.what()
                      << std::endl;
        }
    }
}

// TestDataGenerator implementation
bool TestDataGenerator::generateAudioFile(const std::filesystem::path& filepath,
                                          const AudioConfig& config,
                                          const std::string& pattern) {
    std::vector<float> audioData;

    if (pattern == "sine") {
        audioData = generateSineWave(config);
    } else if (pattern == "noise") {
        audioData = generateNoise(config);
    } else if (pattern == "chirp") {
        audioData = generateChirp(config);
    } else {
        // Default to sine wave
        audioData = generateSineWave(config);
    }

    return writeWavFile(filepath, audioData, config);
}

bool TestDataGenerator::generateFeatureFile(const std::filesystem::path& filepath,
                                            const FeatureConfig& config,
                                            const std::string& pattern) {
    auto features = generateMFCCFeatures(config, pattern);

    try {
        std::ofstream file(filepath, std::ios::binary);
        if (!file.is_open())
            return false;

        // Write header
        uint32_t numFrames = static_cast<uint32_t>(features.size());
        uint32_t numCoeffs = features.empty() ? 0 : static_cast<uint32_t>(features[0].size());

        file.write(reinterpret_cast<const char*>(&numFrames), sizeof(numFrames));
        file.write(reinterpret_cast<const char*>(&numCoeffs), sizeof(numCoeffs));

        // Write feature data
        for (const auto& frame : features) {
            file.write(reinterpret_cast<const char*>(frame.data()), frame.size() * sizeof(float));
        }

        return file.good();
    } catch (const std::exception& e) {
        std::cerr << "[TestDataGenerator] Error writing feature file: " << e.what() << std::endl;
        return false;
    }
}

bool TestDataGenerator::createTestDataset(const std::vector<std::string>& masterCallIds) {
    bool success = true;

    for (const auto& callId : masterCallIds) {
        // Generate audio file
        auto audioPath = TestPaths::getMasterCallFile(callId, ".wav");
        if (!std::filesystem::exists(audioPath)) {
            AudioConfig audioConfig;
            audioConfig.duration = 2.0f + (callId == "buck_grunt" ? 0.5f : 0.0f);  // Vary duration

            if (!generateAudioFile(audioPath, audioConfig, "sine")) {
                std::cerr << "[TestDataGenerator] Failed to generate audio for " << callId
                          << std::endl;
                success = false;
            }
        }

        // Generate feature file
        auto featurePath = TestPaths::getMasterCallFile(callId, ".mfc");
        if (!std::filesystem::exists(featurePath)) {
            FeatureConfig featureConfig;
            featureConfig.numFrames = 80 + (callId == "buck_grunt" ? 20 : 0);  // Vary frame count

            if (!generateFeatureFile(featurePath, featureConfig, "default")) {
                std::cerr << "[TestDataGenerator] Failed to generate features for " << callId
                          << std::endl;
                success = false;
            }
        }
    }

    return success;
}

std::vector<std::vector<float>>
TestDataGenerator::generateMFCCFeatures(const FeatureConfig& config, const std::string& pattern) {
    std::vector<std::vector<float>> features;
    features.reserve(config.numFrames);

    std::mt19937 rng(42);  // Fixed seed for reproducible tests
    std::normal_distribution<float> normal(0.0f, 1.0f);

    for (int frame = 0; frame < config.numFrames; ++frame) {
        std::vector<float> frameFeatures(config.numCoeffs);

        if (pattern == "default") {
            // Generate realistic MFCC-like features
            float t = static_cast<float>(frame) / config.numFrames;

            for (int coeff = 0; coeff < config.numCoeffs; ++coeff) {
                if (coeff == 0) {
                    // Energy coefficient - varies smoothly
                    frameFeatures[coeff] = 10.0f + 5.0f * std::sin(2.0f * M_PI * t * 3.0f);
                } else {
                    // Other coefficients - smaller values with some structure
                    float base = 2.0f * std::sin(2.0f * M_PI * t * (coeff + 1) * 0.5f);
                    frameFeatures[coeff] = base + 0.5f * normal(rng);
                }
            }
        } else {
            // Random features
            for (int coeff = 0; coeff < config.numCoeffs; ++coeff) {
                frameFeatures[coeff] = normal(rng);
            }
        }

        features.push_back(std::move(frameFeatures));
    }

    return features;
}

std::vector<float> TestDataGenerator::generateSineWave(const AudioConfig& config, float frequency) {
    int numSamples = static_cast<int>(config.sampleRate * config.duration);
    std::vector<float> audioData(numSamples);

    for (int i = 0; i < numSamples; ++i) {
        float t = static_cast<float>(i) / config.sampleRate;
        audioData[i] = config.amplitude * std::sin(2.0f * M_PI * frequency * t);
    }

    return audioData;
}

std::vector<float> TestDataGenerator::generateNoise(const AudioConfig& config) {
    int numSamples = static_cast<int>(config.sampleRate * config.duration);
    std::vector<float> audioData(numSamples);

    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(-config.amplitude, config.amplitude);

    for (int i = 0; i < numSamples; ++i) {
        audioData[i] = dist(rng);
    }

    return audioData;
}

std::vector<float>
TestDataGenerator::generateChirp(const AudioConfig& config, float startFreq, float endFreq) {
    int numSamples = static_cast<int>(config.sampleRate * config.duration);
    std::vector<float> audioData(numSamples);

    for (int i = 0; i < numSamples; ++i) {
        float t = static_cast<float>(i) / config.sampleRate;
        float progress = t / config.duration;
        float freq = startFreq + (endFreq - startFreq) * progress;
        audioData[i] = config.amplitude * std::sin(2.0f * M_PI * freq * t);
    }

    return audioData;
}

bool TestDataGenerator::writeWavFile(const std::filesystem::path& filepath,
                                     const std::vector<float>& audioData,
                                     const AudioConfig& config) {
    try {
        std::ofstream file(filepath, std::ios::binary);
        if (!file.is_open())
            return false;

        // WAV file header
        struct WAVHeader {
            char riff[4] = {'R', 'I', 'F', 'F'};
            uint32_t chunkSize;
            char wave[4] = {'W', 'A', 'V', 'E'};
            char fmt[4] = {'f', 'm', 't', ' '};
            uint32_t subChunk1Size = 16;
            uint16_t audioFormat = 3;  // IEEE float
            uint16_t numChannels;
            uint32_t sampleRate;
            uint32_t byteRate;
            uint16_t blockAlign;
            uint16_t bitsPerSample = 32;
            char data[4] = {'d', 'a', 't', 'a'};
            uint32_t subChunk2Size;
        };

        WAVHeader header;
        header.numChannels = static_cast<uint16_t>(config.channels);
        header.sampleRate = static_cast<uint32_t>(config.sampleRate);
        header.byteRate = header.sampleRate * header.numChannels * (header.bitsPerSample / 8);
        header.blockAlign = header.numChannels * (header.bitsPerSample / 8);
        header.subChunk2Size = static_cast<uint32_t>(audioData.size() * sizeof(float));
        header.chunkSize = 36 + header.subChunk2Size;

        // Write header
        file.write(reinterpret_cast<const char*>(&header), sizeof(header));

        // Write audio data
        file.write(reinterpret_cast<const char*>(audioData.data()),
                   audioData.size() * sizeof(float));

        return file.good();
    } catch (const std::exception& e) {
        std::cerr << "[TestDataGenerator] Error writing WAV file: " << e.what() << std::endl;
        return false;
    }
}

// TestResourceManager implementation
TestResourceManager::TestResourceManager() {
    TestPaths::initialize();
}

TestResourceManager::~TestResourceManager() {
    // Clean up temporary files
    for (const auto& file : tempFiles_) {
        CrossPlatformUtils::safeRemove(file);
    }

    // Clean up temporary directories
    for (const auto& dir : tempDirectories_) {
        try {
            if (std::filesystem::exists(dir)) {
                std::filesystem::remove_all(dir);
            }
        } catch (const std::exception& e) {
            std::cerr << "[TestResourceManager] Error removing temp directory: " << e.what()
                      << std::endl;
        }
    }
}

void TestResourceManager::registerTempFile(const std::filesystem::path& filepath) {
    tempFiles_.push_back(filepath);
}

void TestResourceManager::registerTempDirectory(const std::filesystem::path& dirpath) {
    tempDirectories_.push_back(dirpath);
}

std::filesystem::path TestResourceManager::createTempFile(const std::string& prefix,
                                                          const std::string& suffix) {
    auto tempDir = TestPaths::getTempPath();
    auto uniqueId = generateUniqueId();
    auto filepath = tempDir / (prefix + uniqueId + suffix);

    registerTempFile(filepath);
    return filepath;
}

std::filesystem::path TestResourceManager::createTempDirectory(const std::string& prefix) {
    auto tempDir = TestPaths::getTempPath();
    auto uniqueId = generateUniqueId();
    auto dirpath = tempDir / (prefix + uniqueId);

    CrossPlatformUtils::ensureDirectoryExists(dirpath);
    registerTempDirectory(dirpath);
    return dirpath;
}

bool TestResourceManager::ensureTestData(const std::vector<std::string>& masterCallIds) {
    return TestDataGenerator::createTestDataset(masterCallIds);
}

std::string TestResourceManager::generateUniqueId() {
    static std::atomic<int> counter{0};
    auto timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
                         std::chrono::steady_clock::now().time_since_epoch())
                         .count();

    return std::to_string(timestamp) + "_" + std::to_string(counter.fetch_add(1));
}

// CrossPlatformUtils implementation
std::string CrossPlatformUtils::normalizePath(const std::string& path) {
    std::string normalized = path;
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    return normalized;
}

bool CrossPlatformUtils::isAbsolutePath(const std::string& path) {
    std::filesystem::path p(path);
    return p.is_absolute();
}

std::filesystem::path CrossPlatformUtils::makeAbsolute(const std::string& relativePath,
                                                       const std::filesystem::path& reference) {
    std::filesystem::path p(relativePath);
    if (p.is_absolute()) {
        return p;
    }
    return std::filesystem::absolute(reference / p);
}

std::filesystem::path CrossPlatformUtils::getTempDirectory() {
    try {
        return std::filesystem::temp_directory_path();
    } catch (const std::exception&) {
// Fallback options
#ifdef _WIN32
        return "C:\\temp";
#else
        return "/tmp";
#endif
    }
}

bool CrossPlatformUtils::safeRemove(const std::filesystem::path& path) {
    try {
        if (std::filesystem::exists(path)) {
            return std::filesystem::remove(path);
        }
        return true;  // File doesn't exist, consider it removed
    } catch (const std::exception&) {
        return false;
    }
}

bool CrossPlatformUtils::ensureDirectoryExists(const std::filesystem::path& path) {
    try {
        return std::filesystem::create_directories(path) || std::filesystem::exists(path);
    } catch (const std::exception&) {
        return false;
    }
}

// TestFixtureBase implementation
TestFixtureBase::TestFixtureBase() = default;

TestFixtureBase::~TestFixtureBase() = default;

void TestFixtureBase::SetUp() {
    TestPaths::initialize();
}

void TestFixtureBase::TearDown() {
    // Resource manager destructor will handle cleanup
}

void TestFixtureBase::skipIfDataMissing(const std::string& masterCallId,
                                        const std::string& message) {
    auto audioPath = TestPaths::getMasterCallFile(masterCallId, ".wav");
    auto featurePath = TestPaths::getMasterCallFile(masterCallId, ".mfc");

    if (!std::filesystem::exists(audioPath) && !std::filesystem::exists(featurePath)) {
        // Try to generate test data
        if (!resourceManager_.ensureTestData({masterCallId})) {
            std::string skipMessage =
                message.empty() ? ("Test data for " + masterCallId + " not available") : message;
            GTEST_SKIP() << skipMessage;
        }
    }
}

}  // namespace test
}  // namespace huntmaster
