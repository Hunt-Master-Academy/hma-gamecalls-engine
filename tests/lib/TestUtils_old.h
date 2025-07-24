#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace huntmaster {
namespace test {

/**
 * @brief Centralized path management for test files
 *
 * Provides robust path resolution that works across different platforms
 * and build configurations, eliminating hardcoded relative paths.
 */
class TestPaths {
  public:
    /**
     * @brief Initialize test paths based on the current working directory
     *
     * Automatically detects if running from build directory, source directory,
     * or installed location and adjusts paths accordingly.
     */
    static void initialize();

    /**
     * @brief Get path to master calls directory
     * @return Absolute path to master calls directory
     */
    static std::filesystem::path getMasterCallsPath();

    /**
     * @brief Get path to test audio directory
     * @return Absolute path to test audio directory
     */
    static std::filesystem::path getTestAudioPath();

    /**
     * @brief Get path to test vectors directory
     * @return Absolute path to test vectors directory
     */
    static std::filesystem::path getTestVectorsPath();

    /**
     * @brief Get path to temporary test files directory
     * @return Absolute path to temporary directory, creates if needed
     */
    static std::filesystem::path getTempPath();

    /**
     * @brief Get path to specific master call file
     * @param callId Master call identifier (without extension)
     * @param extension File extension (.wav, .mfc, etc.)
     * @return Absolute path to master call file
     */
    static std::filesystem::path getMasterCallFile(const std::string& callId,
                                                   const std::string& extension = ".wav");

    /**
     * @brief Check if test data is available
     * @return true if basic test data structure exists
     */
    static bool hasTestData();

    /**
     * @brief Get list of available master call files
     * @param extension Filter by extension (empty for all)
     * @return Vector of available master call identifiers
     */
    static std::vector<std::string> getAvailableMasterCalls(const std::string& extension = ".wav");

    /**
     * @brief Clean up temporary test files
     */
    static void cleanup();

  private:
    static std::filesystem::path projectRoot_;
    static std::filesystem::path dataRoot_;
    static std::filesystem::path tempRoot_;
    static bool initialized_;

    /**
     * @brief Find project root by looking for characteristic files
     */
    static std::filesystem::path findProjectRoot();

    /**
     * @brief Ensure directory exists, create if needed
     */
    static void ensureDirectory(const std::filesystem::path& path);
};

/**
 * @brief Test data generator for creating synthetic test files
 *
 * Generates realistic test audio and feature files when real data
 * is not available, ensuring tests can run in any environment.
 */
class TestDataGenerator {
  public:
    struct AudioConfig {
        float sampleRate = 44100.0f;
        float duration = 2.0f;
        int channels = 1;
        float amplitude = 0.5f;
    };

    struct FeatureConfig {
        int numFrames = 100;
        int numCoeffs = 13;
        float frameTime = 0.025f;  // 25ms frames
    };

    /**
     * @brief Generate synthetic audio file for testing
     * @param filepath Output file path
     * @param config Audio configuration
     * @param pattern Pattern type (sine, noise, chirp, etc.)
     * @return true if generation successful
     */
    static bool generateAudioFile(
        const std::filesystem::path& filepath,
        const AudioConfig& config,
        const std::string& pattern = "sine"); /**
                                               * @brief Generate synthetic MFCC feature file
                                               * @param filepath Output file path
                                               * @param config Feature configuration
                                               * @param pattern Pattern type for features
                                               * @return true if generation successful
                                               */
    static bool
    generateFeatureFile(const std::filesystem::path& filepath,
                        const FeatureConfig& config,
                        const std::string& pattern =
                            "default"); /**
                                         * @brief Create complete test dataset
                                         * @param masterCallIds List of master call IDs to create
                                         * @return true if all files created successfully
                                         */
    static bool createTestDataset(const std::vector<std::string>& masterCallIds = {
                                      "test_call", "buck_grunt", "doe_grunt"});

    /**
     * @brief Generate realistic MFCC features
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
 */
class TestResourceManager {
  public:
    /**
     * @brief Constructor - sets up test environment
     */
    TestResourceManager();

    /**
     * @brief Destructor - cleans up test resources
     */
    ~TestResourceManager();

    /**
     * @brief Register temporary file for cleanup
     * @param filepath File to clean up on destruction
     */
    void registerTempFile(const std::filesystem::path& filepath);

    /**
     * @brief Register temporary directory for cleanup
     * @param dirpath Directory to clean up on destruction
     */
    void registerTempDirectory(const std::filesystem::path& dirpath);

    /**
     * @brief Create temporary file with unique name
     * @param prefix File prefix
     * @param suffix File suffix/extension
     * @return Path to created temporary file
     */
    std::filesystem::path createTempFile(const std::string& prefix = "test_",
                                         const std::string& suffix = ".tmp");

    /**
     * @brief Create temporary directory with unique name
     * @param prefix Directory prefix
     * @return Path to created temporary directory
     */
    std::filesystem::path createTempDirectory(const std::string& prefix = "test_");

    /**
     * @brief Ensure test data is available, generate if needed
     * @param masterCallIds Required master call IDs
     * @return true if test data is ready
     */
    bool ensureTestData(const std::vector<std::string>& masterCallIds = {"test_call",
                                                                         "buck_grunt"});

  private:
    std::vector<std::filesystem::path> tempFiles_;
    std::vector<std::filesystem::path> tempDirectories_;

    /**
     * @brief Generate unique identifier for temporary files
     */
    std::string generateUniqueId();
};

/**
 * @brief Cross-platform compatibility utilities
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
     * @brief Convert relative path to absolute based on reference
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
 * @brief Test fixture base class with common utilities
 *
 * Provides consistent setup and teardown for all test classes,
 * eliminating duplicated resource management code.
 */
class TestFixtureBase {
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
     * @brief Get resource manager for this test
     */
    TestResourceManager& getResourceManager() {
        return resourceManager_;
    }

    /**
     * @brief Skip test if required data is not available
     * @param masterCallId Required master call ID
     * @param message Skip message
     */
  protected:
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

  private:
    TestResourceManager resourceManager_;
};

}  // namespace test
}  // namespace huntmaster
