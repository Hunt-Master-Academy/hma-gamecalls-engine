// File: VoiceActivityDetector.cpp
#include "huntmaster/core/VoiceActivityDetector.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <deque>
#include <numeric>
#include <stdexcept>

#include "huntmaster/core/DebugLogger.h"

// Enable debug output for VoiceActivityDetector
#define DEBUG_VAD 0

namespace huntmaster {

// Internal state machine for VAD logic
enum class VADState { SILENCE, VOICE_CANDIDATE, VOICE_ACTIVE, HANGOVER };

class VoiceActivityDetector::Impl {
   public:
    Config config_;
    VADState state_ = VADState::SILENCE;

    // History for adaptive thresholding
    std::deque<float> energy_history_;
    float adaptive_threshold_;

    // Timers for state transitions
    std::chrono::steady_clock::time_point state_change_time_;
    std::chrono::steady_clock::time_point voice_start_time_;
    std::chrono::steady_clock::time_point current_time_;

    // Frame counting for more reliable candidate duration tracking
    int frames_in_candidate_state_ = 0;

    explicit Impl(const Config& config)
        : config_(config), adaptive_threshold_(config.energy_threshold) {
        const auto now = std::chrono::steady_clock::now();
        state_change_time_ = now;
        current_time_ = now;
    }

    float computeEnergy(std::span<const float> audio) {
        if (audio.empty()) return 0.0f;
        double sum_sq = 0.0;
        for (float sample : audio) {
            sum_sq += static_cast<double>(sample) * sample;
        }
        return static_cast<float>(sum_sq / audio.size());
    }

    void updateAdaptiveThreshold(float current_energy) {
        // For testing and initial implementation, use the configured threshold
        // In production, this could be enhanced with adaptive behavior
        adaptive_threshold_ = config_.energy_threshold;
    }

    VADResult process(std::span<const float> audio) {
        using namespace std::chrono;

        // Advance internal time by the duration of the audio window
        current_time_ += config_.window_duration;
        const auto now = current_time_;

        const float energy = computeEnergy(audio);
        updateAdaptiveThreshold(energy);

        bool is_currently_active = energy > adaptive_threshold_;
        VADResult result{.energy_level = energy};

        switch (state_) {
            case VADState::SILENCE:
                if (is_currently_active) {
                    state_ = VADState::VOICE_CANDIDATE;
                    frames_in_candidate_state_ = 1;
                    voice_start_time_ = now - config_.window_duration;
                }
                break;

            case VADState::VOICE_CANDIDATE:
                if (is_currently_active) {
                    frames_in_candidate_state_++;
                    // Calculate total duration of consecutive voice frames
                    const auto candidate_duration =
                        frames_in_candidate_state_ * config_.window_duration;
                    if (candidate_duration >= config_.min_sound_duration) {
                        state_ = VADState::VOICE_ACTIVE;
                    }
                } else {
                    state_ = VADState::SILENCE;
                    frames_in_candidate_state_ = 0;
                }
                break;

            case VADState::VOICE_ACTIVE:
                if (!is_currently_active) {
                    state_ = VADState::HANGOVER;
                    state_change_time_ = now;
                }
                break;

            case VADState::HANGOVER:
                if (is_currently_active) {
                    state_ = VADState::VOICE_ACTIVE;
                } else {
                    if (now - state_change_time_ >= config_.post_buffer) {
                        state_ = VADState::SILENCE;
                    }
                }
                break;
        }

        result.is_active = (state_ == VADState::VOICE_ACTIVE || state_ == VADState::HANGOVER);

        if (result.is_active) {
            result.duration = duration_cast<milliseconds>(now - voice_start_time_);
        }

        return result;
    }
};

// Public Interface Implementation
VoiceActivityDetector::VoiceActivityDetector(const Config& config)
    : pimpl_(std::make_unique<Impl>(config)) {}
VoiceActivityDetector::~VoiceActivityDetector() = default;
VoiceActivityDetector::VoiceActivityDetector(VoiceActivityDetector&&) noexcept = default;
VoiceActivityDetector& VoiceActivityDetector::operator=(VoiceActivityDetector&&) noexcept = default;

huntmaster::expected<VADResult, VADError> VoiceActivityDetector::processWindow(
    std::span<const float> audio) {
    if (audio.empty()) {
        return huntmaster::unexpected(VADError::INVALID_INPUT);
    }

    if (!pimpl_) {
        return huntmaster::unexpected(VADError::NOT_INITIALIZED);
    }

    auto result = pimpl_->process(audio);
    return result;
}

void VoiceActivityDetector::reset() {
    pimpl_->state_ = VADState::SILENCE;
    pimpl_->energy_history_.clear();
    pimpl_->adaptive_threshold_ = pimpl_->config_.energy_threshold;
    pimpl_->current_time_ = std::chrono::steady_clock::now();
    pimpl_->frames_in_candidate_state_ = 0;
}

bool VoiceActivityDetector::isVoiceActive() const noexcept {
    bool basicActive =
        (pimpl_->state_ == VADState::VOICE_ACTIVE || pimpl_->state_ == VADState::HANGOVER);
    return basicActive;
}

std::chrono::milliseconds VoiceActivityDetector::getActiveDuration() const noexcept {
    using namespace std::chrono;
    if (isVoiceActive()) {
        return duration_cast<milliseconds>(pimpl_->current_time_ - pimpl_->voice_start_time_);
    }
    return milliseconds{0};
}

}  // namespace huntmaster