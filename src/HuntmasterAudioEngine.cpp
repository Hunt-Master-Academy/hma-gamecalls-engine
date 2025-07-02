#include "../include/huntmaster_engine/HuntmasterAudioEngine.h"
#include "../include/huntmaster_engine/MFCCProcessor.h"
#include "../include/huntmaster_engine/DTWProcessor.h"
#include "../include/huntmaster_engine/AudioRecorder.h"
#include "../include/huntmaster_engine/AudioPlayer.h"
#include "../libs/dr_wav.h"

#include <algorithm>  // For std::remove_if, std::max, std::min, std::any_of
#include <atomic>     // For std::atomic
#include <chrono>     // For std::chrono
#include <cmath>      // For std::abs
#include <filesystem> // For std::filesystem::path
#include <fstream>    // For std::ifstream, std::ofstream
#include <iostream>   // For std::cout, std::cerr
#include <mutex>
#include <shared_mutex>
#include <vector>
#include <unordered_map>
#include <memory>
#include <span>

namespace huntmaster
{

    /**
     * @class DrWavRAII
     * @brief RAII wrapper for dr_wav allocated memory
     */
    class DrWavRAII
    {
    public:
        explicit DrWavRAII(float *data) : data_(data) {}
        ~DrWavRAII()
        {
            if (data_)
            {
                drwav_free(data_, nullptr);
            }
        }

        // Delete copy operations
        DrWavRAII(const DrWavRAII &) = delete;
        DrWavRAII &operator=(const DrWavRAII &) = delete;

        // Allow move operations
        DrWavRAII(DrWavRAII &&other) noexcept : data_(other.data_)
        {
            other.data_ = nullptr;
        }

        DrWavRAII &operator=(DrWavRAII &&other) noexcept
        {
            if (this != &other)
            {
                if (data_)
                {
                    drwav_free(data_, nullptr);
                }
                data_ = other.data_;
                other.data_ = nullptr;
            }
            return *this;
        }

