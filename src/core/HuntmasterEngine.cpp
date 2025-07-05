// File: HuntmasterEngine.cpp
#include "HuntmasterEngine.h"

#include <algorithm>
#include <barrier>
#include <latch>
#include <ranges>
#include <thread>
#include <vector>

#include "AudioBufferPool.h"
#include "DTWComparator.h"
#include "MFCCProcessor.h"
#include "RealtimeAudioProcessor.h"
#include "VoiceActivityDetector.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

namespace huntmaster {

/**
 * @struct RealtimeSession
 * @brief Represents an active audio processing session
 */
struct RealtimeSession {
    int id;
    std::chrono::steady_clock::time_point start_time;
    std::vector<float> accumulated_audio;
    bool is_active{true};
    std::atomic<size_t> frames_processed{0};
};

/**
 * @class HuntmasterEngine::Impl
 * @brief Private implementation using the Pimpl idiom
 */
class HuntmasterEngine::Impl {
   public:
    // Configuration
    PlatformEngineConfig config_;

    // Core components (using smart pointers for RAII)
    std::unique_ptr<AudioBufferPool> buffer_pool_;
    std::unique_ptr<VoiceActivityDetector> vad_;
    std::unique_ptr<MFCCProcessor> mfcc_processor_;
    std::unique_ptr<DTWComparator> dtw_comparator_;
    std::unique_ptr<RealtimeAudioProcessor> realtime_processor_;

    // Session management with thread safety
    mutable std::shared_mutex sessions_mutex_;
    std::unordered_map<int, std::unique_ptr<RealtimeSession>> sessions_;

    // Master call data
    mutable std::shared_mutex master_calls_mutex_;
    std::unordered_map<std::string, std::vector<std::vector<float>>> master_call_features_;

    // State tracking
    std::atomic<bool> initialized_{false};
    std::atomic<size_t> total_frames_processed_{0};

#ifndef __EMSCRIPTEN__
    // Native platform features
    std::jthread processing_thread_;
    std::stop_token stop_token_;
#endif

    // Add member for storing current session features
    struct SessionFeatures {
        std::vector<std::vector<float>> mfcc_frames;
        std::chrono::steady_clock::time_point last_update;
    };
    mutable std::shared_mutex session_features_mutex_;
    std::unordered_map<int, SessionFeatures> session_features_;

    /**
     * @brief Initialize all engine components
     */
    explicit Impl(const PlatformEngineConfig &config) : config_(config) {
        // Initialize buffer pool
        buffer_pool_ = std::make_unique<AudioBufferPool>(config.buffer_pool_size,
                                                         config.frame_size * sizeof(float));

        // Initialize VAD
        VoiceActivityDetector::Config vad_config{.energy_threshold = config.vad_energy_threshold,
                                                 .window_duration = config.vad_window_duration,
                                                 .sample_rate = config.sample_rate};
        vad_ = std::make_unique<VoiceActivityDetector>(vad_config);

        // Initialize MFCC processor
        MFCCProcessor::Config mfcc_config{.sample_rate = config.sample_rate,
                                          .frame_size = config.frame_size,
                                          .num_coefficients = config.mfcc_coefficients,
                                          .num_filters = 26};
        mfcc_processor_ = std::make_unique<MFCCProcessor>(mfcc_config);

        // Initialize DTW comparator
        dtw_comparator_ = std::make_unique<DTWComparator>();

        // Initialize real-time processor
        RealtimeAudioProcessor::Config rt_config{
            .ring_buffer_size = 256,  // Adjust based on latency requirements
            .chunk_size = config.frame_size,
            .enable_backpressure = false,  // No blocking in real-time
            .enable_metrics = true};
        realtime_processor_ = std::make_unique<RealtimeAudioProcessor>(rt_config);

#ifndef __EMSCRIPTEN__
        // Start background processing thread for native platforms
        if (config.thread_pool_size > 0) {
            startBackgroundProcessing();
        }
#endif

        initialized_.store(true, std::memory_order_release);
    }

