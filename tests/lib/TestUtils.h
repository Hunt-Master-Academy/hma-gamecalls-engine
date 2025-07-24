#ifndef HUNTMASTER_TEST_UTILS_H
#define HUNTMASTER_TEST_UTILS_H

#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace huntmaster {
namespace test {

/**
 * @brief Centralized path management for tests
 *
 * Provides robust, cross-platform path resolution that works
 * regardless of execution context or working directory.
 */
class TestPaths {
  public:
    /**
     * @brief Initialize path resolution system
     */
    static void initialize();

    /**
     * @brief Get master calls directory path
     */
    static std::filesystem::path getMasterCallsPath();

    /**
     * @brief Get test audio directory path
     */
    static std::filesystem::path getTestAudioPath();

    /**
     * @brief Get test vectors directory path
     */
    static std::filesystem::path getTestVectorsPath();

    /**
     * @brief Get temporary directory for this test session
     */
    static std::filesystem::path getTempPath();

    /**
     * @brief Get specific master call file path
     * @param callId Master call identifier
     * @param extension File extension (e.g., ".wav", ".mfc")
     */
    static std::filesystem::path getMasterCallFile(const std::string& callId,
                                                   const std::string& extension = ".wav");

    /**
     * @brief Check if test data directory structure exists
     */
    static bool hasTestData();

    /**
     * @brief Get list of available master calls
     * @param extension Filter by extension (empty = all files)
     */
    static std::vector<std::string> getAvailableMasterCalls(const std::string& extension = ".wav");

    /**
     * @brief Clean up temporary files/directories
     */
    static void cleanup();

  private:
    static std::filesystem::path findProjectRoot();
    static void ensureDirectory(const std::filesystem::path& path);

    static std::filesystem::path projectRoot_;
    static std::filesystem::path dataRoot_;
    static std::filesystem::path tempRoot_;
    static bool initialized_;
};

/**
 * @brief Test data generator for synthetic audio and features
 *
 * Creates consistent, reproducible test data when real data is not available.
 * Useful for CI environments and isolated testing.
 */
class TestDataGenerator {
  public:
    /**
     * @brief Audio generation configuration
     */
    struct AudioConfig {
        float sampleRate = 44100.0f;
        float duration = 2.0f;
        int channels = 1;
        float amplitude = 0.5f;
    };

    /**
     * @brief Feature generation configuration
     */
    struct FeatureConfig {
        int numFrames = 100;
        int numCoeffs = 13;
        float frameTime = 0.025f;  // 25ms frames
    };

    /**
     * @brief Generate synthetic audio file
     * @param filepath Output file path
     * @param config Audio configuration
     * @param pattern Pattern type ("sine", "noise", "chirp")
     * @return true if generation successful
     */
    static bool generateAudioFile(const std::filesystem::path& filepath,
                                  const AudioConfig& config,
                                  const std::string& pattern = "sine");

    /**
     * @brief Generate synthetic feature file
     * @param filepath Output file path
     * @param config Feature configuration
     * @param pattern Pattern type for features
     * @return true if generation successful
     */
    static bool generateFeatureFile(const std::filesystem::path& filepath,
                                    const FeatureConfig& config,
                                    const std::string& pattern = "default");

    /**
     * @brief Create complete test dataset
     * @param masterCallIds List of master call IDs to create
     * @return true if all files created successfully
     */
    static bool createTestDataset(const std::vector<std::string>& masterCallIds);

    /**
     * @brief Generate MFCC-like features
     * @param config Feature configuration
     * @param pattern Pattern type
     * @return Vector of MFCC feature frames
     */
    static std::vector<std::vector<float>>
    generateMFCCFeatures(const FeatureConfig& config, const std::string& pattern = "default");

  private:
    /**
     * @brief Generate sine wave audio data
     */
    static std::vector<float> generateSineWave(const AudioConfig& config, float frequency = 440.0f);

    /**
     * @brief Generate white noise audio data
     */
    static std::vector<float> generateNoise(const AudioConfig& config);

