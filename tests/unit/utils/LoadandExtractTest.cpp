#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <vector>

#include "TestUtils.h"
#include "dr_wav.h"
#include "huntmaster/core/DebugConfig.h"
#include "huntmaster/core/DebugLogger.h"
#include "huntmaster/core/UnifiedAudioEngine.h"

using namespace huntmaster;
using namespace huntmaster::test;
/**

@brief Debug test to investigate DTW feature extraction issues

This test loads an audio file and processes it through the engine to
diagnose why only 3 features are being extracted instead of 42.
*/
class DTWDebugTest {
  public:
    DTWDebugTest() {
        // Enable comprehensive debugging
        DebugConfig::setupFullDebug();
        auto& logger = DebugLogger::getInstance();
        logger.setComponentLogLevel(Component::UNIFIED_ENGINE, LogLevel::TRACE);
        logger.setComponentLogLevel(Component::MFCC_PROCESSOR, LogLevel::TRACE);
        logger.setComponentLogLevel(Component::REALTIME_PROCESSOR, LogLevel::TRACE);
        logger.setComponentLogLevel(Component::DTW_COMPARATOR, LogLevel::TRACE);
    }

    bool run(const std::string& audioFile = "", const std::string& masterCallId = "buck_grunt") {
        // Store masterCallId for use in other methods
        masterCallId_ = masterCallId;

        // Use TestPaths to get proper audio file path if not provided
        std::string actualAudioFile = audioFile;
        if (actualAudioFile.empty()) {
            actualAudioFile = TestPaths::getMasterCallFile("buck_grunt", ".wav").string();
        }

        std::cout << "=== DTW Debug Test ===" << std::endl;

        // Validate input parameters
        if (!validateInputs(actualAudioFile, masterCallId)) {
            return false;
        }

        // Create engine
        auto engineResult = UnifiedAudioEngine::create();
        if (!engineResult.isOk()) {
            std::cerr << "Failed to create engine: " << static_cast<int>(engineResult.error())
                      << std::endl;
            return false;
        }
        engine_ = std::move(engineResult.value);

        // Create session
        auto sessionResult = engine_->createSession(44100.0f);
        if (!sessionResult.isOk()) {
            std::cerr << "Failed to create session: " << static_cast<int>(sessionResult.error())
                      << std::endl;
            return false;
        }
        sessionId_ = sessionResult.value;
        std::cout << "✅ Session created with ID: " << sessionId_ << std::endl;

        // Load master call
        std::cout << "Loading master call '" << masterCallId << "': ";
        auto loadResult = engine_->loadMasterCall(sessionId_, masterCallId);
        if (loadResult != UnifiedAudioEngine::Status::OK) {
            std::cout << "FAILED (Status: " << static_cast<int>(loadResult) << ")" << std::endl;
            return false;
        }
        std::cout << "SUCCESS" << std::endl;

        // Load and process audio file
        if (!loadAndProcessAudioFile(audioFile)) {
            return false;
        }

        // Get feature count
        auto featureCountResult = engine_->getFeatureCount(sessionId_);
        if (!featureCountResult.isOk()) {
            std::cerr << "Failed to get feature count: "
                      << static_cast<int>(featureCountResult.error()) << std::endl;
            return false;
        }

        int featureCount = featureCountResult.value;
        std::cout << "Session feature count: " << featureCount << std::endl;

        // Get similarity score
        auto scoreResult = engine_->getSimilarityScore(sessionId_);
        if (!scoreResult.isOk()) {
            std::cerr << "Failed to get similarity score: " << static_cast<int>(scoreResult.error())
                      << std::endl;
            return false;
        }

        float score = scoreResult.value;
        std::cout << "Similarity score: " << std::fixed << std::setprecision(6) << score
                  << std::endl;

        // Analyze results
        analyzeResults(audioFile, featureCount, score);

        // Cleanup
        auto destroyResult = engine_->destroySession(sessionId_);
        if (destroyResult != UnifiedAudioEngine::Status::OK) {
            std::cerr << "Warning: Failed to destroy session: " << static_cast<int>(destroyResult)
                      << std::endl;
        }

        return true;
    }

  private:
    bool validateInputs(const std::string& audioFile, const std::string& masterCallId) {
        std::cout << "Validating inputs..." << std::endl;

        // Check if audio file exists
        std::ifstream file(audioFile);
        if (!file.good()) {
            std::cerr << "❌ Audio file not found: " << audioFile << std::endl;
            return false;
        }
        std::cout << "✅ Audio file found: " << audioFile << std::endl;

        // Check master call ID is not empty
        if (masterCallId.empty()) {
            std::cerr << "❌ Master call ID is empty" << std::endl;
            return false;
        }
        std::cout << "✅ Master call ID: " << masterCallId << std::endl;

        return true;
    }

    std::unique_ptr<UnifiedAudioEngine> engine_;
    SessionId sessionId_;

