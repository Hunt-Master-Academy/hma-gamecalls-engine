#include "huntmaster/core/HuntmasterAudioEngine.h"

#include <algorithm>   // For std::remove_if, std::max, std::min, std::any_of
#include <atomic>      // For std::atomic
#include <chrono>      // For std::chrono
#include <cmath>       // For std::abs
#include <filesystem>  // For std::filesystem::path
#include <fstream>     // For std::ifstream, std::ofstream
#include <iostream>    // For std::cout, std::cerr
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <span>
#include <unordered_map>
#include <vector>

#include "../libs/dr_wav.h"
#include "huntmaster/core/AudioPlayer.h"
#include "huntmaster/core/AudioRecorder.h"
#include "huntmaster/core/DTWProcessor.h"
#include "huntmaster/core/MFCCProcessor.h"
#include "huntmaster/core/VoiceActivityDetector.h"

namespace huntmaster {

/**
 * @class DrWavRAII
 * @brief RAII wrapper for dr_wav allocated memory
 */
class DrWavRAII {
   public:
    explicit DrWavRAII(float *data) : data_(data) {}
    ~DrWavRAII() {
        if (data_) {
            drwav_free(data_, nullptr);
        }
    }

    // Delete copy operations
    DrWavRAII(const DrWavRAII &) = delete;
    DrWavRAII &operator=(const DrWavRAII &) = delete;

    // Allow move operations
    DrWavRAII(DrWavRAII &&other) noexcept : data_(other.data_) { other.data_ = nullptr; }

    DrWavRAII &operator=(DrWavRAII &&other) noexcept {
        if (this != &other) {
            if (data_) {
                drwav_free(data_, nullptr);
            }
            data_ = other.data_;
            other.data_ = nullptr;
        }
        return *this;
    }

    float *get() const { return data_; }
    float *release() {
        float *temp = data_;
        data_ = nullptr;
        return temp;
    }

   private:
    float *data_;
};

/**
 * @class HuntmasterAudioEngine
 * @brief Improved implementation with better architecture and thread safety
 */
class HuntmasterAudioEngine::Impl {
   public:
    Impl();
    ~Impl();

    // Core functionality
    void initialize();
    void shutdown();

    // Master call management
    EngineStatus loadMasterCall(const std::string_view masterCallId);

    // Real-time session management (C++ API)
    Result<int> startRealtimeSession(float sampleRate, int bufferSize);
    EngineStatus processAudioChunk(int sessionId, std::span<const float> audioBuffer);
    Result<float> getSimilarityScore(int sessionId);
    void endRealtimeSession(int sessionId);

    // Recording management
    Result<int> startRecording(double sampleRate);
    void stopRecording(int recordingId);
    Result<std::string> saveRecording(int recordingId, const std::string &filename);

    // Playback
    EngineStatus playMasterCall(const std::string &callId);
    EngineStatus playRecording(const std::string &filename);
    void stopPlayback();

    // Status queries
    bool isRecording() const;
    float getRecordingLevel() const;
    double getRecordingDuration(int recordingId) const;
    int getSessionFeatureCount(int sessionId) const;

    // static constexpr size_t MAX_BUFFER_SIZE = 1048576;  // 1MB
    // static constexpr size_t FEATURE_VECTOR_SIZE = 13;

   private:
    static constexpr size_t MAX_BUFFER_SIZE = 1048576; // 1MB
    struct RealtimeSessionState {
        std::vector<float> audioBuffer;
        size_t bufferWritePos = 0;
        std::vector<std::vector<float>> features;
        bool isInSoundSegment = false;
        size_t consecutiveSoundFrames = 0;
        size_t consecutiveSilenceFrames = 0;
        std::vector<float> currentSegmentBuffer;
        float sampleRate = 0.0f;
        std::chrono::steady_clock::time_point startTime;
        size_t mfccProcessedFrames = 0;
    };

    struct RecordingSession {
        std::unique_ptr<AudioRecorder> recorder;
        int id;
        double sampleRate;
        std::chrono::steady_clock::time_point startTime;
    };

    // VAD configuration
    struct VADConfig {
        float energyThreshold = 0.01f;
        float silenceThreshold = 0.02f;
        float windowDurationMs = 20.0f;
        float minSoundDurationMs = 100.0f;
        float minSilenceDurationMs = 50.0f;
        float hangoverDurationMs = 100.0f;
    };

    // Thread-safe session management
    mutable std::shared_mutex sessionsMutex_;
    std::unordered_map<int, RealtimeSessionState> sessions_;
    std::atomic<int> nextSessionId_{1};

    // Thread-safe recording management
    mutable std::shared_mutex recordingsMutex_;
    std::unordered_map<int, RecordingSession> recordings_;
    std::atomic<int> nextRecordingId_{1};

    // Master call data (protected by mutex)
    mutable std::shared_mutex masterCallMutex_;
    std::vector<std::vector<float>> masterCallFeatures_;
    std::string currentMasterCallId_;

    // Processors (thread-safe through internal implementation)
    std::unique_ptr<MFCCProcessor> mfccProcessor_;
    std::unique_ptr<AudioPlayer> audioPlayer_;
    std::unique_ptr<VoiceActivityDetector> vad_;
    float currentSampleRate_{0.0f};