    /**
     * @brief Generate frequency sweep (chirp)
     */
    static std::vector<float>
    generateChirp(const AudioConfig& config, float startFreq = 100.0f, float endFreq = 1000.0f);

    /**
     * @brief Write WAV file
     */
    static bool writeWavFile(const std::filesystem::path& filepath,
                             const std::vector<float>& audioData,
                             const AudioConfig& config);
};

/**
 * @brief Test resource manager for automatic cleanup
 *
 * RAII-style resource management for test files and temporary data.
 * Automatically cleans up resources when tests complete.
 */
class TestResourceManager {
  public:
    TestResourceManager();
    ~TestResourceManager();

    /**
     * @brief Register temporary file for cleanup
     */
    void registerTempFile(const std::filesystem::path& filepath);

    /**
     * @brief Register temporary directory for cleanup
     */
    void registerTempDirectory(const std::filesystem::path& dirpath);

    /**
     * @brief Create temporary file with automatic cleanup
     * @param prefix Filename prefix
     * @param suffix Filename suffix (e.g., ".wav")
     * @return Path to created temporary file
     */
    std::filesystem::path createTempFile(const std::string& prefix = "test_",
                                         const std::string& suffix = ".tmp");

    /**
     * @brief Create temporary directory with automatic cleanup
     * @param prefix Directory name prefix
     * @return Path to created temporary directory
     */
    std::filesystem::path createTempDirectory(const std::string& prefix = "test_");

    /**
     * @brief Ensure test data exists, generate if missing
     * @param masterCallIds Required master call IDs
     * @return true if all data is available
     */
    bool ensureTestData(const std::vector<std::string>& masterCallIds);

  private:
    std::string generateUniqueId();

    std::vector<std::filesystem::path> tempFiles_;
    std::vector<std::filesystem::path> tempDirectories_;
};

/**
 * @brief Cross-platform utility functions
 *
 * Handles platform-specific path and file operations to ensure
 * consistent behavior across Windows, Linux, and macOS.
 */
class CrossPlatformUtils {
  public:
    /**
     * @brief Normalize path separators for current platform
     */
    static std::string normalizePath(const std::string& path);

    /**
     * @brief Check if path is absolute
     */
    static bool isAbsolutePath(const std::string& path);

    /**
     * @brief Convert relative path to absolute
     * @param relativePath Path to convert
     * @param reference Reference directory (default: current directory)
     */
    static std::filesystem::path
    makeAbsolute(const std::string& relativePath,
                 const std::filesystem::path& reference = std::filesystem::current_path());

    /**
     * @brief Get platform-specific temporary directory
     */
    static std::filesystem::path getTempDirectory();

    /**
     * @brief Safe file removal that doesn't throw on missing files
     */
    static bool safeRemove(const std::filesystem::path& path);

    /**
     * @brief Create directory hierarchy if it doesn't exist
     */
    static bool ensureDirectoryExists(const std::filesystem::path& path);
};

/**
 * @brief Base class for all test fixtures
 *
 * Provides common test infrastructure including path resolution,
 * test data generation, and resource cleanup.
 */
class TestFixtureBase : public ::testing::Test {
  public:
    TestFixtureBase();
    virtual ~TestFixtureBase();

  protected:
    /**
     * @brief Set up test environment (called automatically)
     */
    virtual void SetUp();

    /**
     * @brief Clean up test environment (called automatically)
     */
    virtual void TearDown();

    /**
     * @brief Skip test if master call data is missing
     * @param masterCallId Master call ID to check
     * @param message Custom message (optional)
     */
    void skipIfDataMissing(const std::string& masterCallId, const std::string& message = "");

    /**
     * @brief Get access to resource manager
     * @return Reference to resource manager
     */
    TestResourceManager& getResourceManager() {
        return resourceManager_;
    }

  private:
    TestResourceManager resourceManager_;
};

}  // namespace test
}  // namespace huntmaster

#endif  // HUNTMASTER_TEST_UTILS_H