    /**
     * @brief Cleanup and stop all processing
     */
    ~Impl() {
        // Signal shutdown
        initialized_.store(false, std::memory_order_release);

#ifndef __EMSCRIPTEN__
        // Stop background thread if running
        if (processing_thread_.joinable()) {
            processing_thread_.request_stop();
        }
#endif

        // Clear all sessions
        {
            std::unique_lock lock(sessions_mutex_);
            sessions_.clear();
        }
    }

    /**
     * @brief Process audio chunk with VAD and feature extraction
     */
    [[nodiscard]] std::expected<ProcessingResult, EngineError> processAudioChunk(
        std::span<const float> audio_data) {
        if (!initialized_.load(std::memory_order_acquire)) {
            return std::unexpected(EngineError{.status = EngineStatus::ERROR_NOT_INITIALIZED,
                                               .message = "Engine not initialized"});
        }

        // Validate input
        if (audio_data.empty() || audio_data.size() > config_.frame_size * 10) {
            return std::unexpected(EngineError{.status = EngineStatus::ERROR_INVALID_INPUT,
                                               .message = "Invalid audio data size"});
        }

        // Enqueue audio data into the real-time processor
        if (!realtime_processor_->tryEnqueueAudio(audio_data)) {
            return std::unexpected(EngineError{.status = EngineStatus::ERROR_BUFFER_OVERFLOW,
                                               .message = "Real-time buffer full"});
        }

        float similarity_score = 0.0f;
        size_t frames_processed = 0;

        // Process all available chunks
        while (auto chunk_opt = realtime_processor_->tryDequeueChunk()) {
            auto &chunk = *chunk_opt;

            // Run VAD on the chunk
            auto vad_result =
                vad_->processWindow(std::span(chunk.data.data(), chunk.valid_samples));
            if (!vad_result || !vad_result->is_active) {
                continue;  // Skip non-voice chunks
            }

            // Extract MFCC features for the chunk
            auto mfcc_frame = mfcc_processor_->processFrame(chunk.data.data());
            if (mfcc_frame.empty()) {
                continue;  // Skip if MFCC extraction failed
            }

            // For demonstration, accumulate features for a default session (e.g., session_id = 0)
            int session_id = 0;
            {
                std::unique_lock lock(session_features_mutex_);
                auto &session = session_features_[session_id];
                session.mfcc_frames.push_back(mfcc_frame);
                session.last_update = std::chrono::steady_clock::now();
            }

            // Calculate similarity if master call is loaded
            {
                std::shared_lock lock(master_calls_mutex_);
                if (!master_call_features_.empty()) {
                    auto &master_features = master_call_features_.begin()->second;
                    std::shared_lock session_lock(session_features_mutex_);
                    auto &session = session_features_[session_id];
                    float dtw_distance =
                        dtw_comparator_->compare(session.mfcc_frames, master_features);
                    similarity_score = 1.0f / (1.0f + dtw_distance);
                }
            }

            ++frames_processed;
            total_frames_processed_.fetch_add(1, std::memory_order_relaxed);
        }

        return ProcessingResult{.similarity_score = similarity_score,
                                .timestamp = std::chrono::steady_clock::now(),
                                .frames_processed = frames_processed};
    }