    // Configuration
    VADConfig vadConfig_;
    std::string masterCallsPath_;
    std::string featuresPath_;
    std::string recordingsPath_;

    // Helper methods
    bool loadFeaturesFromFile(const std::string &masterCallId);
    void saveFeaturesToFile(const std::string &masterCallId);
    float calculateEnergy(std::span<const float> samples) const;
    void processVAD(RealtimeSessionState &session, std::span<const float> audioChunk);
    void extractMFCCFeatures(RealtimeSessionState &session);

    std::vector<float> convertToMono(std::span<const float> interleavedData, unsigned int channels);
};

// --- Constructor / Destructor ---
HuntmasterAudioEngine::Impl::Impl() = default;
HuntmasterAudioEngine::Impl::~Impl() { shutdown(); }

void HuntmasterAudioEngine::Impl::initialize() {
    std::unique_lock lock(sessionsMutex_);

    audioPlayer_ = std::make_unique<AudioPlayer>();
    VoiceActivityDetector::Config vad_config;
    vad_ = std::make_unique<VoiceActivityDetector>(vad_config);

    // Centralize path management with validation
    masterCallsPath_ = "../data/master_calls/";
    featuresPath_ = "../data/features/";
    recordingsPath_ = "../data/recordings/";

    // Initialize VAD configuration
    vadConfig_.energyThreshold = 0.001f;    // Lowered for more sensitive detection
    vadConfig_.silenceThreshold = 0.0001f;  // Lowered for more sensitive detection
    vadConfig_.windowDurationMs = 20.0f;
    vadConfig_.minSoundDurationMs = 100.0f;
    vadConfig_.minSilenceDurationMs =
        500.0f;  // Increased to ensure longer silence before segment ends
    vadConfig_.hangoverDurationMs = 200.0f;  // Increased to capture more trailing sound

    // Ensure directories exist and are writable
    std::error_code ec;
    for (const auto &path_str : {masterCallsPath_, featuresPath_, recordingsPath_}) {
        const std::filesystem::path p(path_str);
        std::filesystem::create_directories(p, ec);
        if (ec) {  // If there's an error creating the directory
            std::string error_msg =
                "[HuntmasterEngine] ERROR: Could not create directory: " + p.string() + " (" +
                ec.message() + ")";
            std::cerr << error_msg << std::endl;  // Print the error message
            throw std::runtime_error(error_msg);  // Throw a runtime error
        }

        // Check for write permissions
        auto perms = std::filesystem::status(p, ec).permissions();
        if (ec || (perms & std::filesystem::perms::owner_write) == std::filesystem::perms::none) {
            std::cerr << "[HuntmasterEngine] ERROR: Directory is not writable: " << p << std::endl;
            return;
        }
    }

    std::cout << "[HuntmasterEngine] Initialized successfully." << std::endl;
}

void HuntmasterAudioEngine::Impl::shutdown() {
    {
        std::unique_lock lock(recordingsMutex_);
        for (auto &[id, session] : recordings_) {
            if (session.recorder) session.recorder->stopRecording();
        }
        recordings_.clear();
    }
    {
        std::unique_lock lock(sessionsMutex_);
        sessions_.clear();
    }
    if (audioPlayer_) {
        audioPlayer_->stop();
    }
    std::cout << "[HuntmasterEngine] Shutdown complete." << std::endl;
}

HuntmasterAudioEngine::EngineStatus HuntmasterAudioEngine::Impl::loadMasterCall(
    std::string_view masterCallId_sv) {
    const std::string masterCallId(masterCallId_sv);
    std::cout << "[HuntmasterEngine] INFO: Attempting to load master call '" << masterCallId
              << "'..." << std::endl;

    // Use a unique_lock for the entire operation to ensure consistency.
    std::unique_lock lock(masterCallMutex_);

    // 1. Check if the requested master call is already loaded.
    if (currentMasterCallId_ == masterCallId && !masterCallFeatures_.empty()) {
        std::cout << "[HuntmasterEngine] DEBUG: Master call '" << masterCallId
                  << "' is already loaded. Skipping." << std::endl;
        return EngineStatus::OK;
    }

    // 2. Try loading pre-computed features from a cache file first.
    if (loadFeaturesFromFile(masterCallId)) {
        currentMasterCallId_ = masterCallId;
        std::cout << "[HuntmasterEngine] INFO: Successfully loaded pre-computed features for '"
                  << masterCallId << "'." << std::endl;
        return EngineStatus::OK;
    }
    std::cout
        << "[HuntmasterEngine] DEBUG: No pre-computed features found. Processing from audio file."
        << std::endl;

    // 3. If no features, load the .wav audio file.
    const std::string audioFilePath = masterCallsPath_ + masterCallId + ".wav";
    if (!std::filesystem::exists(audioFilePath)) {
        std::cerr << "[HuntmasterEngine] ERROR: Audio file not found: " << audioFilePath
                  << std::endl;
        return EngineStatus::FILE_NOT_FOUND;
    }

    unsigned int channels, sampleRate;
    drwav_uint64 totalPCMFrameCount;
    float *rawData = drwav_open_file_and_read_pcm_frames_f32(
        audioFilePath.c_str(), &channels, &sampleRate, &totalPCMFrameCount, nullptr);

    if (!rawData) {
        std::cerr << "[HuntmasterEngine] ERROR: dr_wav could not open or read file: "
                  << audioFilePath << std::endl;
        return EngineStatus::FILE_NOT_FOUND;
    }
    // Use RAII to ensure dr_wav memory is freed.
    DrWavRAII audioData(rawData);
    std::cout << "[HuntmasterEngine] DEBUG: Loaded audio: " << totalPCMFrameCount << " frames, "
              << sampleRate << " Hz, " << channels << " channels." << std::endl;

    // 4. Configure the MFCC processor for the audio's sample rate.
    MFCCProcessor::Config mfcc_config;
    mfcc_config.sample_rate = static_cast<float>(sampleRate);
    currentSampleRate_ = static_cast<float>(sampleRate);
    mfccProcessor_ = std::make_unique<MFCCProcessor>(mfcc_config);

    // Convert to mono if necessary
    std::vector<float> monoSamples;
    if (channels > 1) {
        std::cout << "[HuntmasterEngine] DEBUG: Down-mixing multi-channel audio to mono."
                  << std::endl;
        monoSamples = convertToMono({audioData.get(), totalPCMFrameCount * channels}, channels);
    } else {
        monoSamples.assign(audioData.get(), audioData.get() + totalPCMFrameCount);
    }

    if (monoSamples.empty()) {
        std::cerr << "[HuntmasterEngine] ERROR: Audio data is empty after mono conversion."
                  << std::endl;
        return EngineStatus::PROCESSING_ERROR;
    }

    // 6. Extract MFCC features from the entire mono audio buffer.
    // The hop size determines the overlap between consecutive frames. A 50% overlap is standard.
    const size_t hop_size = mfcc_config.frame_size / 2;
    auto features_result = mfccProcessor_->extractFeaturesFromBuffer(monoSamples, hop_size);

    if (!features_result) {
        std::cerr << "[HuntmasterEngine] ERROR: Failed to extract MFCC features. Reason: "
                  << static_cast<int>(features_result.error()) << std::endl;
        return EngineStatus::PROCESSING_ERROR;
    }

    // 7. Store the new features and save them to the cache file for next time.
    masterCallFeatures_ = std::move(*features_result);
    std::cout << "[HuntmasterEngine] DEBUG: Extracted " << masterCallFeatures_.size()
              << " MFCC feature frames." << std::endl;

    saveFeaturesToFile(masterCallId);
    currentMasterCallId_ = masterCallId;

    std::cout << "[HuntmasterEngine] INFO: Successfully processed and loaded master call '"
              << masterCallId << "'." << std::endl;
    return EngineStatus::OK;
}

HuntmasterAudioEngine::Result<int> HuntmasterAudioEngine::Impl::startRealtimeSession(
    float sampleRate, int bufferSize) {
    std::cout << "[HuntmasterEngine] INFO: Starting realtime session with sample rate "
              << sampleRate << " Hz, buffer size " << bufferSize << "..." << std::endl;

    if (sampleRate <= 0.0f || bufferSize <= 0) {
        std::cerr << "[HuntmasterEngine] ERROR: Invalid parameters for startRealtimeSession. "
                  << "Sample rate and buffer size must be positive." << std::endl;
        return {-1, EngineStatus::INVALID_PARAMS};
    }

    std::unique_lock lock(sessionsMutex_);
    const int sessionId = nextSessionId_++;
    auto &session = sessions_[sessionId];
    session.sampleRate = sampleRate;
    session.startTime = std::chrono::steady_clock::now();

    // Handle MFCC processor configuration. It must match the session's sample rate.
    if (!mfccProcessor_ || currentSampleRate_ != sampleRate) {
        if (mfccProcessor_) {
            std::cout << "[HuntmasterEngine] DEBUG: Sample rate changed from " << currentSampleRate_
                      << " to " << sampleRate << ". Re-initializing MFCC processor." << std::endl;
            if (!masterCallFeatures_.empty()) {
                std::cout << "[HuntmasterEngine] WARNING: Master call features were for a "
                             "different sample rate and are now invalid. Please reload the master "
                             "call."
                          << std::endl;
                // Clear the now-invalid features to prevent incorrect comparisons.
                std::unique_lock masterLock(masterCallMutex_);
                masterCallFeatures_.clear();
                currentMasterCallId_.clear();
            }
        } else {
            std::cout << "[HuntmasterEngine] DEBUG: MFCC processor not initialized. Creating new "
                         "instance."
                      << std::endl;
        }

        MFCCProcessor::Config mfcc_config;
        mfcc_config.sample_rate = sampleRate;
        // NOTE: The MFCC frame_size should be based on audio characteristics (e.g., 25ms window),
        // not the transport bufferSize. We use the default frame_size from the MFCCProcessor
        // config.
        mfccProcessor_ = std::make_unique<MFCCProcessor>(mfcc_config);
        currentSampleRate_ = sampleRate;
        std::cout << "[HuntmasterEngine] DEBUG: MFCC processor configured for " << sampleRate
                  << " Hz." << std::endl;
    }

    std::cout << "[HuntmasterEngine] INFO: Started realtime session " << sessionId << "."
              << std::endl;
    return {sessionId, EngineStatus::OK};
}

HuntmasterAudioEngine::EngineStatus HuntmasterAudioEngine::Impl::processAudioChunk(
    int sessionId, std::span<const float> audioBuffer) {
    std::shared_lock lock(sessionsMutex_);

    auto it = sessions_.find(sessionId);
    if (it == sessions_.end()) {
        return EngineStatus::INVALID_SESSION;
    }

    auto &session = it->second;
    lock.unlock();

    // Validate buffer size
    if (audioBuffer.size() > MAX_BUFFER_SIZE - session.audioBuffer.size()) {
        return EngineStatus::BUFFER_OVERFLOW;
    }

    // Process VAD and extract features
    processVAD(session, audioBuffer);

    return EngineStatus::OK;
}

// void HuntmasterAudioEngine::Impl::processVAD(RealtimeSessionState &session,
//                                              std::span<const float> audioChunk) {
//     session.audioBuffer.insert(session.audioBuffer.end(), audioChunk.begin(), audioChunk.end());
//     const float sampleRate = session.sampleRate;
//     const size_t windowSamples =
//         static_cast<size_t>(vadConfig_.windowDurationMs * sampleRate / 1000.0f);

//     if (windowSamples == 0) return;
//     const size_t minSoundSamples =
//         static_cast<size_t>(vadConfig_.minSoundDurationMs * sampleRate / 1000.0f);
//     const size_t minSilenceSamples =
//         static_cast<size_t>(vadConfig_.minSilenceDurationMs * sampleRate / 1000.0f);
//     const size_t hangoverSamples =
//         static_cast<size_t>(vadConfig_.hangoverDurationMs * sampleRate / 1000.0f);

//     // Process windows
//     while (session.bufferWritePos + windowSamples <= session.audioBuffer.size()) {
//         const std::span<const float> window(session.audioBuffer.data() + session.bufferWritePos,
//                                             windowSamples);

//         const float energy = calculateEnergy(window);
//         const float peak = *std::max_element(window.begin(), window.end(), [](float a, float b) {
//             return std::abs(a) < std::abs(b);
//         });

//         const bool isActive =
//             (energy > vadConfig_.energyThreshold || std::abs(peak) >
//             vadConfig_.silenceThreshold);

//         if (isActive) {
//             session.consecutiveSoundFrames += windowSamples;
//             session.consecutiveSilenceFrames = 0;

//             if (!session.isInSoundSegment && session.consecutiveSoundFrames >= minSoundSamples) {
//                 session.isInSoundSegment = true;
//                 std::cout << "[VAD] Sound detected" << std::endl;
//             }
//         } else {
//             session.consecutiveSilenceFrames += windowSamples;
//             session.consecutiveSoundFrames = 0;

//             if (session.isInSoundSegment && session.consecutiveSilenceFrames >=
//             minSilenceSamples) {
//                 session.isInSoundSegment = false;
//                 std::cout << "[VAD] Silence detected, processing segment" << std::endl;

//                 // Process the accumulated segment
//                 extractMFCCFeatures(session);
//                 session.currentSegmentBuffer.clear();
//                 session.mfccProcessedFrames = 0;
//             }
//         }

//         // Accumulate audio for active segments
//         if (session.isInSoundSegment || (session.consecutiveSilenceFrames > 0 &&
//                                          session.consecutiveSilenceFrames <= hangoverSamples)) {
//             session.currentSegmentBuffer.insert(session.currentSegmentBuffer.end(),
//             window.begin(),
//                                                 window.end());
//         }

//         session.bufferWritePos += windowSamples;
//     }

//     // Clean up processed audio
//     if (session.bufferWritePos > 0) {
//         session.audioBuffer.erase(session.audioBuffer.begin(),
//                                   session.audioBuffer.begin() + session.bufferWritePos);
//         session.bufferWritePos = 0;
//     }
// }
void HuntmasterAudioEngine::Impl::processVAD(RealtimeSessionState &session,
                                             std::span<const float> audioChunk) {
    // 1. Check if we have a VAD instance. If not, we can't process.
    if (!vad_) {
        return;
    }

    // 2. Process the incoming audio chunk in smaller windows, as the VAD expects.
    // Calculate window size in samples based on VAD config and session sample rate
    const size_t windowSamples = static_cast<size_t>(vadConfig_.windowDurationMs * session.sampleRate / 1000.0f);
    if (windowSamples == 0) return;

    for (size_t i = 0; i + windowSamples <= audioChunk.size(); i += windowSamples) {
        auto window = audioChunk.subspan(i, windowSamples);

        // 3. Get the result from the dedicated VAD component.
        auto vad_result = vad_->processWindow(window);

        if (vad_result) {
            bool wasInSoundSegment = session.isInSoundSegment;
            session.isInSoundSegment = vad_result->is_active;

            // 4. If voice is active, accumulate the audio.
            if (session.isInSoundSegment) {
                session.currentSegmentBuffer.insert(session.currentSegmentBuffer.end(),
                                                    window.begin(), window.end());
            }
            // 5. If voice just stopped, process the accumulated segment.
            else if (wasInSoundSegment && !session.isInSoundSegment) {
                std::cout << "[VAD] Silence detected, processing segment" << std::endl;
                extractMFCCFeatures(session);
                session.currentSegmentBuffer.clear();  // Clear buffer after processing
            }
        }
    }
}

void HuntmasterAudioEngine::Impl::extractMFCCFeatures(RealtimeSessionState &session) {
    if (!mfccProcessor_ || session.currentSegmentBuffer.empty()) {
        return;
    }

    // Use extractFeaturesFromBuffer instead of processBuffer
    // The second argument, hop_size, is optional and defaults to frame_size / 2.
    // If MFCCProcessor::Config::frame_size is not accessible, use a default or known value.
    const size_t hop_size = 512 / 2; // Replace 512 with the actual frame size if known/configurable
    auto features_result = mfccProcessor_->extractFeaturesFromBuffer(session.currentSegmentBuffer, hop_size);
    if (!features_result) {
        std::cerr << "[HuntmasterEngine] ERROR: Failed to extract MFCC features in segment." << std::endl;
        return;
    }

    session.features.reserve(session.features.size() + features_result->size());
    for (const auto &frame : *features_result) {
        session.features.push_back(frame);
    }
}

HuntmasterAudioEngine::Result<float> HuntmasterAudioEngine::Impl::getSimilarityScore(
    int sessionId) {
    std::shared_lock sessionLock(sessionsMutex_);

    auto it = sessions_.find(sessionId);
    if (it == sessions_.end()) {
        return {0.0f, EngineStatus::INVALID_SESSION};
    }

    const auto &sessionFeatures = it->second.features;
    sessionLock.unlock();

    std::shared_lock masterLock(masterCallMutex_);

    if (masterCallFeatures_.empty() || sessionFeatures.empty()) {
        return {0.0f, EngineStatus::INSUFFICIENT_DATA};
    }

    const float distance = DTWProcessor::calculateDistance(masterCallFeatures_, sessionFeatures);

    masterLock.unlock();

    // Convert distance to similarity score (0-1 range)
    const float score = 1.0f / (1.0f + distance);

    std::cout << "[HuntmasterEngine] DTW Distance: " << distance << ", Similarity Score: " << score
              << std::endl;

    return {score, EngineStatus::OK};
}

float HuntmasterAudioEngine::Impl::calculateEnergy(std::span<const float> samples) const {
    if (samples.empty()) return 0.0f;

    float sum = 0.0f;
    for (float sample : samples) {
        sum += sample * sample;
    }

    return sum / static_cast<float>(samples.size());
}

std::vector<float> HuntmasterAudioEngine::Impl::convertToMono(
    std::span<const float> interleavedData, unsigned int channels) {
    if (channels == 0) return {};

    const size_t frames = interleavedData.size() / channels;
    std::vector<float> mono(frames);
    const float channelScale = 1.0f / static_cast<float>(channels);

    for (size_t i = 0; i < frames; ++i) {
        float sum = 0.0f;
        for (unsigned int ch = 0; ch < channels; ++ch) {
            sum += interleavedData[i * channels + ch];  // Access span like an array
        }
        mono[i] = sum * channelScale;
    }

    return mono;
}

// ... Continuing from previous implementation ...

bool HuntmasterAudioEngine::Impl::loadFeaturesFromFile(const std::string &masterCallId) {
    const std::string featureFilePath = featuresPath_ + masterCallId + ".mfc";
    std::ifstream inFile(featureFilePath, std::ios::binary);

    if (!inFile.is_open()) {
        return false;
    }

    // Read header
    uint32_t numFrames = 0;
    uint32_t numCoeffs = 0;

    inFile.read(reinterpret_cast<char *>(&numFrames), sizeof(numFrames));
    inFile.read(reinterpret_cast<char *>(&numCoeffs), sizeof(numCoeffs));

    if (!inFile.good() || numFrames == 0 || numCoeffs == 0 || numCoeffs > FEATURE_VECTOR_SIZE * 2) {
        std::cerr << "[HuntmasterEngine] Invalid feature file header: " << featureFilePath
                  << std::endl;
        return false;
    }

    // Validate file size matches expected data
    const size_t expectedBytes = numFrames * numCoeffs * sizeof(float);
    inFile.seekg(0, std::ios::end);
    const size_t fileSize = static_cast<size_t>(inFile.tellg());
    const size_t headerSize = 2 * sizeof(uint32_t);

    if (fileSize < headerSize + expectedBytes) {
        std::cerr << "[HuntmasterEngine] Feature file size mismatch: " << featureFilePath
                  << std::endl;
        return false;
    }

    inFile.seekg(headerSize, std::ios::beg);

    // Read features
    masterCallFeatures_.clear();
    masterCallFeatures_.reserve(numFrames);

    std::vector<float> buffer(numCoeffs);

    for (uint32_t i = 0; i < numFrames; ++i) {
        inFile.read(reinterpret_cast<char *>(buffer.data()), numCoeffs * sizeof(float));

        if (!inFile.good()) {
            std::cerr << "[HuntmasterEngine] Error reading feature frame " << i << std::endl;
            masterCallFeatures_.clear();
            return false;
        }

        masterCallFeatures_.push_back(buffer);
    }

    std::cout << "[HuntmasterEngine] Loaded " << numFrames << " feature frames from "
              << featureFilePath << std::endl;

    return true;
}

void HuntmasterAudioEngine::Impl::saveFeaturesToFile(const std::string &masterCallId) {
    // const std::string featureFilePath = featuresPath_ + masterCallId + ".mfc";
    std::filesystem::path featureFilePath =
        std::filesystem::path(featuresPath_) / (masterCallId + ".mfc");

    // Create directory if it doesn't exist
    // Note: In production, use std::filesystem::create_directories

    std::ofstream outFile(featureFilePath, std::ios::binary);

    if (!outFile.is_open()) {
        std::cerr << "[HuntmasterEngine] Could not create feature file: " << featureFilePath
                  << std::endl;
        return;
    }

    const uint32_t numFrames = static_cast<uint32_t>(masterCallFeatures_.size());
    const uint32_t numCoeffs =
        numFrames > 0 ? static_cast<uint32_t>(masterCallFeatures_[0].size()) : 0;

    // Write header
    outFile.write(reinterpret_cast<const char *>(&numFrames), sizeof(numFrames));
    outFile.write(reinterpret_cast<const char *>(&numCoeffs), sizeof(numCoeffs));

    // Write features
    for (const auto &frame : masterCallFeatures_) {
        outFile.write(reinterpret_cast<const char *>(frame.data()), frame.size() * sizeof(float));
    }

    if (!outFile.good()) {
        std::cerr << "[HuntmasterEngine] Error writing feature file: " << featureFilePath
                  << std::endl;
        // Consider deleting the partial file
        return;
    }

    std::cout << "[HuntmasterEngine] Saved " << numFrames << " feature frames to "
              << featureFilePath << std::endl;
}

void HuntmasterAudioEngine::Impl::endRealtimeSession(int sessionId) {
    std::unique_lock lock(sessionsMutex_);

    auto it = sessions_.find(sessionId);
    if (it != sessions_.end()) {
        const auto duration = std::chrono::steady_clock::now() - it->second.startTime;
        const auto durationMs =
            std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

        // Using std::format for cleaner output (requires C++20)
        // If C++20 is not available, use traditional iostream formatting.
        std::cout << "[HuntmasterEngine] Ended session " << sessionId
                  << " (duration: " << durationMs << "ms, features: " << it->second.features.size()
                  << ")\n";
        // sessionId, durationMs, it->second.features.size() sessions_.erase(it););
        sessions_.erase(it);
    }
}

// Recording Management

HuntmasterAudioEngine::Result<int> HuntmasterAudioEngine::Impl::startRecording(double sampleRate) {
    if (sampleRate <= 0) {
        return {-1, EngineStatus::INVALID_PARAMS};
    }

    std::unique_lock lock(recordingsMutex_);

    const int recordingId = nextRecordingId_++;

    auto &session = recordings_[recordingId];
    session.id = recordingId;
    session.sampleRate = sampleRate;
    session.recorder = std::make_unique<AudioRecorder>();
    session.startTime = std::chrono::steady_clock::now();

    AudioRecorder::Config config;
    config.sampleRate = static_cast<int>(sampleRate);
    config.channels = 1;       // Mono recording
    config.bufferSize = 4096;  // Reasonable default

    if (!session.recorder->startRecording(config)) {
        recordings_.erase(recordingId);
        return {-1, EngineStatus::RECORDER_INIT_FAILED};
    }

    std::cout << "[HuntmasterEngine] Started recording " << recordingId << " at " << sampleRate
              << "Hz" << std::endl;

    return {recordingId, EngineStatus::OK};
}

void HuntmasterAudioEngine::Impl::stopRecording(int recordingId) {
    std::unique_lock lock(recordingsMutex_);

    auto it = recordings_.find(recordingId);
    if (it != recordings_.end()) {
        it->second.recorder->stopRecording();

        const auto duration = std::chrono::steady_clock::now() - it->second.startTime;
        const auto durationSec = std::chrono::duration_cast<std::chrono::seconds>(duration).count();

        std::cout << "[HuntmasterEngine] Stopped recording " << recordingId
                  << " (duration: " << durationSec << "s)" << std::endl;
    }
}

// HuntmasterAudioEngine::Result<std::string> HuntmasterAudioEngine::Impl::saveRecording(
//     int recordingId, const std::string &filename) {
//     if (filename.empty()) {
//         return {"", EngineStatus::INVALID_PARAMS};
//     }

HuntmasterAudioEngine::Result<std::string> HuntmasterAudioEngine::saveRecording(
    int recordingId, std::string_view filename) {
    return pimpl->saveRecording(recordingId, filename);
}

std::unique_lock lock(recordingsMutex_);

auto it = recordings_.find(recordingId);
if (it == recordings_.end()) {
    return {"", EngineStatus::INVALID_RECORDING_ID};
}

// Ensure recording is stopped
it->second.recorder->stopRecording();

// Sanitize filename (remove path separators)
std::string safeFilename = filename;
safeFilename.erase(std::remove_if(safeFilename.begin(), safeFilename.end(),
                                  [](char c) { return c == '/' || c == '\\'; }),
                   safeFilename.end());

// Add .wav extension if not present
if (safeFilename.find(".wav") == std::string::npos) {
    safeFilename += ".wav";
}

const std::string fullPath = recordingsPath_ + safeFilename;

if (!it->second.recorder->saveToWav(fullPath)) {
    return {"", EngineStatus::FILE_WRITE_ERROR};
}

// Remove recording from active list after successful save
recordings_.erase(it);

std::cout << "[HuntmasterEngine] Saved recording to " << fullPath << std::endl;

return {fullPath, EngineStatus::OK};
}