        float *get() const { return data_; }
        float *release()
        {
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
    class HuntmasterAudioEngine::Impl
    {
    public:
        Impl();
        ~Impl();

        // Core functionality
        void initialize();
        void shutdown();

        // Master call management
        EngineStatus loadMasterCall(const std::string &masterCallId);

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

    private:
        // Configuration constants
        static constexpr float DEFAULT_ENERGY_THRESHOLD = 0.01f;
        static constexpr float DEFAULT_SILENCE_THRESHOLD = 0.02f;
        static constexpr size_t MAX_BUFFER_SIZE = 1048576; // 1MB
        static constexpr size_t FEATURE_VECTOR_SIZE = 13;

        // VAD configuration
        struct VADConfig
        {
            float energyThreshold = DEFAULT_ENERGY_THRESHOLD;
            float silenceThreshold = DEFAULT_SILENCE_THRESHOLD;
            float windowDurationMs = 20.0f;
            float minSoundDurationMs = 100.0f;
            float minSilenceDurationMs = 50.0f;
            float hangoverDurationMs = 100.0f;
        };

        // Session state with improved structure
        struct RealtimeSessionState
        {
            std::vector<float> audioBuffer; // Changed from deque for better performance
            size_t bufferWritePos = 0;
            std::vector<std::vector<float>> features;

            // VAD state
            bool isInSoundSegment = false;
            size_t consecutiveSoundFrames = 0;
            size_t consecutiveSilenceFrames = 0;
            std::vector<float> currentSegmentBuffer;
            size_t mfccProcessedFrames = 0;

            // Session metadata
            float sampleRate = 0.0f;
            std::chrono::steady_clock::time_point startTime;
        };

        struct RecordingSession
        {
            std::unique_ptr<AudioRecorder> recorder;
            int id;
            double sampleRate;
            std::chrono::steady_clock::time_point startTime;
        };

        // Thread-safe session management
        mutable std::shared_mutex sessionsMutex_;
        std::unordered_map<int, RealtimeSessionState> sessions_;
        std::atomic<int> nextSessionId_{1};

        // Thread-safe recording management
        mutable std::shared_mutex recordingsMutex_;
        std::unordered_map<int, RecordingSession> recordings_;
        std::atomic<int> nextRecordingId_{1};

        // Processors (thread-safe through internal implementation)
        std::unique_ptr<MFCCProcessor> mfccProcessor_;
        std::unique_ptr<AudioPlayer> audioPlayer_;

        // Master call data (protected by mutex)
        mutable std::shared_mutex masterCallMutex_;
        std::vector<std::vector<float>> masterCallFeatures_;
        std::string currentMasterCallId_;

        // Configuration
        VADConfig vadConfig_;
        std::string masterCallsPath_;
        std::string featuresPath_;
        std::string recordingsPath_;

        // Helper methods
        bool loadFeaturesFromFile(const std::string &masterCallId);
        void saveFeaturesToFile(const std::string &masterCallId);
        float calculateEnergy(std::span<const float> samples) const;
        std::vector<float> convertToMono(const float *interleavedData, size_t frames, unsigned int channels);
        void processVAD(RealtimeSessionState &session, std::span<const float> audioChunk);
        void extractMFCCFeatures(RealtimeSessionState &session);
    };

    // Constructor
    HuntmasterAudioEngine::Impl::Impl() = default;

    HuntmasterAudioEngine::Impl::~Impl()
    {
        shutdown();
    }

    void HuntmasterAudioEngine::Impl::initialize()
    {
        std::unique_lock lock(sessionsMutex_);

        audioPlayer_ = std::make_unique<AudioPlayer>();

        // Centralize path management with validation
        masterCallsPath_ = "../data/master_calls/";
        featuresPath_ = "../data/features/";
        recordingsPath_ = "../data/recordings/";

        // TODO: Validate paths exist and are writable

        std::cout << "[HuntmasterEngine] Initialized successfully." << std::endl;
    }

    void HuntmasterAudioEngine::Impl::shutdown()
    {
        // Stop all recordings
        {
            std::unique_lock lock(recordingsMutex_);
            for (auto &[id, session] : recordings_)
            {
                session.recorder->stopRecording();
            }
            recordings_.clear();
        }

        // Clear all sessions
        {
            std::unique_lock lock(sessionsMutex_);
            sessions_.clear();
        }

        // Stop playback
        if (audioPlayer_)
        {
            audioPlayer_->stop();
        }

        std::cout << "[HuntmasterEngine] Shutdown complete." << std::endl;
    }

    HuntmasterAudioEngine::EngineStatus
        HuntmasterAudioEngine::EngineStatus
        HuntmasterAudioEngine::Impl::loadMasterCall(std::string_view masterCallId_sv)
    {
        // Convert string_view to string for use with file paths and existing variables
        const std::string masterCallId(masterCallId_sv);

        std::unique_lock lock(masterCallMutex_);

        std::cout << "[HuntmasterEngine] Loading master call: " << masterCallId << std::endl;

        // Try loading pre-computed features first
        if (loadFeaturesFromFile(masterCallId))
        {
            currentMasterCallId_ = masterCallId;
            return EngineStatus::OK;
        }

        // Load and process audio file
        const std::string audioFilePath = masterCallsPath_ + masterCallId + ".wav";

        unsigned int channels;
        unsigned int sampleRate;
        drwav_uint64 totalPCMFrameCount;

        float *rawData = drwav_open_file_and_read_pcm_frames_f32(
            audioFilePath.c_str(), &channels, &sampleRate, &totalPCMFrameCount, nullptr);

        if (!rawData)
        {
            std::cerr << "[HuntmasterEngine] ERROR: Could not load WAV file: " << audioFilePath << std::endl;
            return EngineStatus::FILE_NOT_FOUND;
        }

        // RAII wrapper ensures cleanup
        DrWavRAII audioData(rawData);

        std::cout << "[HuntmasterEngine] Loaded audio - Channels: " << channels
                  << ", Sample Rate: " << sampleRate
                  << ", Frames: " << totalPCMFrameCount << std::endl;

        // Initialize or reconfigure MFCC processor
        if (!mfccProcessor_ || mfccProcessor_->getConfig().sampleRate != static_cast<float>(sampleRate))
        {
            MFCCProcessor::Config config;
            config.sampleRate = static_cast<float>(sampleRate);
            mfccProcessor_ = std::make_unique<MFCCProcessor>(config);
        }

        // Convert to mono if necessary
        std::vector<float> monoSamples;
        if (channels > 1)
        {
            monoSamples = convertToMono(audioData.get(), totalPCMFrameCount, channels);
        }
        else
        {
            monoSamples.assign(audioData.get(), audioData.get() + totalPCMFrameCount);
        }

        // Process MFCC features
        auto mfccFrames = mfccProcessor_->processBuffer(monoSamples.data(), monoSamples.size());

        masterCallFeatures_.clear();
        masterCallFeatures_.reserve(mfccFrames.size());

        for (const auto &frame : mfccFrames)
        {
            masterCallFeatures_.push_back(frame.coefficients);
        }

        std::cout << "[HuntmasterEngine] Processed " << masterCallFeatures_.size()
                  << " MFCC frames." << std::endl;

        // Save features for future use
        saveFeaturesToFile(masterCallId);

        currentMasterCallId_ = masterCallId;
        return EngineStatus::OK;
    }

    HuntmasterAudioEngine::Result<int>
    HuntmasterAudioEngine::Impl::startRealtimeSession(float sampleRate, int bufferSize)
    {
        if (sampleRate <= 0 || bufferSize <= 0)
        {
            return {-1, EngineStatus::INVALID_PARAMS};
        }

        std::unique_lock lock(sessionsMutex_);

        const int sessionId = nextSessionId_++;

        auto &session = sessions_[sessionId];
        session.sampleRate = sampleRate;
        session.audioBuffer.reserve(MAX_BUFFER_SIZE);
        session.startTime = std::chrono::steady_clock::now();

        // Configure MFCC processor for this sample rate
        if (!mfccProcessor_ || mfccProcessor_->getConfig().sampleRate != sampleRate)
        {
            MFCCProcessor::Config config;
            config.sampleRate = sampleRate;
            mfccProcessor_ = std::make_unique<MFCCProcessor>(config);
        }

        std::cout << "[HuntmasterEngine] Started realtime session " << sessionId
                  << " - Sample Rate: " << sampleRate
                  << ", Buffer Size: " << bufferSize << std::endl;

        return {sessionId, EngineStatus::OK};
    }

    HuntmasterAudioEngine::EngineStatus
    HuntmasterAudioEngine::Impl::processAudioChunk(int sessionId, std::span<const float> audioBuffer)
    {
        std::shared_lock lock(sessionsMutex_);

        auto it = sessions_.find(sessionId);
        if (it == sessions_.end())
        {
            return EngineStatus::INVALID_SESSION;
        }

        auto &session = it->second;
        lock.unlock();

        // Validate buffer size
        if (audioBuffer.size() > MAX_BUFFER_SIZE - session.audioBuffer.size())
        {
            return EngineStatus::BUFFER_OVERFLOW;
        }

        // Process VAD and extract features
        processVAD(session, audioBuffer);

        return EngineStatus::OK;
    }

    void HuntmasterAudioEngine::Impl::processVAD(RealtimeSessionState &session,
                                                 std::span<const float> audioChunk)
    {
        // Append new audio to buffer
        session.audioBuffer.insert(session.audioBuffer.end(),
                                   audioChunk.begin(), audioChunk.end());

        const float sampleRate = session.sampleRate;
        const size_t windowSamples = static_cast<size_t>(
            vadConfig_.windowDurationMs * sampleRate / 1000.0f);

        if (windowSamples == 0)
            return;

        const size_t minSoundSamples = static_cast<size_t>(
            vadConfig_.minSoundDurationMs * sampleRate / 1000.0f);
        const size_t minSilenceSamples = static_cast<size_t>(
            vadConfig_.minSilenceDurationMs * sampleRate / 1000.0f);
        const size_t hangoverSamples = static_cast<size_t>(
            vadConfig_.hangoverDurationMs * sampleRate / 1000.0f);

        // Process windows
        while (session.bufferWritePos + windowSamples <= session.audioBuffer.size())
        {
            const std::span<const float> window(
                session.audioBuffer.data() + session.bufferWritePos, windowSamples);

            const float energy = calculateEnergy(window);
            const float peak = *std::max_element(window.begin(), window.end(),
                                                 [](float a, float b)
                                                 { return std::abs(a) < std::abs(b); });

            const bool isActive = (energy > vadConfig_.energyThreshold ||
                                   std::abs(peak) > vadConfig_.silenceThreshold);

            if (isActive)
            {
                session.consecutiveSoundFrames += windowSamples;
                session.consecutiveSilenceFrames = 0;

                if (!session.isInSoundSegment &&
                    session.consecutiveSoundFrames >= minSoundSamples)
                {
                    session.isInSoundSegment = true;
                    std::cout << "[VAD] Sound detected" << std::endl;
                }
            }
            else
            {
                session.consecutiveSilenceFrames += windowSamples;
                session.consecutiveSoundFrames = 0;

                if (session.isInSoundSegment &&
                    session.consecutiveSilenceFrames >= minSilenceSamples)
                {
                    session.isInSoundSegment = false;
                    std::cout << "[VAD] Silence detected, processing segment" << std::endl;

                    // Process the accumulated segment
                    extractMFCCFeatures(session);
                    session.currentSegmentBuffer.clear();
                    session.mfccProcessedFrames = 0;
                }
            }

            // Accumulate audio for active segments
            if (session.isInSoundSegment ||
                (session.consecutiveSilenceFrames > 0 &&
                 session.consecutiveSilenceFrames <= hangoverSamples))
            {
                session.currentSegmentBuffer.insert(
                    session.currentSegmentBuffer.end(),
                    window.begin(), window.end());
            }

            session.bufferWritePos += windowSamples;
        }

        // Clean up processed audio
        if (session.bufferWritePos > 0)
        {
            session.audioBuffer.erase(
                session.audioBuffer.begin(),
                session.audioBuffer.begin() + session.bufferWritePos);
            session.bufferWritePos = 0;
        }
    }

    void HuntmasterAudioEngine::Impl::extractMFCCFeatures(RealtimeSessionState &session)
    {
        if (!mfccProcessor_ || session.currentSegmentBuffer.empty())
        {
            return;
        }

        auto mfccFrames = mfccProcessor_->processBuffer(
            session.currentSegmentBuffer.data(),
            session.currentSegmentBuffer.size());

        session.features.reserve(session.features.size() + mfccFrames.size());
        for (const auto &frame : mfccFrames)
        {
            session.features.push_back(frame.coefficients);
        }
    }

    HuntmasterAudioEngine::Result<float>
    HuntmasterAudioEngine::Impl::getSimilarityScore(int sessionId)
    {
        std::shared_lock sessionLock(sessionsMutex_);

        auto it = sessions_.find(sessionId);
        if (it == sessions_.end())
        {
            return {0.0f, EngineStatus::INVALID_SESSION};
        }

        const auto &sessionFeatures = it->second.features;
        sessionLock.unlock();

        std::shared_lock masterLock(masterCallMutex_);

        if (masterCallFeatures_.empty() || sessionFeatures.empty())
        {
            return {0.0f, EngineStatus::INSUFFICIENT_DATA};
        }

        const float distance = DTWProcessor::calculateDistance(
            masterCallFeatures_, sessionFeatures);

        masterLock.unlock();

        // Convert distance to similarity score (0-1 range)
        const float score = 1.0f / (1.0f + distance);

        std::cout << "[HuntmasterEngine] DTW Distance: " << distance
                  << ", Similarity Score: " << score << std::endl;

        return {score, EngineStatus::OK};
    }

    float HuntmasterAudioEngine::Impl::calculateEnergy(std::span<const float> samples) const
    {
        if (samples.empty())
            return 0.0f;

        float sum = 0.0f;
        for (float sample : samples)
        {
            sum += sample * sample;
        }

        return sum / static_cast<float>(samples.size());
    }

    std::vector<float> HuntmasterAudioEngine::Impl::convertToMono(
        const float *interleavedData, size_t frames, unsigned int channels)
    {

        std::vector<float> mono(frames);
        const float channelScale = 1.0f / static_cast<float>(channels);

        for (size_t i = 0; i < frames; ++i)
        {
            float sum = 0.0f;
            for (unsigned int ch = 0; ch < channels; ++ch)
            {
                sum += interleavedData[i * channels + ch];
            }
            mono[i] = sum * channelScale;
        }

        return mono;
    }

    // ... Continuing from previous implementation ...

    bool HuntmasterAudioEngine::Impl::loadFeaturesFromFile(const std::string &masterCallId)
    {
        const std::string featureFilePath = featuresPath_ + masterCallId + ".mfc";
        std::ifstream inFile(featureFilePath, std::ios::binary);

        if (!inFile.is_open())
        {
            return false;
        }

        // Read header
        uint32_t numFrames = 0;
        uint32_t numCoeffs = 0;

        inFile.read(reinterpret_cast<char *>(&numFrames), sizeof(numFrames));
        inFile.read(reinterpret_cast<char *>(&numCoeffs), sizeof(numCoeffs));

        if (!inFile.good() || numFrames == 0 || numCoeffs == 0 || numCoeffs > FEATURE_VECTOR_SIZE * 2)
        {
            std::cerr << "[HuntmasterEngine] Invalid feature file header: " << featureFilePath << std::endl;
            return false;
        }

        // Validate file size matches expected data
        const size_t expectedBytes = numFrames * numCoeffs * sizeof(float);
        inFile.seekg(0, std::ios::end);
        const size_t fileSize = static_cast<size_t>(inFile.tellg());
        const size_t headerSize = 2 * sizeof(uint32_t);

        if (fileSize < headerSize + expectedBytes)
        {
            std::cerr << "[HuntmasterEngine] Feature file size mismatch: " << featureFilePath << std::endl;
            return false;
        }

        inFile.seekg(headerSize, std::ios::beg);

        // Read features
        masterCallFeatures_.clear();
        masterCallFeatures_.reserve(numFrames);

        std::vector<float> buffer(numCoeffs);

        for (uint32_t i = 0; i < numFrames; ++i)
        {
            inFile.read(reinterpret_cast<char *>(buffer.data()), numCoeffs * sizeof(float));

            if (!inFile.good())
            {
                std::cerr << "[HuntmasterEngine] Error reading feature frame " << i << std::endl;
                masterCallFeatures_.clear();
                return false;
            }

            masterCallFeatures_.push_back(buffer);
        }

        std::cout << "[HuntmasterEngine] Loaded " << numFrames
                  << " feature frames from " << featureFilePath << std::endl;

        return true;
    }

    void HuntmasterAudioEngine::Impl::saveFeaturesToFile(const std::string &masterCallId)
    {
        // const std::string featureFilePath = featuresPath_ + masterCallId + ".mfc";
        std::filesystem::path featureFilePath = std::filesystem::path(featuresPath_) / (masterCallId + ".mfc");

        // Create directory if it doesn't exist
        // Note: In production, use std::filesystem::create_directories

        std::ofstream outFile(featureFilePath, std::ios::binary);

        if (!outFile.is_open())
        {
            std::cerr << "[HuntmasterEngine] Could not create feature file: " << featureFilePath << std::endl;
            return;
        }

        const uint32_t numFrames = static_cast<uint32_t>(masterCallFeatures_.size());
        const uint32_t numCoeffs = numFrames > 0 ? static_cast<uint32_t>(masterCallFeatures_[0].size()) : 0;

        // Write header
        outFile.write(reinterpret_cast<const char *>(&numFrames), sizeof(numFrames));
        outFile.write(reinterpret_cast<const char *>(&numCoeffs), sizeof(numCoeffs));

        // Write features
        for (const auto &frame : masterCallFeatures_)
        {
            outFile.write(reinterpret_cast<const char *>(frame.data()),
                          frame.size() * sizeof(float));
        }

        if (!outFile.good())
        {
            std::cerr << "[HuntmasterEngine] Error writing feature file: " << featureFilePath << std::endl;
            // Consider deleting the partial file
            return;
        }

        std::cout << "[HuntmasterEngine] Saved " << numFrames
                  << " feature frames to " << featureFilePath << std::endl;
    }

    void HuntmasterAudioEngine::Impl::endRealtimeSession(int sessionId)
    {
        std::unique_lock lock(sessionsMutex_);

        auto it = sessions_.find(sessionId);
        if (it != sessions_.end())
        {
            const auto duration = std::chrono::steady_clock::now() - it->second.startTime;
            const auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

            // Using std::format for cleaner output (requires C++20)
            // If C++20 is not available, use traditional iostream formatting.
            std::cout << "[HuntmasterEngine] Ended session " << sessionId << " (duration: " << durationMs << "ms, features: " << it->second.features.size() << ")\n";
            sessionId,
                durationMs,
                it->second.features.size()
                    sessions_.erase(it););
        }
    }

