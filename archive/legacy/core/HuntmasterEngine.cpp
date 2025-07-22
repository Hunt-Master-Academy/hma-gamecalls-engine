// File: HuntmasterEngine.cpp
#include "huntmaster/core/HuntmasterEngine.h"

#include <algorithm>
#include <thread>
#include <vector>

// Include all the component headers
#include "huntmaster/core/AudioBufferPool.h"
#include "huntmaster/core/DTWComparator.h"
#include "huntmaster/core/MFCCProcessor.h"
#include "huntmaster/core/RealTimeAudioProcessor.h"
#include "huntmaster/core/VoiceActivityDetector.h"
#include "dr_wav.h"  // For loading master calls

// These headers are not used in the final implementation but were in the original.
// They are more suited for advanced threading models that can be added later.
#include <barrier>
#include <latch>
#include <ranges>

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
    // REVIEW: The 'accumulated_audio' vector has been replaced.
    // Instead of storing all raw audio, which can consume a lot of memory,
    // we will now store the processed MFCC features directly.
    // std::vector<float> accumulated_audio;
    std::vector<std::vector<float>> features;  // Store MFCC features for this session
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
    mutable std::shared_mutex master_call_mutex_;
    // REVIEW: The original stored features in a map of strings to features.
    // For simplicity in the core logic, we will store only one master call at a time.
    // A more complex system could use the map to cache multiple loaded calls.
    // std::unordered_map<std::string, std::vector<std::vector<float>>> master_call_features_;
    std::vector<std::vector<float>> master_call_features_;

    // State tracking
    std::atomic<bool> initialized_{false};
    std::atomic<size_t> total_frames_processed_{0};

/*
REVIEW: This separate map for session features is redundant.
It's much cleaner and safer to store the features directly inside the
RealtimeSession struct, which keeps all session-related data together.
This avoids the need for a second map and a second mutex.
This section has been commented out.
*/
#if 0
    // Add member for storing current session features
    struct SessionFeatures {
        std::vector<std::vector<float>> mfcc_frames;
        std::chrono::steady_clock::time_point last_update;
    };
    mutable std::shared_mutex session_features_mutex_;
    std::unordered_map<int, SessionFeatures> session_features_;
#endif

#ifndef __EMSCRIPTEN__
    // REVIEW: This is a placeholder for an advanced multi-threaded processing model.
    // It's excellent for a future version, but for now, we will focus on a correct
    // single-threaded implementation. This is kept but unused in the current logic.
    std::jthread processing_thread_;
    std::stop_token stop_token_;