bool HuntmasterAudioEngine::Impl::isRecording() const {
    std::shared_lock lock(recordingsMutex_);

    return std::any_of(recordings_.begin(), recordings_.end(), [](const auto &pair) {
        return pair.second.recorder && pair.second.recorder->isRecording();
    });
}

float HuntmasterAudioEngine::Impl::getRecordingLevel() const {
    std::shared_lock lock(recordingsMutex_);

    float maxLevel = 0.0f;

    for (const auto &[id, session] : recordings_) {
        if (session.recorder) {
            maxLevel = std::max(maxLevel, session.recorder->getCurrentLevel());
        }
    }

    return maxLevel;
}

double HuntmasterAudioEngine::Impl::getRecordingDuration(int recordingId) const {
    std::shared_lock lock(recordingsMutex_);

    auto it = recordings_.find(recordingId);
    if (it != recordings_.end() && it->second.recorder) {
        return it->second.recorder->getDuration();
    }

    return 0.0;
}

int HuntmasterAudioEngine::Impl::getSessionFeatureCount(int sessionId) const {
    std::shared_lock lock(sessionsMutex_);
    auto it = sessions_.find(sessionId);
    if (it != sessions_.end()) {
        return static_cast<int>(it->second.features.size());
    }
    return 0;
}

// Playback Management