    // Add method to calculate similarity
    [[nodiscard]] float calculateSimilarity(int session_id) {
        std::shared_lock session_lock(sessions_mutex_);
        std::shared_lock features_lock(session_features_mutex_);
        std::shared_lock master_lock(master_calls_mutex_);

        auto session_it = session_features_.find(session_id);
        if (session_it == session_features_.end() || session_it->second.mfcc_frames.empty()) {
            return 0.0f;
            // Compare against all loaded master calls and return the best similarity score
            if (master_call_features_.empty()) {
                return 0.0f;
            }

            auto &session_features = session_it->second.mfcc_frames;
            float best_similarity = 0.0f;
            auto &master_features = master_call_features_.begin()->second;
            auto &session_features = session_it->second.mfcc_frames;

            // Use DTW to compare
            float dtw_distance = dtw_comparator_->compare(session_features, master_features);

            // Convert distance to similarity score (0-1)
            // Lower distance = higher similarity
            return 1.0f / (1.0f + dtw_distance);
            return best_similarity;
            // Lower distance = higher similarity
            return 1.0f / (1.0f + dtw_distance);
        }

#ifndef __EMSCRIPTEN__
        /**
         * @brief Start background processing thread (native platforms only)
         */
        void startBackgroundProcessing() {
            processing_thread_ = std::jthread([this](std::stop_token st) {
                while (!st.stop_requested()) {
                    processActiveSessions();
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            });
        }

        /**
         * @brief Process all active sessions in background
         */
        void processActiveSessions() {
            std::shared_lock lock(sessions_mutex_);
            for (auto &[id, session] : sessions_) {
                if (session && session->is_active) {
                    // TODO: Process accumulated audio for this session.
                    // This should include extracting features from the session's audio,
                    // updating session state, and possibly comparing against master calls.
                    // The exact processing logic depends on application requirements.
                }
            }
        }
#endif
    };

    // Public interface implementation

    HuntmasterEngine::HuntmasterEngine(const PlatformEngineConfig &config)
        : pimpl_(std::make_unique<Impl>(config)) {}

    HuntmasterEngine::~HuntmasterEngine() = default;

    HuntmasterEngine::HuntmasterEngine(HuntmasterEngine &&) noexcept = default;

    HuntmasterEngine &HuntmasterEngine::operator=(HuntmasterEngine &&) noexcept = default;

    std::expected<ProcessingResult, EngineError> HuntmasterEngine::processChunk(
        std::span<const float> audio_data) {
        return pimpl_->processAudioChunk(audio_data);
    }

    std::expected<void, EngineError> HuntmasterEngine::loadMasterCall(std::string_view call_name) {
        // Loads pre-computed MFCC features for the reference call identified by 'call_name'.
        // Expected input: call_name - the identifier or filename of the master call to load.
        // Expected output: On success, the master call features are loaded into the engine for
        // similarity comparison. Returns: std::expected<void, EngineError> indicating success or
        // failure.
        // TODO: Implement master call loading logic here.
        return {};
        std::expected<void, EngineError> HuntmasterEngine::startSession(int session_id) {
            std::unique_lock lock(pimpl_->sessions_mutex_);

            auto [it, inserted] =
                pimpl_->sessions_.try_emplace(session_id, std::make_unique<RealtimeSession>());
            if (!inserted) {
                return std::unexpected(EngineError{.status = EngineStatus::ERROR_INVALID_INPUT,
                                                   .message = "Session already exists"});
            }

            it->second->id = session_id;
            it->second->start_time = std::chrono::steady_clock::now();

            return {};
        }
        return {};
    }

    std::expected<void, EngineError> HuntmasterEngine::endSession(int session_id) {
        std::unique_lock lock(pimpl_->sessions_mutex_);

        auto it = pimpl_->sessions_.find(session_id);
        if (it == pimpl_->sessions_.end()) {
            return std::unexpected(EngineError{.status = EngineStatus::ERROR_INVALID_INPUT,
                                               .message = "Session not found"});
        }

        pimpl_->sessions_.erase(it);

        pimpl_->sessions_.erase(it);

        // Clean up session features to avoid stale data
        [[nodiscard]] bool HuntmasterEngine::isInitialized() const noexcept {
            return pimpl_->initialized_.load(std::memory_order_acquire);
        }
    }

    return {};
} return pimpl_->initialized_.load(std::memory_order_acquire);
}

size_t HuntmasterEngine::getActiveSessionCount() const noexcept {
    std::shared_lock lock(pimpl_->sessions_mutex_);
    return pimpl_->sessions_.size();
}

}  // namespace huntmaster