    bool loadAndProcessAudioFile(const std::string& filename) {
        std::cout << "Loading audio file: " << filename << std::endl;

        // Load WAV file
        unsigned int channels;
        unsigned int sampleRate;
        drwav_uint64 totalFrames;

        float* rawAudioData = drwav_open_file_and_read_pcm_frames_f32(
            filename.c_str(), &channels, &sampleRate, &totalFrames, nullptr);

        if (!rawAudioData) {
            std::cerr << "Failed to load audio file: " << filename << std::endl;
            return false;
        }

        // Convert to mono if needed
        std::vector<float> monoData;
        if (channels > 1) {
            monoData.resize(totalFrames);
            for (drwav_uint64 i = 0; i < totalFrames; ++i) {
                float sum = 0.0f;
                for (unsigned int ch = 0; ch < channels; ++ch) {
                    sum += rawAudioData[i * channels + ch];
                }
                monoData[i] = sum / channels;
            }
        } else {
            monoData.assign(rawAudioData, rawAudioData + totalFrames);
        }

        drwav_free(rawAudioData, nullptr);

        float duration = static_cast<float>(totalFrames) / sampleRate;
        std::cout << "Audio info: " << totalFrames << " samples, " << sampleRate << "Hz, "
                  << channels << " channels, " << std::fixed << std::setprecision(3) << duration
                  << "s" << std::endl;

        // Validate sample rate matches session
        if (sampleRate != 44100) {
            std::cout << "⚠️  WARNING: Audio sample rate (" << sampleRate
                      << "Hz) doesn't match session (44100Hz)" << std::endl;
            std::cout << "This may cause feature extraction issues." << std::endl;
        }

        // Validate audio has reasonable duration
        if (duration < 0.1f) {
            std::cout << "⚠️  WARNING: Audio duration is very short (" << duration << "s)"
                      << std::endl;
            std::cout << "May not provide enough data for meaningful analysis." << std::endl;
        } else if (duration > 30.0f) {
            std::cout << "⚠️  WARNING: Audio is quite long (" << duration << "s)" << std::endl;
            std::cout << "Processing may take significant time." << std::endl;
        }

        // Process audio in different chunk sizes to diagnose the issue
        testChunkProcessing(monoData, 1024, "1024 samples");
        testChunkProcessing(monoData, 512, "512 samples");
        testChunkProcessing(monoData, static_cast<size_t>(monoData.size()), "entire file");

        // Now process normally for the actual test
        std::cout << "\nProcessing for main session..." << std::endl;
        std::cout << "Current master call: " << masterCallId_ << std::endl;
        std::cout << "Processing audio chunk (" << monoData.size() << " samples): ";

        // Process the entire audio at once
        auto processResult =
            engine_->processAudioChunk(sessionId_, std::span<const float>(monoData));
        if (processResult != UnifiedAudioEngine::Status::OK) {
            std::cout << "FAILED (Status: " << static_cast<int>(processResult) << ")" << std::endl;
            return false;
        }
        std::cout << "SUCCESS" << std::endl;

        return true;
    }

    void testChunkProcessing(const std::vector<float>& audioData,
                             size_t chunkSize,
                             const std::string& description) {
        std::cout << "\nTesting chunk processing with " << description << ":" << std::endl;

        // Create temporary session for testing
        auto testSessionResult = engine_->createSession(44100.0f);
        if (!testSessionResult.isOk()) {
            std::cerr << "  Failed to create test session" << std::endl;
            return;
        }

        SessionId testSession = testSessionResult.value;

        // Load master call - use the actual masterCallId instead of hardcoded
        auto loadResult = engine_->loadMasterCall(testSession, masterCallId_);
        if (loadResult != UnifiedAudioEngine::Status::OK) {
            std::cerr << "  Failed to load master call '" << masterCallId_
                      << "' (Status: " << static_cast<int>(loadResult) << ")" << std::endl;
            auto destroyResult = engine_->destroySession(testSession);
            if (destroyResult != UnifiedAudioEngine::Status::OK) {
                std::cerr << "  Warning: Failed to destroy test session: "
                          << static_cast<int>(destroyResult) << std::endl;
            }
            return;
        }

        // Process chunks
        size_t totalProcessed = 0;
        size_t numChunks = 0;
        UnifiedAudioEngine::Status lastResult = UnifiedAudioEngine::Status::OK;

        for (size_t i = 0; i < audioData.size(); i += chunkSize) {
            size_t remaining = audioData.size() - i;
            size_t toProcess = std::min(chunkSize, remaining);

            std::span<const float> chunk(audioData.data() + i, toProcess);
            auto result = engine_->processAudioChunk(testSession, chunk);

            if (result == UnifiedAudioEngine::Status::OK) {
                totalProcessed += toProcess;
                numChunks++;
            } else {
                lastResult = result;
                std::cerr << "  Chunk " << numChunks
                          << " failed (Status: " << static_cast<int>(result) << ")" << std::endl;
            }
        }

        // Get feature count
        auto featureResult = engine_->getFeatureCount(testSession);
        if (featureResult.isOk()) {
            std::cout << "  Processed " << numChunks << " chunks, " << totalProcessed
                      << " samples total" << std::endl;
            std::cout << "  Features extracted: " << featureResult.value << std::endl;

            // Get and display similarity score for comparison
            auto scoreResult = engine_->getSimilarityScore(testSession);
            if (scoreResult.isOk()) {
                std::cout << "  Similarity score: " << std::fixed << std::setprecision(6)
                          << scoreResult.value << std::endl;
            }
        } else {
            std::cerr << "  Failed to get feature count" << std::endl;
        }

        // Cleanup
        auto destroyResult = engine_->destroySession(testSession);
        if (destroyResult != UnifiedAudioEngine::Status::OK) {
            std::cerr << "  Warning: Failed to destroy test session: "
                      << static_cast<int>(destroyResult) << std::endl;
        }
    }