// HuntmasterAudioEngine::EngineStatus HuntmasterAudioEngine::Impl::playMasterCall(
//     const std::string &callId) {
//     if (callId.empty()) {
//         return EngineStatus::INVALID_PARAMS;
//     }
HuntmasterAudioEngine::EngineStatus HuntmasterAudioEngine::playMasterCall(std::string_view callId) {
    return pimpl->playMasterCall(callId);
}

const std::string filePath = masterCallsPath_ + callId + ".wav";

if (!audioPlayer_) {
    return EngineStatus::PLAYER_NOT_INITIALIZED;
}

if (!audioPlayer_->loadFile(filePath)) {
    std::cerr << "[HuntmasterEngine] Failed to load master call: " << filePath << std::endl;
    return EngineStatus::FILE_NOT_FOUND;
}

audioPlayer_->play();

std::cout << "[HuntmasterEngine] Playing master call: " << callId << std::endl;

return EngineStatus::OK;
}

// HuntmasterAudioEngine::EngineStatus HuntmasterAudioEngine::Impl::playRecording(
//     const std::string &filename) {
//     if (filename.empty()) {
//         return EngineStatus::INVALID_PARAMS;
//     }

HuntmasterAudioEngine::EngineStatus HuntmasterAudioEngine::playRecording(
    std::string_view filename) {
    return pimpl->playRecording(filename);
}
// Determine if it's a full path or just a filename
const std::string filePath =
    (filename.find('/') != std::string::npos || filename.find('\\') != std::string::npos)
        ? filename
        : recordingsPath_ + filename;

