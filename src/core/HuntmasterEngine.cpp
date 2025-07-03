// File: HuntmasterEngine.cpp
#include "HuntmasterEngine.h"
#include "AudioBufferPool.h"
#include "VoiceActivityDetector.h"
#include "MFCCProcessor.h"
#include "DTWComparator.h"

#include <vector>
#include <algorithm>
#include <ranges>
#include <thread>
#include <latch>
#include <barrier>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

namespace huntmaster
{

    /**
     * @struct RealtimeSession
     * @brief Represents an active audio processing session
     */
    struct RealtimeSession
    {
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
    class HuntmasterEngine::Impl
    {
    public:
        // Configuration
        PlatformEngineConfig config_;

        // Core components (using smart pointers for RAII)
        std::unique_ptr<AudioBufferPool> buffer_pool_;
        std::unique_ptr<VoiceActivityDetector> vad_;
        std::unique_ptr<MFCCProcessor> mfcc_processor_;
        std::unique_ptr<DTWComparator> dtw_comparator_;

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

        /**
         * @brief Initialize all engine components
         */
        explicit Impl(const PlatformEngineConfig &config)
            : config_(config)
        {

            // Initialize buffer pool
            buffer_pool_ = std::make_unique<AudioBufferPool>(
                config.buffer_pool_size,
                config.frame_size * sizeof(float));

            // Initialize VAD
            VoiceActivityDetector::Config vad_config{
                .energy_threshold = config.vad_energy_threshold,
                .window_duration = config.vad_window_duration,
                .sample_rate = config.sample_rate};
            vad_ = std::make_unique<VoiceActivityDetector>(vad_config);

            // Initialize MFCC processor
            MFCCProcessor::Config mfcc_config{
                .sample_rate = config.sample_rate,
                .frame_size = config.frame_size,
                .num_coefficients = config.mfcc_coefficients,
                .num_filters = 26};
            mfcc_processor_ = std::make_unique<MFCCProcessor>(mfcc_config);

            // Initialize DTW comparator
            dtw_comparator_ = std::make_unique<DTWComparator>();

#ifndef __EMSCRIPTEN__
            // Start background processing thread for native platforms
            if (config.thread_pool_size > 0)
            {
                startBackgroundProcessing();
            }
#endif

            initialized_.store(true, std::memory_order_release);
        }

