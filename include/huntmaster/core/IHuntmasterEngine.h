//File: IHuntmasterEngine.h
#pragma once

#include "Platform.h"
#include "Expected.h"


#include <span>
#include <string_view>
#include <memory>
#include <expected>
#include <chrono>
#include <concepts>

namespace huntmaster
{

    /**
     * @enum EngineStatus
     * @brief Status codes returned by engine operations
     */
    enum class EngineStatus
    {
        OK,
        ERROR_INVALID_INPUT,
        ERROR_NOT_INITIALIZED,
        ERROR_RESOURCE_UNAVAILABLE,
        ERROR_BUFFER_OVERFLOW,
        ERROR_PROCESSING_FAILED
    };

    /**
     * @struct EngineError
     * @brief Detailed error information for engine operations
     */
    struct EngineError
    {
        EngineStatus status;
        std::string message;
    };

    /**
     * @struct ProcessingResult
     * @brief Results from audio processing operations
     */
    struct ProcessingResult
    {
        float similarity_score{0.0f};
        std::chrono::steady_clock::time_point timestamp;
        size_t frames_processed{0};
    };

    /**
     * @concept AudioEngine
#else
#define AudioEngine typename
#endif
     * @brief C++20 concept defining requirements for audio engine implementations
     *
     * This concept ensures that any audio engine implementation provides the
     * required interface for processing audio chunks and managing master calls.
     */
    template <typename T>
    concept AudioEngine = requires(T engine, std::span<const float> audio_data,
                                   std::string_view call_name,
                                   int session_id) {
        // Engine must be constructible
        { T{} } -> std::same_as<T>;

        // Core processing methods
        { engine.processChunk(audio_data) } -> std::same_as<huntmaster::expected<ProcessingResult, EngineError>>;
        { engine.loadMasterCall(call_name) } -> std::same_as<huntmaster::expected<void, EngineError>>;
        { engine.startSession(session_id) } -> std::same_as<huntmaster::expected<void, EngineError>>;
        { engine.endSession(session_id) } -> std::same_as<huntmaster::expected<void, EngineError>>;

        // State queries
        { engine.isInitialized() } -> std::same_as<bool>;
        { engine.getActiveSessionCount() } -> std::same_as<size_t>;
    };

    /**
     * @struct EngineConfig
     * @brief Configuration parameters for engine initialization
     */
    struct EngineConfig
    {
        size_t sample_rate{44100};
        size_t frame_size{512};
        size_t hop_size{256};
        size_t mfcc_coefficients{13};
        float vad_energy_threshold{0.01f};
        std::chrono::milliseconds vad_window_duration{20};
        size_t max_concurrent_sessions{10};
        size_t buffer_pool_size{32};
    };

// Platform-specific configuration selection
#ifdef __EMSCRIPTEN__
    using PlatformEngineConfig = EngineConfig; // WASM uses base config
#else
    /**
     * @struct NativeEngineConfig
     * @brief Extended configuration for native platforms
     */
    struct NativeEngineConfig : public EngineConfig
    {
        bool enable_simd{true};
        size_t thread_pool_size{4};
        bool enable_gpu_acceleration{false};
    };
    using PlatformEngineConfig = NativeEngineConfig;
#endif

} // namespace huntmaster