// File: VoiceActivityDetector.h
#pragma once

#include <span>
#include <expected>
#include <chrono>
#include <memory>
#include <variant>

namespace huntmaster
{

    enum class VADError
    {
        INVALID_INPUT,
        PROCESSING_FAILED,
        NOT_INITIALIZED
    };

    struct VADResult
    {
        bool is_active{false};
        float energy_level{0.0f};
        std::chrono::milliseconds duration{0};
    };

    class VoiceActivityDetector
    {
    public:
        struct Config
        {
            float energy_threshold{0.01f};
            std::chrono::milliseconds window_duration{20};
            std::chrono::milliseconds min_sound_duration{100};
            std::chrono::milliseconds pre_buffer{50};
            std::chrono::milliseconds post_buffer{100};
            size_t sample_rate{44100};
        };

        explicit VoiceActivityDetector(const Config &config);
        ~VoiceActivityDetector();

        VoiceActivityDetector(VoiceActivityDetector &&) noexcept;
        VoiceActivityDetector &operator=(VoiceActivityDetector &&) noexcept;

        [[nodiscard]] std::expected<VADResult, VADError>
        processWindow(std::span<const float> audio);

        void reset();

        [[nodiscard]] bool isVoiceActive() const noexcept;
        [[nodiscard]] std::chrono::milliseconds getActiveDuration() const noexcept;

    private:
        class Impl;
        std::unique_ptr<Impl> pimpl_;
    };

} // namespace huntmaster