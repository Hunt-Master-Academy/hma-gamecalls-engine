#include "huntmaster/core/HuntmasterAudioEngine.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <span>
#include <stdexcept>

// Include the necessary headers for the components used
#include "dr_wav.h"
#include "huntmaster/core/AudioPlayer.h"
#include "huntmaster/core/AudioRecorder.h"
#include "huntmaster/core/DTWProcessor.h"
#include "huntmaster/core/MFCCProcessor.h"
#include "huntmaster/core/VoiceActivityDetector.h"

namespace huntmaster {

// RAII wrapper for dr_wav memory
class DrWavRAII {
   public:
    explicit DrWavRAII(float *data) : data_(data) {}
    ~DrWavRAII() {
        if (data_) {
            drwav_free(data_, nullptr);
        }
    }
    DrWavRAII(const DrWavRAII &) = delete;
    DrWavRAII &operator=(const DrWavRAII &) = delete;
    DrWavRAII(DrWavRAII &&other) noexcept : data_(other.data_) { other.data_ = nullptr; }
    DrWavRAII &operator=(DrWavRAII &&other) noexcept {
        if (this != &other) {
            if (data_) drwav_free(data_, nullptr);
            data_ = other.data_;
            other.data_ = nullptr;
        }
        return *this;
    }
    float *get() const { return data_; }

   private:
    float *data_;
};

// Private implementation of the audio engine
class HuntmasterAudioEngine::Impl {
   public:
    Impl();
    ~Impl();

    // Core functionality
    void initialize();
    void shutdown();

    // Master call management
    EngineStatus loadMasterCall(std::string_view masterCallId);

    // Real-time session management
    Result<int> startRealtimeSession(float sampleRate, int bufferSize);
    EngineStatus processAudioChunk(int sessionId, std::span<const float> audioBuffer);
    Result<float> getSimilarityScore(int sessionId);
    void endRealtimeSession(int sessionId);

    // Recording management
    Result<int> startRecording(double sampleRate);
    void stopRecording(int recordingId);
    Result<std::string> saveRecording(int recordingId, std::string_view filename);

    // Playback
    EngineStatus playMasterCall(std::string_view callId);
    EngineStatus playRecording(std::string_view filename);
    void stopPlayback();

    // Status queries
    bool isRecording() const;
    float getRecordingLevel() const;
    double getRecordingDuration(int recordingId) const;
    int getSessionFeatureCount(int sessionId) const;

   private:
    // Internal data structures
    struct RealtimeSessionState {
        std::vector<float> audioBuffer;
        size_t bufferWritePos = 0;
        std::vector<std::vector<float>> features;
        bool isInSoundSegment = false;
        std::vector<float> currentSegmentBuffer;
        float sampleRate = 0.0f;
        std::chrono::steady_clock::time_point startTime;
    };

    struct RecordingSession {
        std::unique_ptr<AudioRecorder> recorder;
        std::chrono::steady_clock::time_point startTime;
    };

    // Thread-safe containers
    mutable std::shared_mutex sessionsMutex_;
    std::unordered_map<int, RealtimeSessionState> sessions_;
    std::atomic<int> nextSessionId_{1};

    mutable std::shared_mutex recordingsMutex_;
    std::unordered_map<int, RecordingSession> recordings_;
    std::atomic<int> nextRecordingId_{1};

    mutable std::shared_mutex masterCallMutex_;
    std::vector<std::vector<float>> masterCallFeatures_;
    std::string currentMasterCallId_;

    // Processors
    std::unique_ptr<MFCCProcessor> mfccProcessor_;
    std::unique_ptr<VoiceActivityDetector> vad_;
    std::unique_ptr<AudioPlayer> audioPlayer_;

    // Configuration paths
    std::string masterCallsPath_;
    std::string featuresPath_;
    std::string recordingsPath_;