    // Recording Management

    HuntmasterAudioEngine::Result<int>
    HuntmasterAudioEngine::Impl::startRecording(double sampleRate)
    {
        if (sampleRate <= 0)
        {
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
        config.channels = 1;      // Mono recording
        config.bufferSize = 4096; // Reasonable default

        if (!session.recorder->startRecording(config))
        {
            recordings_.erase(recordingId);
            return {-1, EngineStatus::RECORDER_INIT_FAILED};
        }

        std::cout << "[HuntmasterEngine] Started recording " << recordingId
                  << " at " << sampleRate << "Hz" << std::endl;

        return {recordingId, EngineStatus::OK};
    }

    void HuntmasterAudioEngine::Impl::stopRecording(int recordingId)
    {
        std::unique_lock lock(recordingsMutex_);

        auto it = recordings_.find(recordingId);
        if (it != recordings_.end())
        {
            it->second.recorder->stopRecording();

            const auto duration = std::chrono::steady_clock::now() - it->second.startTime;
            const auto durationSec = std::chrono::duration_cast<std::chrono::seconds>(duration).count();

            std::cout << "[HuntmasterEngine] Stopped recording " << recordingId
                      << " (duration: " << durationSec << "s)" << std::endl;
        }
    }

    HuntmasterAudioEngine::Result<std::string>
    HuntmasterAudioEngine::Impl::saveRecording(int recordingId, const std::string &filename)
    {
        if (filename.empty())
        {
            return {"", EngineStatus::INVALID_PARAMS};
        }

        std::unique_lock lock(recordingsMutex_);

        auto it = recordings_.find(recordingId);
        if (it == recordings_.end())
        {
            return {"", EngineStatus::INVALID_RECORDING_ID};
        }

        // Ensure recording is stopped
        it->second.recorder->stopRecording();

        // Sanitize filename (remove path separators)
        std::string safeFilename = filename;
        safeFilename.erase(
            std::remove_if(safeFilename.begin(), safeFilename.end(),
                           [](char c)
                           { return c == '/' || c == '\\'; }),
            safeFilename.end());

        // Add .wav extension if not present
        if (safeFilename.find(".wav") == std::string::npos)
        {
            safeFilename += ".wav";
        }

        const std::string fullPath = recordingsPath_ + safeFilename;

        if (!it->second.recorder->saveToWav(fullPath))
        {
            return {"", EngineStatus::FILE_WRITE_ERROR};
        }

        // Remove recording from active list after successful save
        recordings_.erase(it);

        std::cout << "[HuntmasterEngine] Saved recording to " << fullPath << std::endl;

        return {fullPath, EngineStatus::OK};
    }

    bool HuntmasterAudioEngine::Impl::isRecording() const
    {
        std::shared_lock lock(recordingsMutex_);

        return std::any_of(recordings_.begin(), recordings_.end(),
                           [](const auto &pair)
                           {
                               return pair.second.recorder && pair.second.recorder->isRecording();
                           });
    }

    float HuntmasterAudioEngine::Impl::getRecordingLevel() const
    {
        std::shared_lock lock(recordingsMutex_);

        float maxLevel = 0.0f;

        for (const auto &[id, session] : recordings_)
        {
            if (session.recorder)
            {
                maxLevel = std::max(maxLevel, session.recorder->getCurrentLevel());
            }
        }

        return maxLevel;
    }

    double HuntmasterAudioEngine::Impl::getRecordingDuration(int recordingId) const
    {
        std::shared_lock lock(recordingsMutex_);

        auto it = recordings_.find(recordingId);
        if (it != recordings_.end() && it->second.recorder)
        {
            return it->second.recorder->getDuration();
        }

        return 0.0;
    }

    // Playback Management

    HuntmasterAudioEngine::EngineStatus
    HuntmasterAudioEngine::Impl::playMasterCall(const std::string &callId)
    {
        if (callId.empty())
        {
            return EngineStatus::INVALID_PARAMS;
        }

        const std::string filePath = masterCallsPath_ + callId + ".wav";

        if (!audioPlayer_)
        {
            return EngineStatus::PLAYER_NOT_INITIALIZED;
        }

        if (!audioPlayer_->loadFile(filePath))
        {
            std::cerr << "[HuntmasterEngine] Failed to load master call: " << filePath << std::endl;
            return EngineStatus::FILE_NOT_FOUND;
        }

        audioPlayer_->play();

        std::cout << "[HuntmasterEngine] Playing master call: " << callId << std::endl;

        return EngineStatus::OK;
    }

    HuntmasterAudioEngine::EngineStatus
    HuntmasterAudioEngine::Impl::playRecording(const std::string &filename)
    {
        if (filename.empty())
        {
            return EngineStatus::INVALID_PARAMS;
        }

        // Determine if it's a full path or just a filename
        const std::string filePath = (filename.find('/') != std::string::npos ||
                                      filename.find('\\') != std::string::npos)
                                         ? filename
                                         : recordingsPath_ + filename;

        if (!audioPlayer_)
        {
            return EngineStatus::PLAYER_NOT_INITIALIZED;
        }

        if (!audioPlayer_->loadFile(filePath))
        {
            std::cerr << "[HuntmasterEngine] Failed to load recording: " << filePath << std::endl;
            return EngineStatus::FILE_NOT_FOUND;
        }

        audioPlayer_->play();

        std::cout << "[HuntmasterEngine] Playing recording: " << filename << std::endl;

        return EngineStatus::OK;
    }

    void HuntmasterAudioEngine::Impl::stopPlayback()
    {
        if (audioPlayer_)
        {
            audioPlayer_->stop();
            std::cout << "[HuntmasterEngine] Playback stopped" << std::endl;
        }
    }

    // ============================================================================
    // Main HuntmasterAudioEngine Class Implementation
    // ============================================================================

    HuntmasterAudioEngine::HuntmasterAudioEngine()
        : pimpl(std::make_unique<Impl>())
    {
    }

    HuntmasterAudioEngine::~HuntmasterAudioEngine() = default;

    HuntmasterAudioEngine &HuntmasterAudioEngine::getInstance()
    {
        static HuntmasterAudioEngine instance;
        return instance;
    }

    void HuntmasterAudioEngine::initialize()
    {
        pimpl->initialize();
    }

    void HuntmasterAudioEngine::shutdown()
    {
        pimpl->shutdown();
    }

    // Delegating methods

    HuntmasterAudioEngine::EngineStatus
    HuntmasterAudioEngine::loadMasterCall(const std::string &masterCallId)
    {
        return pimpl->loadMasterCall(masterCallId);
    }

    HuntmasterAudioEngine::Result<int>
    HuntmasterAudioEngine::startRealtimeSession(float sampleRate, int bufferSize)
    {
        return pimpl->startRealtimeSession(sampleRate, bufferSize);
    }

    HuntmasterAudioEngine::EngineStatus
    HuntmasterAudioEngine::processAudioChunk(int sessionId, const float *audioBuffer, int bufferSize)
    {
        if (!audioBuffer || bufferSize <= 0)
        {
            return EngineStatus::INVALID_PARAMS;
        }

        // Create span from raw pointer
        const std::span<const float> audioSpan(audioBuffer, bufferSize);
        return pimpl->processAudioChunk(sessionId, audioSpan);
    }

    HuntmasterAudioEngine::Result<float>
    HuntmasterAudioEngine::getSimilarityScore(int sessionId)
    {
        return pimpl->getSimilarityScore(sessionId);
    }

    void HuntmasterAudioEngine::endRealtimeSession(int sessionId)
    {
        pimpl->endRealtimeSession(sessionId);
    }

    HuntmasterAudioEngine::Result<int>
    HuntmasterAudioEngine::startRecording(double sampleRate)
    {
        return pimpl->startRecording(sampleRate);
    }

    void HuntmasterAudioEngine::stopRecording(int recordingId)
    {
        pimpl->stopRecording(recordingId);
    }

    HuntmasterAudioEngine::Result<std::string>
    HuntmasterAudioEngine::saveRecording(int recordingId, const std::string &filename)
    {
        return pimpl->saveRecording(recordingId, std::string(filename));
    }

    bool HuntmasterAudioEngine::isRecording() const
    {
        return pimpl->isRecording();
    }

    float HuntmasterAudioEngine::getRecordingLevel() const
    {
        return pimpl->getRecordingLevel();
    }

    double HuntmasterAudioEngine::getRecordingDuration(int recordingId) const
    {
        return pimpl->getRecordingDuration(recordingId);
    }

    HuntmasterAudioEngine::EngineStatus
    HuntmasterAudioEngine::playMasterCall(const std::string &callId)
    {
        return pimpl->playMasterCall(std::string(callId));
    }

    HuntmasterAudioEngine::EngineStatus
    HuntmasterAudioEngine::playRecording(const std::string &filename)
    {
        return pimpl->playRecording(std::string(filename));
    }

    void HuntmasterAudioEngine::stopPlayback()
    {
        pimpl->stopPlayback();
    }

    // ============================================================================
    // C API Implementation (for WASM/FFI)
    // ============================================================================

    extern "C"
    {

        int createEngine()
        {
            try
            {
                HuntmasterAudioEngine::getInstance().initialize();
                return 1; // Success
            }
            catch (const std::exception &e)
            {
                std::cerr << "[C API] createEngine failed: " << e.what() << std::endl;
                return -1;
            }
        }

        void destroyEngine(int /*engineId*/)
        {
            try
            {
                HuntmasterAudioEngine::getInstance().shutdown();
            }
            catch (const std::exception &e)
            {
                std::cerr << "[C API] destroyEngine failed: " << e.what() << std::endl;
            }
        }

        int loadMasterCall(int /*engineId*/, const char *masterCallId)
        {
            if (!masterCallId)
                return -1;

            try
            {
                auto status = HuntmasterAudioEngine::getInstance().loadMasterCall(masterCallId);
                return status == HuntmasterAudioEngine::EngineStatus::OK ? 1 : -1;
            }
            catch (const std::exception &e)
            {
                std::cerr << "[C API] loadMasterCall failed: " << e.what() << std::endl;
                return -1;
            }
        }

        int startSession(int /*engineId*/)
        {
            try
            {
                // Default sample rate for web audio
                auto result = HuntmasterAudioEngine::getInstance().startRealtimeSession(44100.0f, 4096);
                return result.status == HuntmasterAudioEngine::EngineStatus::OK ? result.value : -1;
            }
            catch (const std::exception &e)
            {
                std::cerr << "[C API] startSession failed: " << e.what() << std::endl;
                return -1;
            }
        }

        int processAudioChunk(int /*engineId*/, int sessionId,
                              const float *audioBuffer, int bufferSize, float sampleRate)
        {
            if (!audioBuffer || bufferSize <= 0)
                return -1;

            try
            {
                // Note: sampleRate parameter is ignored as it should be set during session creation
                auto status = HuntmasterAudioEngine::getInstance().processAudioChunk(
                    sessionId, audioBuffer, bufferSize);
                return status == HuntmasterAudioEngine::EngineStatus::OK ? 1 : -1;
            }
            catch (const std::exception &e)
            {
                std::cerr << "[C API] processAudioChunk failed: " << e.what() << std::endl;
                return -1;
            }
        }

        float getSimilarityScore(int /*engineId*/, int sessionId)
        {
            try
            {
                auto result = HuntmasterAudioEngine::getInstance().getSimilarityScore(sessionId);
                return result.status == HuntmasterAudioEngine::EngineStatus::OK ? result.value : -1.0f;
            }
            catch (const std::exception &e)
            {
                std::cerr << "[C API] getSimilarityScore failed: " << e.what() << std::endl;
                return -1.0f;
            }
        }

        void endSession(int /*engineId*/, int sessionId)
        {
            try
            {
                HuntmasterAudioEngine::getInstance().endRealtimeSession(sessionId);
            }
            catch (const std::exception &e)
            {
                std::cerr << "[C API] endSession failed: " << e.what() << std::endl;
            }
        }

        int getSessionFeatureCount(int /*engineId*/, int sessionId)
        {
            std::shared_lock lock(sessionsMutex_);
            auto it = sessions_.find(sessionId);
            if (it != sessions_.end())
            {
                return static_cast<int>(it->second.features.size());
            }
            return 0;
        }

    } // extern "C"

} // namespace huntmaster