#endif

    /**
     * @brief Initialize all engine components
     */
    explicit Impl(const PlatformEngineConfig& config) : config_(config) {
        // Initialize buffer pool
        buffer_pool_ = std::make_unique<AudioBufferPool>(config_.buffer_pool_size,
                                                         config_.frame_size * sizeof(float));

        // Initialize VAD
        VoiceActivityDetector::Config vad_config{.energy_threshold = config_.vad_energy_threshold,
                                                 .window_duration = config_.vad_window_duration,
                                                 .sample_rate = config_.sample_rate};
        vad_ = std::make_unique<VoiceActivityDetector>(vad_config);

        // Initialize MFCC processor
        MFCCProcessor::Config mfcc_config{.sample_rate = config_.sample_rate,
                                          .frame_size = config_.frame_size,
                                          .num_coefficients = config_.mfcc_coefficients,
                                          .num_filters = 26};  // A common default
        mfcc_processor_ = std::make_unique<MFCCProcessor>(mfcc_config);

        // Initialize DTW comparator, explicitly providing a default config
        dtw_comparator_ = std::make_unique<DTWComparator>(DTWComparator::Config{});

        // Initialize real-time processor
        RealtimeAudioProcessor::Config rt_config{
            .ring_buffer_size = 1024,        // A reasonable default
            .chunk_size = config_.hop_size,  // Process audio in hops
            .enable_backpressure = false,
            .enable_metrics = true};
        realtime_processor_ = std::make_unique<RealtimeAudioProcessor>(rt_config);

#ifndef __EMSCRIPTEN__
        // Start background processing thread for native platforms
        if (config_.thread_pool_size > 0) {
            startBackgroundProcessing();
        }
#endif

        initialized_.store(true, std::memory_order_release);
    }

    /**
     * @brief Cleanup and stop all processing
     */
    ~Impl() {
        initialized_.store(false, std::memory_order_release);
#ifndef __EMSCRIPTEN__
        if (processing_thread_.joinable()) {
            processing_thread_.request_stop();
        }
#endif
        std::unique_lock lock(sessions_mutex_);
        sessions_.clear();
    }

    // --- NEW, CORRECTED IMPLEMENTATION ---
    huntmaster::expected<void, EngineError> loadMasterCall(std::string_view call_name) {
        // This is a simplified loader. A real implementation would search
        // a known directory for the file.
        std::string filePath = "../data/master_calls/" + std::string(call_name) + ".wav";

        unsigned int channels, sampleRate;
        drwav_uint64 totalFrames;
        float* pSampleData = drwav_open_file_and_read_pcm_frames_f32(
            filePath.c_str(), &channels, &sampleRate, &totalFrames, nullptr);

        if (!pSampleData) {
            return huntmaster::unexpected(EngineError{EngineStatus::ERROR_RESOURCE_UNAVAILABLE,
                                               "Master call file not found: " + filePath});
        }

        std::vector<float> audioData(pSampleData, pSampleData + totalFrames);
        drwav_free(pSampleData, nullptr);

        // Process the entire file to get its features
        auto features_result =
            mfcc_processor_->extractFeaturesFromBuffer(audioData, config_.hop_size);
        if (!features_result) {
            return huntmaster::unexpected(EngineError{EngineStatus::ERROR_PROCESSING_FAILED,
                                               "Failed to extract features from master call"});
        }

        // Store the features
        std::unique_lock lock(master_call_mutex_);
        master_call_features_ = std::move(*features_result);

        return {};
    }

    // --- NEW, CORRECTED IMPLEMENTATION ---
    huntmaster::expected<ProcessingResult, EngineError> processAudioChunk(
        std::span<const float> audio_data) {
        if (!initialized_.load(std::memory_order_acquire)) {
            return huntmaster::unexpected(
                EngineError{EngineStatus::ERROR_NOT_INITIALIZED, "Engine not initialized"});
        }

        // For this implementation, we assume a single, default session (ID 0)
        const int current_session_id = 0;
        {
            std::shared_lock lock(sessions_mutex_);
            if (!sessions_.contains(current_session_id)) {
                return huntmaster::unexpected(
                    EngineError{EngineStatus::ERROR_INVALID_INPUT,
                                "Session 0 not started. Call startSession(0) first."});
            }
        }

        // 1. Run VAD to check for active audio
        auto vad_result = vad_->processWindow(audio_data);
        if (!vad_result) {
            return huntmaster::unexpected(
                EngineError{EngineStatus::ERROR_PROCESSING_FAILED, "VAD processing failed."});
        }
        if (!vad_result->is_active) {
            return ProcessingResult{};  // Return empty result for silence
        }

        // 2. Extract MFCCs from the active audio
        auto mfcc_result = mfcc_processor_->extractFeatures(audio_data);
        if (!mfcc_result) {
            return huntmaster::unexpected(
                EngineError{EngineStatus::ERROR_PROCESSING_FAILED, "MFCC extraction failed"});
        }

        // 3. Store features in the session and calculate the new score
        float similarity_score = 0.0f;
        {
            std::unique_lock session_lock(sessions_mutex_);
            auto& session = sessions_.at(current_session_id);
            session->features.push_back(std::move(*mfcc_result));

            std::shared_lock master_lock(master_call_mutex_);
            if (!master_call_features_.empty()) {
                float dist = dtw_comparator_->compare(master_call_features_, session->features);
                similarity_score = 1.0f / (1.0f + dist);
            }
        }

        total_frames_processed_.fetch_add(1, std::memory_order_relaxed);

        return ProcessingResult{.similarity_score = similarity_score,
                                .timestamp = std::chrono::steady_clock::now(),
                                .frames_processed = 1};
    }

/*
REVIEW: This entire method was logically flawed and incomplete.
- It was disconnected from the main `processAudioChunk` pipeline.
- It contained multiple `return` statements that made the logic invalid.
- It redeclared variables like `session_features` that were already defined elsewhere.
The correct logic for calculating similarity has been integrated directly into
the `processAudioChunk` method above for a much cleaner data flow.
*/
#if 0  // Commenting out the entire old, broken method
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
#endif

