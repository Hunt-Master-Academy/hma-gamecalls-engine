//File: HuntmasterEngine.h
#pragma once

#include "IHuntmasterEngine.h"
#include <memory>
#include <unordered_map>
#include <shared_mutex>
#include <atomic>
#include <optional>

namespace huntmaster
{

    /**
     * @class HuntmasterEngine
     * @brief Unified audio engine implementation with platform-specific optimizations
     *
     * This class provides a unified implementation of the Huntmaster audio engine
     * that works across all platforms (native, WASM, mobile) with compile-time
     * optimizations for each platform.
     */
    class HuntmasterEngine
    {
    public:
        /**
         * @brief Constructs the engine with the specified configuration
         * @param config Platform-specific engine configuration
         */
        explicit HuntmasterEngine(const PlatformEngineConfig &config = {});

        /**
         * @brief Destructor - ensures all resources are properly released
         */
        ~HuntmasterEngine();

        // Rule of 5 - Disable copy, enable move
        HuntmasterEngine(const HuntmasterEngine &) = delete;
        HuntmasterEngine &operator=(const HuntmasterEngine &) = delete;
        HuntmasterEngine(HuntmasterEngine &&) noexcept;
        HuntmasterEngine &operator=(HuntmasterEngine &&) noexcept;

        /**
         * @brief Process a chunk of audio data
         * @param audio_data Input audio samples as a span
         * @return Processing result or error
         */
        [[nodiscard]] huntmaster::expected<ProcessingResult, EngineError>
        processChunk(std::span<const float> audio_data);

        /**
         * @brief Load a master call reference for comparison
         * @param call_name Name of the call to load
         * @return Success or error
         */
        [[nodiscard]] huntmaster::expected<void, EngineError>
        loadMasterCall(std::string_view call_name);

        /**
         * @brief Start a new processing session
         * @param session_id Unique identifier for the session
         * @return Success or error
         */
        [[nodiscard]] huntmaster::expected<void, EngineError>
        startSession(int session_id);

        /**
         * @brief End an active processing session
         * @param session_id Session to terminate
         * @return Success or error
         */
        [[nodiscard]] huntmaster::expected<void, EngineError>
        endSession(int session_id);

        /**
         * @brief Check if the engine is properly initialized
         * @return True if initialized and ready
         */
        [[nodiscard]] bool isInitialized() const noexcept;

        /**
         * @brief Get the number of active processing sessions
         * @return Active session count
         */
        [[nodiscard]] size_t getActiveSessionCount() const noexcept;

    private:
        class Impl; // Forward declaration for Pimpl
        std::unique_ptr<Impl> pimpl_;
    };

    // Verify that HuntmasterEngine satisfies the AudioEngine concept
    #ifndef HUNTMASTER_PLATFORM_WASM
        static_assert(AudioEngine<HuntmasterEngine>);
    #endif

} // namespace huntmaster