if (!audioPlayer_) {
    return EngineStatus::PLAYER_NOT_INITIALIZED;
}

if (!audioPlayer_->loadFile(filePath)) {
    std::cerr << "[HuntmasterEngine] Failed to load recording: " << filePath << std::endl;
    return EngineStatus::FILE_NOT_FOUND;
}

audioPlayer_->play();

std::cout << "[HuntmasterEngine] Playing recording: " << filename << std::endl;

return EngineStatus::OK;
}

void HuntmasterAudioEngine::Impl::stopPlayback() {
    if (audioPlayer_) {
        audioPlayer_->stop();
        std::cout << "[HuntmasterEngine] Playback stopped" << std::endl;
    }
}

// ============================================================================
// Main HuntmasterAudioEngine Class Implementation
// ============================================================================

HuntmasterAudioEngine::HuntmasterAudioEngine() : pimpl(std::make_unique<Impl>()) {}

HuntmasterAudioEngine::~HuntmasterAudioEngine() = default;

HuntmasterAudioEngine &HuntmasterAudioEngine::getInstance() {
    static HuntmasterAudioEngine instance;
    return instance;
}

void HuntmasterAudioEngine::initialize() { pimpl->initialize(); }

void HuntmasterAudioEngine::shutdown() { pimpl->shutdown(); }