    void analyzeResults(const std::string& filename, int featureCount, float score) {
        std::cout << "\n=== ANALYSIS ===" << std::endl;

        // Expected vs actual
        const int expectedFeatures = 42;  // Based on RealWildlifeCallAnalysisTest output

        std::cout << "File: " << filename << std::endl;
        std::cout << "Expected features: " << expectedFeatures << std::endl;
        std::cout << "Actual features: " << featureCount << std::endl;
        std::cout << "Difference: " << (featureCount - expectedFeatures) << std::endl;

        if (featureCount < expectedFeatures / 2) {
            std::cout << "\n⚠️  WARNING: Significantly fewer features than expected!" << std::endl;
            std::cout << "Possible causes:" << std::endl;
            std::cout << "  1. Audio not being processed completely" << std::endl;
            std::cout << "  2. Frame size/hop size mismatch" << std::endl;
            std::cout << "  3. VAD (Voice Activity Detection) filtering out frames" << std::endl;
            std::cout << "  4. Processing buffer size too small" << std::endl;
            std::cout << "  5. Sample rate mismatch between audio and session" << std::endl;

            // Calculate expected frame count dynamically
            // TODO: Get actual frame size/hop size from engine configuration
            const size_t frameSize = 512;
            const size_t hopSize = 256;
            const float sampleRate = 44100.0f;

            // Calculate duration from the actual audio file instead of hardcoded value
            std::cout << "\nFrame calculation (estimated):" << std::endl;
            std::cout << "  Frame size: " << frameSize << std::endl;
            std::cout << "  Hop size: " << hopSize << std::endl;

            // Note: We should get actual audio duration from the loaded file
            std::cout << "  Note: Frame calculation uses estimated values" << std::endl;
            std::cout << "  Consider exposing actual engine configuration for accurate calculation"
                      << std::endl;
        }

        if (score < 0.001f) {
            std::cout << "\n⚠️  WARNING: Very low similarity score!" << std::endl;
            std::cout << "This indicates the DTW comparison may not have enough data." << std::endl;
        } else if (score > 0.8f) {
            std::cout << "\n✅ Good similarity score - feature extraction appears successful"
                      << std::endl;
        }

        // Additional diagnostics
        std::cout << "\n=== ADDITIONAL DIAGNOSTICS ===" << std::endl;

        // Check if we can get detailed scoring information
        auto detailedScoreResult = engine_->getDetailedScore(sessionId_);
        if (detailedScoreResult.isOk()) {
            auto detailedScore = detailedScoreResult.value;
            std::cout << "Detailed scoring available:" << std::endl;
            std::cout << "  MFCC score: " << detailedScore.mfcc << std::endl;
            std::cout << "  Volume score: " << detailedScore.volume << std::endl;
            std::cout << "  Timing score: " << detailedScore.timing << std::endl;
            std::cout << "  Confidence: " << detailedScore.confidence << std::endl;
            std::cout << "  Samples analyzed: " << detailedScore.samplesAnalyzed << std::endl;
            std::cout << "  Is reliable: " << (detailedScore.isReliable ? "Yes" : "No")
                      << std::endl;
            std::cout << "  Is match: " << (detailedScore.isMatch ? "Yes" : "No") << std::endl;
        } else {
            std::cout << "Detailed scoring not available (Status: "
                      << static_cast<int>(detailedScoreResult.error()) << ")" << std::endl;
        }

        // Check session duration
        auto durationResult = engine_->getSessionDuration(sessionId_);
        if (durationResult.isOk()) {
            std::cout << "Session duration: " << durationResult.value << " seconds" << std::endl;
        }
    }

    // Add member variable to store masterCallId
    std::string masterCallId_;
};

int main(int argc, char* argv[]) {
    // Initialize TestUtils paths
    TestPaths::initialize();

    DTWDebugTest test;
    std::string audioFile = TestPaths::getMasterCallFile("buck_grunt", ".wav").string();
    std::string masterCallId = "buck_grunt";

    if (argc > 1) {
        audioFile = argv[1];
    }
    if (argc > 2) {
        masterCallId = argv[2];
    }

    bool success = test.run(audioFile, masterCallId);

    return success ? 0 : 1;
}