        /**
         * @brief Cleanup and stop all processing
         */
        ~Impl()
        {
            // Signal shutdown
            initialized_.store(false, std::memory_order_release);

#ifndef __EMSCRIPTEN__
            // Stop background thread if running
            if (processing_thread_.joinable())
            {
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
        [[nodiscard]] std::expected<ProcessingResult, EngineError>
        processAudioChunk(std::span<const float> audio_data)
        {
            if (!initialized_.load(std::memory_order_acquire))
            {
                return std::unexpected(EngineError{
                    .status = EngineStatus::ERROR_NOT_INITIALIZED,
                    .message = "Engine not initialized"});
            }

            // Validate input
            if (audio_data.empty() || audio_data.size() > config_.frame_size * 10)
            {
                return std::unexpected(EngineError{
                    .status = EngineStatus::ERROR_INVALID_INPUT,
                    .message = "Invalid audio data size"});
            }

            // Get buffer from pool
            auto buffer_result = buffer_pool_->acquire();
            if (!buffer_result)
            {
                return std::unexpected(EngineError{
                    .status = EngineStatus::ERROR_RESOURCE_UNAVAILABLE,
                    .message = "No available buffers"});
            }

            auto &buffer = buffer_result.value();

            // Copy audio data to buffer
            std::ranges::copy(audio_data, buffer.begin());

            // Apply VAD
            auto vad_result = vad_->processWindow(audio_data);
            if (!vad_result || !vad_result->is_active)
            {
                // No voice activity detected
                buffer_pool_->release(buffer);
                return ProcessingResult{
                    .similarity_score = 0.0f,
                    .timestamp = std::chrono::steady_clock::now(),
                    .frames_processed = 1};
            }

            // Extract MFCC features
            auto features = mfcc_processor_->extractFeatures(audio_data);
            if (!features)
            {
                buffer_pool_->release(buffer);
                return std::unexpected(EngineError{
                    .status = EngineStatus::ERROR_PROCESSING_FAILED,
                    .message = "MFCC extraction failed"});
            }

            // Compare with master call if loaded
            float similarity_score = 0.0f;
            {
                std::shared_lock lock(master_calls_mutex_);
                if (!master_call_features_.empty())
                {
                    auto &master_features = master_call_features_.begin()->second;
                    similarity_score = dtw_comparator_->compare(
                        features.value(),
                        master_features);
                }
            }

            // Update metrics
            total_frames_processed_.fetch_add(1, std::memory_order_relaxed);

            // Release buffer back to pool
            buffer_pool_->release(buffer);

            return ProcessingResult{
                .similarity_score = similarity_score,
                .timestamp = std::chrono::steady_clock::now(),
                .frames_processed = total_frames_processed_.load()};
        }

#ifndef __EMSCRIPTEN__
        /**
         * @brief Start background processing thread (native platforms only)
         */
        void startBackgroundProcessing()
        {
            processing_thread_ = std::jthread([this](std::stop_token st)
                                              {
            while (!st.stop_requested()) {
                processActiveSessions();
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            } });
        }

        /**
         * @brief Process all active sessions in background
         */
        void processActiveSessions()
        {
            std::shared_lock lock(sessions_mutex_);
            for (auto &[id, session] : sessions_)
            {
                if (session && session->is_active)
                {
                    // Process accumulated audio for this session
                    // Implementation depends on specific requirements
                }
            }
        }
#endif
    };

    // Public interface implementation

    HuntmasterEngine::HuntmasterEngine(const PlatformEngineConfig &config)
        : pimpl_(std::make_unique<Impl>(config))
    {
    }

    HuntmasterEngine::~HuntmasterEngine() = default;

    HuntmasterEngine::HuntmasterEngine(HuntmasterEngine &&) noexcept = default;

    HuntmasterEngine &HuntmasterEngine::operator=(HuntmasterEngine &&) noexcept = default;

    std::expected<ProcessingResult, EngineError>
    HuntmasterEngine::processChunk(std::span<const float> audio_data)
    {
        return pimpl_->processAudioChunk(audio_data);
    }

    std::expected<void, EngineError>
    HuntmasterEngine::loadMasterCall(std::string_view call_name)
    {
        // TODO: Implement master call loading
        // This would load pre-computed MFCC features for the reference call
        return {};
    }

    std::expected<void, EngineError>
    HuntmasterEngine::startSession(int session_id)
    {
        std::unique_lock lock(pimpl_->sessions_mutex_);

        if (pimpl_->sessions_.contains(session_id))
        {
            return std::unexpected(EngineError{
                .status = EngineStatus::ERROR_INVALID_INPUT,
                .message = "Session already exists"});
        }

        auto session = std::make_unique<RealtimeSession>();
        session->id = session_id;
        session->start_time = std::chrono::steady_clock::now();

        pimpl_->sessions_[session_id] = std::move(session);

        return {};
    }

    std::expected<void, EngineError>
    HuntmasterEngine::endSession(int session_id)
    {
        std::unique_lock lock(pimpl_->sessions_mutex_);

        auto it = pimpl_->sessions_.find(session_id);
        if (it == pimpl_->sessions_.end())
        {
            return std::unexpected(EngineError{
                .status = EngineStatus::ERROR_INVALID_INPUT,
                .message = "Session not found"});
        }

        pimpl_->sessions_.erase(it);

        return {};
    }

    bool HuntmasterEngine::isInitialized() const noexcept
    {
        return pimpl_->initialized_.load(std::memory_order_acquire);
    }

    size_t HuntmasterEngine::getActiveSessionCount() const noexcept
    {
        std::shared_lock lock(pimpl_->sessions_mutex_);
        return pimpl_->sessions_.size();
    }

} // namespace huntmaster