// Delegating methods

// HuntmasterAudioEngine::EngineStatus HuntmasterAudioEngine::loadMasterCall(
//     const std::string &masterCallId) {
//     return pimpl->loadMasterCall(masterCallId);
// }
HuntmasterAudioEngine::EngineStatus HuntmasterAudioEngine::loadMasterCall(
    std::string_view masterCallId) {
    return pimpl->loadMasterCall(masterCallId);
}

HuntmasterAudioEngine::Result<int> HuntmasterAudioEngine::startRealtimeSession(float sampleRate,
                                                                               int bufferSize) {
    return pimpl->startRealtimeSession(sampleRate, bufferSize);
}

HuntmasterAudioEngine::EngineStatus HuntmasterAudioEngine::processAudioChunk(
    int sessionId, const float *audioBuffer, int bufferSize) {
    if (!audioBuffer || bufferSize <= 0) {
        return EngineStatus::INVALID_PARAMS;
    }

    // Create span from raw pointer
    const std::span<const float> audioSpan(audioBuffer, bufferSize);
    return pimpl->processAudioChunk(sessionId, audioSpan);
}

HuntmasterAudioEngine::Result<float> HuntmasterAudioEngine::getSimilarityScore(int sessionId) {
    return pimpl->getSimilarityScore(sessionId);
}

