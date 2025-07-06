// File: VoiceActivityDetector.cpp
#include "huntmaster/core/VoiceActivityDetector.h"

#include <algorithm>
#include <cmath>
#include <deque>
#include <numeric>

namespace huntmaster {

// Internal state machine for VAD logic
enum class VADState {
    SILENCE,
    VOICE_CANDIDATE,
    VOICE_ACTIVE,
    HANGOVER
};

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

    explicit Impl(const Config& config) : config_(config), adaptive_threshold_(config.energy_threshold) {
        state_change_time_ = std::chrono::steady_clock::now();
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
        // Only update threshold based on silence
        if (state_ == VADState::SILENCE) {
            energy_history_.push_back(current_energy);
            if (energy_history_.size() > 50) { // Keep history of last 50 silence frames
                energy_history_.pop_front();
            }

            // A simple adaptive threshold: average of recent silence energies + a margin
            double sum = std::accumulate(energy_history_.begin(), energy_history_.end(), 0.0);
            float noise_floor = static_cast<float>(sum / energy_history_.size());
            adaptive_threshold_ = std::max(config_.energy_threshold, noise_floor * 1.5f);
        }
    }

    VADResult process(std::span<const float> audio) {
        using namespace std::chrono;
        const auto now = steady_clock::now();
        const float energy = computeEnergy(audio);
        updateAdaptiveThreshold(energy);

        bool is_currently_active = energy > adaptive_threshold_;
        VADResult result{.energy_level = energy};

        switch (state_) {
            case VADState::SILENCE:
                if (is_currently_active) {
                    state_ = VADState::VOICE_CANDIDATE;
                    state_change_time_ = now;
                    voice_start_time_ = now;
                }
                break;

            case VADState::VOICE_CANDIDATE:
                if (is_currently_active) {
                    if (now - state_change_time_ >= config_.min_sound_duration) {
                        state_ = VADState::VOICE_ACTIVE;
                    }
                } else {
                    state_ = VADState::SILENCE;
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
VoiceActivityDetector::VoiceActivityDetector(const Config& config) : pimpl_(std::make_unique<Impl>(config)) {}
VoiceActivityDetector::~VoiceActivityDetector() = default;
VoiceActivityDetector::VoiceActivityDetector(VoiceActivityDetector&&) noexcept = default;
VoiceActivityDetector& VoiceActivityDetector::operator=(VoiceActivityDetector&&) noexcept = default;

std::expected<VADResult, VADError> VoiceActivityDetector::processWindow(std::span<const float> audio) {
    if (audio.empty()) {
        return std::unexpected(VADError::INVALID_INPUT);
    }
    return pimpl_->process(audio);
}

void VoiceActivityDetector::reset() {
    pimpl_->state_ = VADState::SILENCE;
    pimpl_->energy_history_.clear();
    pimpl_->adaptive_threshold_ = pimpl_->config_.energy_threshold;
}

bool VoiceActivityDetector::isVoiceActive() const noexcept {
    return pimpl_->state_ == VADState::VOICE_ACTIVE || pimpl_->state_ == VADState::HANGOVER;
}

std::chrono::milliseconds VoiceActivityDetector::getActiveDuration() const noexcept {
    using namespace std::chrono;
    if (isVoiceActive()) {
        return duration_cast<milliseconds>(steady_clock::now() - pimpl_->voice_start_time_);
    }
    return milliseconds{0};
}

} // namespace huntmaster