#ifndef __EMSCRIPTEN__
    /**
     * @brief Start background processing thread (native platforms only)
     * REVIEW: This is a placeholder for a future multi-threaded implementation.
     * It is not used by the current single-threaded processing model.
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
        for (auto& [id, session] : sessions_) {
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

// ============================================================================
// Public C++ API Implementation
// ============================================================================

HuntmasterEngine::HuntmasterEngine(const PlatformEngineConfig& config)
    : pimpl_(std::make_unique<Impl>(config)) {}

HuntmasterEngine::~HuntmasterEngine() = default;

HuntmasterEngine::HuntmasterEngine(HuntmasterEngine&&) noexcept = default;

HuntmasterEngine& HuntmasterEngine::operator=(HuntmasterEngine&&) noexcept = default;

/*
REVIEW: The original implementation of the public API functions was
syntactically incorrect and incomplete. For example, it had multiple return
statements and did not match the function signatures in the header file.
The block below has been commented out and replaced with the correct,
functional implementations that properly delegate to the private Impl class.
*/
#if 0  // Commenting out the entire block of old, broken implementations
huntmaster::expected<void, EngineError> HuntmasterEngine::loadMasterCall(std::string_view call_name) {
    // Loads pre-computed MFCC features for the reference call identified by 'call_name'.
    // Expected input: call_name - the identifier or filename of the master call to load.
    // Expected output: On success, the master call features are loaded into the engine for
    // similarity comparison. Returns: huntmaster::expected<void, EngineError> indicating success or
    // failure.
    // TODO: Implement master call loading logic here.
    return {};
    huntmaster::expected<void, EngineError> HuntmasterEngine::startSession(int session_id) {
        std::unique_lock lock(pimpl_->sessions_mutex_);

        auto [it, inserted] =
            pimpl_->sessions_.try_emplace(session_id, std::make_unique<RealtimeSession>());
        if (!inserted) {
            return huntmaster::unexpected(EngineError{.status = EngineStatus::ERROR_INVALID_INPUT,
                                               .message = "Session already exists"});
        }

        it->second->id = session_id;
        it->second->start_time = std::chrono::steady_clock::now();

        return {};
    }
    return {};
}

huntmaster::expected<void, EngineError> HuntmasterEngine::endSession(int session_id) {
    std::unique_lock lock(pimpl_->sessions_mutex_);

    auto it = pimpl_->sessions_.find(session_id);
    if (it == pimpl_->sessions_.end()) {
        return huntmaster::unexpected(EngineError{.status = EngineStatus::ERROR_INVALID_INPUT,
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
#endif

// --- NEW, CORRECTED PUBLIC API IMPLEMENTATIONS ---

huntmaster::expected<ProcessingResult, EngineError> HuntmasterEngine::processChunk(
    std::span<const float> audio_data) {
    return pimpl_->processAudioChunk(audio_data);
}

huntmaster::expected<void, EngineError> HuntmasterEngine::loadMasterCall(std::string_view call_name) {
    return pimpl_->loadMasterCall(call_name);
}

huntmaster::expected<void, EngineError> HuntmasterEngine::startSession(int session_id) {
    std::unique_lock lock(pimpl_->sessions_mutex_);
    if (pimpl_->sessions_.contains(session_id)) {
        return huntmaster::unexpected(
            EngineError{EngineStatus::ERROR_INVALID_INPUT, "Session ID already exists"});
    }
    // Create a new session object
    auto session = std::make_unique<RealtimeSession>();
    session->id = session_id;
    session->start_time = std::chrono::steady_clock::now();
    pimpl_->sessions_[session_id] = std::move(session);
    return {};
}

huntmaster::expected<void, EngineError> HuntmasterEngine::endSession(int session_id) {
    std::unique_lock lock(pimpl_->sessions_mutex_);
    if (pimpl_->sessions_.erase(session_id) == 0) {
        return huntmaster::unexpected(EngineError{EngineStatus::ERROR_INVALID_INPUT, "Session not found"});
    }
    return {};
}

bool HuntmasterEngine::isInitialized() const noexcept {
    return pimpl_ && pimpl_->initialized_.load(std::memory_order_acquire);
}

size_t HuntmasterEngine::getActiveSessionCount() const noexcept {
    std::shared_lock lock(pimpl_->sessions_mutex_);
    return pimpl_->sessions_.size();
}

}  // namespace huntmaster