void HuntmasterAudioEngine::endRealtimeSession(int sessionId) {
    pimpl->endRealtimeSession(sessionId);
}

HuntmasterAudioEngine::Result<int> HuntmasterAudioEngine::startRecording(double sampleRate) {
    return pimpl->startRecording(sampleRate);
}

void HuntmasterAudioEngine::stopRecording(int recordingId) { pimpl->stopRecording(recordingId); }

HuntmasterAudioEngine::Result<std::string> HuntmasterAudioEngine::saveRecording(
    int recordingId, const std::string &filename) {
    return pimpl->saveRecording(recordingId, std::string(filename));
}

bool HuntmasterAudioEngine::isRecording() const { return pimpl->isRecording(); }

float HuntmasterAudioEngine::getRecordingLevel() const { return pimpl->getRecordingLevel(); }

double HuntmasterAudioEngine::getRecordingDuration(int recordingId) const {
    return pimpl->getRecordingDuration(recordingId);
}

HuntmasterAudioEngine::EngineStatus HuntmasterAudioEngine::playMasterCall(
    const std::string &callId) {
    return pimpl->playMasterCall(std::string(callId));
}

HuntmasterAudioEngine::EngineStatus HuntmasterAudioEngine::playRecording(
    const std::string &filename) {
    return pimpl->playRecording(std::string(filename));
}