    // Helper methods
    bool loadFeaturesFromFile(const std::string &masterCallId);
    void saveFeaturesToFile(const std::string &masterCallId);
    void extractMFCCFeatures(RealtimeSessionState &session);
};

// --- Constructor / Destructor ---
HuntmasterAudioEngine::Impl::Impl() = default;
HuntmasterAudioEngine::Impl::~Impl() { shutdown(); }

// --- Core Functionality ---
void HuntmasterAudioEngine::Impl::initialize() {
    audioPlayer_ = std::make_unique<AudioPlayer>();

    // Initialize VAD with a default configuration
    VoiceActivityDetector::Config vad_config;
    vad_ = std::make_unique<VoiceActivityDetector>(vad_config);

    // Initialize paths relative to the executable working directory
    // When running from build directory, we need to go up one level
    masterCallsPath_ = "../data/master_calls/";
    featuresPath_ = "../data/features/";
    recordingsPath_ = "../data/recordings/";
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

// --- Master Call Management ---
HuntmasterAudioEngine::EngineStatus HuntmasterAudioEngine::Impl::loadMasterCall(
    std::string_view masterCallId_sv) {
    const std::string masterCallId(masterCallId_sv);
    std::unique_lock lock(masterCallMutex_);
    if (loadFeaturesFromFile(masterCallId)) {
        currentMasterCallId_ = masterCallId;
        return EngineStatus::OK;
    }

    const std::string audioFilePath = masterCallsPath_ + masterCallId + ".wav";
    unsigned int channels, sampleRate;
    drwav_uint64 totalPCMFrameCount;
    float *rawData = drwav_open_file_and_read_pcm_frames_f32(
        audioFilePath.c_str(), &channels, &sampleRate, &totalPCMFrameCount, nullptr);
    if (!rawData) {
        return EngineStatus::FILE_NOT_FOUND;
    }
    DrWavRAII audioData(rawData);

    // Use consistent MFCC configuration
    MFCCProcessor::Config mfcc_config;
    mfcc_config.sample_rate = static_cast<float>(sampleRate);
    mfcc_config.frame_size = 512;  // Standard frame size
    mfcc_config.num_coefficients = 13;
    mfcc_config.num_filters = 26;
    mfccProcessor_ = std::make_unique<MFCCProcessor>(mfcc_config);

    std::vector<float> monoSamples(totalPCMFrameCount);
    if (channels > 1) {
        for (drwav_uint64 i = 0; i < totalPCMFrameCount; ++i) {
            float sample_sum = 0;
            for (unsigned int j = 0; j < channels; ++j) {
                sample_sum += rawData[i * channels + j];
            }
            monoSamples[i] = sample_sum / static_cast<float>(channels);
        }
    } else {
        std::copy(rawData, rawData + totalPCMFrameCount, monoSamples.begin());
    }

    // Use 50% overlap (256 samples hop size for 512 frame size)
    auto features_result = mfccProcessor_->extractFeaturesFromBuffer(monoSamples, 256);
    if (!features_result) {
        return EngineStatus::PROCESSING_ERROR;
    }

    masterCallFeatures_ = std::move(*features_result);
    saveFeaturesToFile(masterCallId);
    currentMasterCallId_ = masterCallId;
    return EngineStatus::OK;
}

// --- Real-time Session Management ---
HuntmasterAudioEngine::Result<int> HuntmasterAudioEngine::Impl::startRealtimeSession(
    float sampleRate, int bufferSize) {
    if (sampleRate <= 0 || bufferSize <= 0) {
        return {-1, EngineStatus::INVALID_PARAMS};
    }

    std::unique_lock lock(sessionsMutex_);
    const int sessionId = nextSessionId_++;
    auto &session = sessions_[sessionId];
    session.sampleRate = sampleRate;
    session.startTime = std::chrono::steady_clock::now();

    // Use standard MFCC configuration instead of buffer-size dependent
    MFCCProcessor::Config mfcc_config;
    mfcc_config.sample_rate = sampleRate;
    mfcc_config.frame_size = 512;  // Standard 512-sample frame (11.6ms at 44.1kHz)
    mfcc_config.num_coefficients = 13;
    mfcc_config.num_filters = 26;
    mfccProcessor_ = std::make_unique<MFCCProcessor>(mfcc_config);

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

    // Directly extract MFCC features from the audio chunk instead of relying on VAD
    // This fixes the "0 features extracted" issue that was blocking similarity scoring
    if (!mfccProcessor_) return EngineStatus::PROCESSING_ERROR;

    // Store the audio for feature extraction
    session.currentSegmentBuffer.insert(session.currentSegmentBuffer.end(), audioBuffer.begin(),
                                        audioBuffer.end());

    // Process features from accumulated buffer
    extractMFCCFeatures(session);

    // Optional: Keep VAD for future enhancements, but don't gate feature extraction
    if (vad_) {
        const size_t windowSamples = 512;
        for (size_t i = 0; i + windowSamples <= audioBuffer.size(); i += windowSamples) {
            auto window = audioBuffer.subspan(i, windowSamples);
            auto vad_result = vad_->processWindow(window);
            if (vad_result) {
                session.isInSoundSegment = vad_result->is_active;
                // VAD result available for future use, but not blocking feature extraction
            }
        }
    }

    return EngineStatus::OK;
}

void HuntmasterAudioEngine::Impl::extractMFCCFeatures(RealtimeSessionState &session) {
    if (!mfccProcessor_ || session.currentSegmentBuffer.empty()) {
        return;
    }

    // Use hop size that's compatible with frame size (50% overlap is standard)
    size_t frameSize = 512;          // Standard MFCC frame size
    size_t hopSize = frameSize / 2;  // 50% overlap (256 samples)

    auto features_result =
        mfccProcessor_->extractFeaturesFromBuffer(session.currentSegmentBuffer, hopSize);
    if (features_result) {
        session.features.insert(session.features.end(), features_result->begin(),
                                features_result->end());
    }

    // Clear the buffer to prevent unbounded memory growth
    // Keep only the last frame for continuity in streaming processing
    if (session.currentSegmentBuffer.size() > frameSize) {
        std::vector<float> overlap(session.currentSegmentBuffer.end() - frameSize,
                                   session.currentSegmentBuffer.end());
        session.currentSegmentBuffer = std::move(overlap);
    }
}

HuntmasterAudioEngine::Result<float> HuntmasterAudioEngine::Impl::getSimilarityScore(
    int sessionId) {
    std::shared_lock sessionLock(sessionsMutex_);
    auto it = sessions_.find(sessionId);
    if (it == sessions_.end()) return {0.0f, EngineStatus::INVALID_SESSION};
    const auto &sessionFeatures = it->second.features;
    sessionLock.unlock();
 
    std::shared_lock masterLock(masterCallMutex_);
    // A master call must be loaded to get a score.
    if (masterCallFeatures_.empty()) {
        return {0.0f, EngineStatus::INSUFFICIENT_DATA};
    }
    // If no audio has been processed in the session, the similarity is zero.
    if (sessionFeatures.empty()) {
        return {0.0f, EngineStatus::OK};
    }

    const float distance = DTWProcessor::calculateDistance(masterCallFeatures_, sessionFeatures);
    const float score = 1.0f / (1.0f + distance);
    return {score, EngineStatus::OK};
}

void HuntmasterAudioEngine::Impl::endRealtimeSession(int sessionId) {
    std::unique_lock lock(sessionsMutex_);
    sessions_.erase(sessionId);
}

// --- Recording Management ---
HuntmasterAudioEngine::Result<int> HuntmasterAudioEngine::Impl::startRecording(double sampleRate) {
    std::unique_lock lock(recordingsMutex_);
    const int recordingId = nextRecordingId_++;
    auto &session = recordings_[recordingId];
    session.recorder = std::make_unique<AudioRecorder>();
    session.startTime = std::chrono::steady_clock::now();

    AudioRecorder::Config config;
    config.sampleRate = static_cast<int>(sampleRate);
    if (!session.recorder->startRecording(config)) {
        recordings_.erase(recordingId);
        return {-1, EngineStatus::RECORDER_INIT_FAILED};
    }
    return {recordingId, EngineStatus::OK};
}

void HuntmasterAudioEngine::Impl::stopRecording(int recordingId) {
    std::unique_lock lock(recordingsMutex_);
    auto it = recordings_.find(recordingId);
    if (it != recordings_.end()) {
        it->second.recorder->stopRecording();
    }
}

HuntmasterAudioEngine::Result<std::string> HuntmasterAudioEngine::Impl::saveRecording(
    int recordingId, std::string_view filename_sv) {
    std::string filename(filename_sv);
    if (filename.empty()) return {"", EngineStatus::INVALID_PARAMS};

    std::unique_lock lock(recordingsMutex_);
    auto it = recordings_.find(recordingId);
    if (it == recordings_.end()) return {"", EngineStatus::INVALID_RECORDING_ID};

    it->second.recorder->stopRecording();
    const std::string fullPath = recordingsPath_ + filename;
    if (!it->second.recorder->saveToWav(fullPath)) {
        return {"", EngineStatus::FILE_WRITE_ERROR};
    }
    recordings_.erase(it);
    return {fullPath, EngineStatus::OK};
}

// --- Playback ---
HuntmasterAudioEngine::EngineStatus HuntmasterAudioEngine::Impl::playMasterCall(
    std::string_view callId_sv) {
    const std::string filePath = masterCallsPath_ + std::string(callId_sv) + ".wav";
    if (!audioPlayer_ || !audioPlayer_->loadFile(filePath)) return EngineStatus::FILE_NOT_FOUND;
    audioPlayer_->play();
    return EngineStatus::OK;
}

HuntmasterAudioEngine::EngineStatus HuntmasterAudioEngine::Impl::playRecording(
    std::string_view filename_sv) {
    const std::string filePath = recordingsPath_ + std::string(filename_sv);
    if (!audioPlayer_ || !audioPlayer_->loadFile(filePath)) return EngineStatus::FILE_NOT_FOUND;
    audioPlayer_->play();
    return EngineStatus::OK;
}

void HuntmasterAudioEngine::Impl::stopPlayback() {
    if (audioPlayer_) audioPlayer_->stop();
}

// --- Status Queries ---
bool HuntmasterAudioEngine::Impl::isRecording() const {
    std::shared_lock lock(recordingsMutex_);
    return !recordings_.empty();
}

float HuntmasterAudioEngine::Impl::getRecordingLevel() const {
    std::shared_lock lock(recordingsMutex_);
    if (recordings_.empty()) return 0.0f;
    return recordings_.begin()->second.recorder->getCurrentLevel();
}

double HuntmasterAudioEngine::Impl::getRecordingDuration(int recordingId) const {
    std::shared_lock lock(recordingsMutex_);
    auto it = recordings_.find(recordingId);
    if (it != recordings_.end()) return it->second.recorder->getDuration();
    return 0.0;
}

int HuntmasterAudioEngine::Impl::getSessionFeatureCount(int sessionId) const {
    std::shared_lock lock(sessionsMutex_);
    auto it = sessions_.find(sessionId);
    if (it != sessions_.end()) return static_cast<int>(it->second.features.size());
    return 0;
}

// --- Private Helper Methods ---
bool HuntmasterAudioEngine::Impl::loadFeaturesFromFile(const std::string &masterCallId) {
    const std::string featureFilePath = featuresPath_ + masterCallId + ".mfc";
    std::ifstream inFile(featureFilePath, std::ios::binary);
    if (!inFile) return false;

    uint32_t numFrames = 0, numCoeffs = 0;
    inFile.read(reinterpret_cast<char *>(&numFrames), sizeof(numFrames));
    inFile.read(reinterpret_cast<char *>(&numCoeffs), sizeof(numCoeffs));
    if (!inFile || numFrames == 0 || numCoeffs == 0) return false;

    masterCallFeatures_.assign(numFrames, std::vector<float>(numCoeffs));
    for (uint32_t i = 0; i < numFrames; ++i) {
        inFile.read(reinterpret_cast<char *>(masterCallFeatures_[i].data()),
                    numCoeffs * sizeof(float));
    }
    return inFile.good();
}

void HuntmasterAudioEngine::Impl::saveFeaturesToFile(const std::string &masterCallId) {
    const std::string featureFilePath = featuresPath_ + masterCallId + ".mfc";
    std::ofstream outFile(featureFilePath, std::ios::binary);
    if (!outFile || masterCallFeatures_.empty()) return;

    uint32_t numFrames = static_cast<uint32_t>(masterCallFeatures_.size());
    uint32_t numCoeffs = static_cast<uint32_t>(masterCallFeatures_[0].size());
    outFile.write(reinterpret_cast<const char *>(&numFrames), sizeof(numFrames));
    outFile.write(reinterpret_cast<const char *>(&numCoeffs), sizeof(numCoeffs));

    for (const auto &frame : masterCallFeatures_) {
        outFile.write(reinterpret_cast<const char *>(frame.data()), frame.size() * sizeof(float));
    }
}

// ============================================================================
// Main HuntmasterAudioEngine Class Implementation (Pimpl Delegation)
// ============================================================================

HuntmasterAudioEngine::HuntmasterAudioEngine() : pimpl(std::make_unique<Impl>()) {}
HuntmasterAudioEngine::~HuntmasterAudioEngine() = default;

HuntmasterAudioEngine &HuntmasterAudioEngine::getInstance() {
    static HuntmasterAudioEngine instance;
    return instance;
}

void HuntmasterAudioEngine::initialize() { pimpl->initialize(); }
void HuntmasterAudioEngine::shutdown() { pimpl->shutdown(); }

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
    if (!audioBuffer || bufferSize <= 0) return EngineStatus::INVALID_PARAMS;
    return pimpl->processAudioChunk(sessionId, {audioBuffer, static_cast<size_t>(bufferSize)});
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
    int recordingId, std::string_view filename) {
    return pimpl->saveRecording(recordingId, filename);
}
bool HuntmasterAudioEngine::isRecording() const { return pimpl->isRecording(); }
float HuntmasterAudioEngine::getRecordingLevel() const { return pimpl->getRecordingLevel(); }
double HuntmasterAudioEngine::getRecordingDuration(int recordingId) const {
    return pimpl->getRecordingDuration(recordingId);
}
int HuntmasterAudioEngine::getSessionFeatureCount(int sessionId) const {
    return pimpl->getSessionFeatureCount(sessionId);
}
HuntmasterAudioEngine::EngineStatus HuntmasterAudioEngine::playMasterCall(std::string_view callId) {
    return pimpl->playMasterCall(callId);
}
HuntmasterAudioEngine::EngineStatus HuntmasterAudioEngine::playRecording(
    std::string_view filename) {
    return pimpl->playRecording(filename);
}
void HuntmasterAudioEngine::stopPlayback() { pimpl->stopPlayback(); }

// ============================================================================
// C API Implementation (for WASM/FFI)
// ============================================================================
extern "C" {
int createEngine() {
    try {
        HuntmasterAudioEngine::getInstance().initialize();
        return 1;
    } catch (...) {
        return -1;
    }
}
void destroyEngine(int) {
    try {
        HuntmasterAudioEngine::getInstance().shutdown();
    } catch (...) {
    }
}
int loadMasterCall(int, const char *masterCallId) {
    if (!masterCallId) return -1;
    return HuntmasterAudioEngine::getInstance().loadMasterCall(masterCallId) ==
                   HuntmasterAudioEngine::EngineStatus::OK
               ? 1
               : -1;
}
int startSession(int) {
    auto result = HuntmasterAudioEngine::getInstance().startRealtimeSession(44100.0f, 4096);
    return result.status == HuntmasterAudioEngine::EngineStatus::OK ? result.value : -1;
}
int processAudioChunk(int, int sessionId, const float *audioBuffer, int bufferSize, float) {
    if (!audioBuffer || bufferSize <= 0) return -1;
    return HuntmasterAudioEngine::getInstance().processAudioChunk(
               sessionId, audioBuffer, bufferSize) == HuntmasterAudioEngine::EngineStatus::OK
               ? 1
               : -1;
}
float getSimilarityScore(int, int sessionId) {
    auto result = HuntmasterAudioEngine::getInstance().getSimilarityScore(sessionId);
    return result.status == HuntmasterAudioEngine::EngineStatus::OK ? result.value : -1.0f;
}
void endSession(int, int sessionId) {
    HuntmasterAudioEngine::getInstance().endRealtimeSession(sessionId);
}
int getSessionFeatureCount(int, int sessionId) {
    try {
        return HuntmasterAudioEngine::getInstance().getSessionFeatureCount(sessionId);
    } catch (...) {
        return -1;
    }
}
}  // extern "C"

}  // namespace huntmaster