void HuntmasterAudioEngine::stopPlayback() { pimpl->stopPlayback(); }

// ============================================================================
// C API Implementation (for WASM/FFI)
// ============================================================================

extern "C" {

int createEngine() {
    try {
        HuntmasterAudioEngine::getInstance().initialize();
        return 1;  // Success
    } catch (const std::exception &e) {
        std::cerr << "[C API] createEngine failed: " << e.what() << std::endl;
        return -1;
    }
}

void destroyEngine(int /*engineId*/) {
    try {
        HuntmasterAudioEngine::getInstance().shutdown();
    } catch (const std::exception &e) {
        std::cerr << "[C API] destroyEngine failed: " << e.what() << std::endl;
    }
}

int loadMasterCall(int /*engineId*/, const char *masterCallId) {
    if (!masterCallId) return -1;

    try {
        auto status = HuntmasterAudioEngine::getInstance().loadMasterCall(masterCallId);
        return status == HuntmasterAudioEngine::EngineStatus::OK ? 1 : -1;
    } catch (const std::exception &e) {
        std::cerr << "[C API] loadMasterCall failed: " << e.what() << std::endl;
        return -1;
    }
}

int startSession(int /*engineId*/) {
    try {
        // Default sample rate for web audio
        auto result = HuntmasterAudioEngine::getInstance().startRealtimeSession(44100.0f, 4096);
        return result.status == HuntmasterAudioEngine::EngineStatus::OK ? result.value : -1;
    } catch (const std::exception &e) {
        std::cerr << "[C API] startSession failed: " << e.what() << std::endl;
        return -1;
    }
}

int processAudioChunk(int /*engineId*/, int sessionId, const float *audioBuffer, int bufferSize,
                      float sampleRate) {
    if (!audioBuffer || bufferSize <= 0) return -1;

    try {
        // Note: sampleRate parameter is ignored as it should be set during session creation
        auto status = HuntmasterAudioEngine::getInstance().processAudioChunk(sessionId, audioBuffer,
                                                                             bufferSize);
        return status == HuntmasterAudioEngine::EngineStatus::OK ? 1 : -1;
    } catch (const std::exception &e) {
        std::cerr << "[C API] processAudioChunk failed: " << e.what() << std::endl;
        return -1;
    }
}

float getSimilarityScore(int /*engineId*/, int sessionId) {
    try {
        auto result = HuntmasterAudioEngine::getInstance().getSimilarityScore(sessionId);
        return result.status == HuntmasterAudioEngine::EngineStatus::OK ? result.value : -1.0f;
    } catch (const std::exception &e) {
        std::cerr << "[C API] getSimilarityScore failed: " << e.what() << std::endl;
        return -1.0f;
    }
}

void endSession(int /*engineId*/, int sessionId) {
    try {
        HuntmasterAudioEngine::getInstance().endRealtimeSession(sessionId);
    } catch (const std::exception &e) {
        std::cerr << "[C API] endSession failed: " << e.what() << std::endl;
    }
}

// int getSessionFeatureCount(int /*engineId*/, int sessionId) {
//     std::shared_lock lock(sessionsMutex_);
//     auto it = sessions_.find(sessionId);
//     if (it != sessions_.end()) {
//         return static_cast<int>(it->second.features.size());
//     }
//     return 0;
// }
int getSessionFeatureCount(int /*engineId*/, int sessionId) {
    try {
        // This now correctly calls the Impl method through the pimpl pointer
        return HuntmasterAudioEngine::getInstance().pimpl->getSessionFeatureCount(sessionId);
    } catch (const std::exception &e) {
        std::cerr << "[C API] getSessionFeatureCount failed: " << e.what() << std::endl;
        return -1;  // Return an error code on exception
    }
}
}  // extern "C"

}  // namespace huntmaster