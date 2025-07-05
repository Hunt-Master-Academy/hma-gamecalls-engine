// File: VoiceActivityDetector.cpp
#include "VoiceActivityDetector.h"

#include <algorithm>
#include <cmath>
#include <deque>
#include <numeric>
#include <ranges>

namespace huntmaster {

enum class VADState { SILENCE, VOICE_ONSET, VOICE_ACTIVE, VOICE_OFFSET };

class VoiceActivityDetector::Impl {
   public:
    Config config_;

    // State machine
    VADState state_{VADState::SILENCE};
    std::chrono::steady_clock::time_point voice_start_time_;
    std::chrono::steady_clock::time_point last_active_time_;

    // Buffers for pre/post buffering
    std::deque<std::vector<float>> pre_buffer_;
    std::deque<std::vector<float>> post_buffer_;
    size_t pre_buffer_frames_;
    size_t post_buffer_frames_;
    size_t pre_buffer_frames_;
    size_t post_buffer_frames_;

    // Energy tracking
    std::deque<float> energy_history_;
    size_t energy_window_size_;
    float adaptive_threshold_;

    // Frame timing
    size_t samples_per_window_;
    size_t total_frames_processed_{0};

    explicit Impl(const Config &config)
        : config_(config),
          adaptive_threshold_(config.energy_threshold) samples_per_window_ =
              (config.sample_rate *
               std::chrono::duration_cast<std::chrono::milliseconds>(config.window_duration)
                   .count()) /
              1000;
    energy_window_size_ = std::max(
        size_t(10),
        size_t(
            1000 /
            std::chrono::duration_cast<std::chrono::milliseconds>(config.window_duration).count()));

    pre_buffer_frames_ =
        std::chrono::duration_cast<std::chrono::milliseconds>(config.pre_buffer).count() /
        std::chrono::duration_cast<std::chrono::milliseconds>(config.window_duration).count();
    post_buffer_frames_ =
        std::chrono::duration_cast<std::chrono::milliseconds>(config.post_buffer).count() /
        std::chrono::duration_cast<std::chrono::milliseconds>(config.window_duration).count();

    pre_buffer_.resize(pre_buffer_frames_);
    post_buffer_.resize(post_buffer_frames_);
}

[[nodiscard]] float
computeEnergy(std::span<const float> audio) {
    return std::ranges::transform_reduce(audio, 0.0f, std::plus{},
                                         [](float sample) { return sample * sample; }) /
           audio.size();
}

void updateAdaptiveThreshold(float energy) {
    energy_history_.push_back(energy);
    if (energy_history_.size() > energy_window_size_) {
        energy_history_.pop_front();
    }

    if (energy_history_.size() >= 5) {
        auto sorted = std::vector(energy_history_.begin(), energy_history_.end());
        std::ranges::sort(sorted);

        float noise_floor = sorted[sorted.size() / 4];  // 25th percentile
        adaptive_threshold_ = std::max(config_.energy_threshold, noise_floor * 3.0f);
    }
}

[[nodiscard]] VADResult processStateMachine(float energy, std::span<const float> audio) {
    using namespace std::chrono;
    auto now = steady_clock::now();
    bool is_voice = energy > adaptive_threshold_;

    VADResult result{.energy_level = energy};

    auto transition = [this](VADState new_state) {
        state_ = new_state;
        // State machine logic based on current state_
        switch (state_) {
            case VADState::SILENCE:
                if (is_voice) {
                    transition(VADState::VOICE_ONSET);
                    voice_start_time_ = now;
                    last_active_time_ = now;
                }
                break;
            case VADState::VOICE_ONSET:
                if (is_voice) {
                    auto duration = duration_cast<milliseconds>(now - voice_start_time_);
                    if (duration >= config_.min_sound_duration) {
                        transition(VADState::VOICE_ACTIVE);
                        result.is_active = true;
                    }
                    last_active_time_ = now;
                } else {
                    transition(VADState::SILENCE);
                }
                break;
            case VADState::VOICE_ACTIVE:
                if (is_voice) {
                    last_active_time_ = now;
                    result.is_active = true;
                } else {
                    transition(VADState::VOICE_OFFSET);
                }
                break;
            case VADState::VOICE_OFFSET: {
                auto silence_duration = duration_cast<milliseconds>(now - last_active_time_);
                if (is_voice) {
                    transition(VADState::VOICE_ACTIVE);
                    last_active_time_ = now;
                    result.is_active = true;
                } else if (silence_duration >= config_.post_buffer) {
                    transition(VADState::SILENCE);
                } else {
                    result.is_active = true;  // Still in post-buffer period
                }
                break;
            }
        }
    } if (state_ == VADState::SILENCE || state_ == VADState::VOICE_ONSET) {
        pre_buffer_.push_back(std::move(frame));
        if (pre_buffer_.size() > pre_buffer_frames_) {
            pre_buffer_.pop_front();
        }
    }

    if (state_ == VADState::VOICE_OFFSET) {
        post_buffer_.push_back(std::move(frame));
        if (post_buffer_.size() > post_buffer_frames_) {
            if (state_ == VADState::SILENCE || state_ == VADState::VOICE_ONSET) {
                pre_buffer_.push_back(std::move(frame));
                if (pre_buffer_.size() > pre_buffer_frames_) {
                    pre_buffer_.pop_front();
                }
            }

            if (state_ == VADState::VOICE_OFFSET) {
                post_buffer_.push_back(std::move(frame));
                if (post_buffer_.size() > post_buffer_frames_) {
                    post_buffer_.pop_front();
                }
            }

            VoiceActivityDetector::VoiceActivityDetector(const Config &config)
                : pimpl_(std::make_unique<Impl>(config)) {}

            VoiceActivityDetector::~VoiceActivityDetector() = default;

            VoiceActivityDetector::VoiceActivityDetector(VoiceActivityDetector &&) noexcept =
                default;

            VoiceActivityDetector &VoiceActivityDetector::operator=(
                VoiceActivityDetector &&) noexcept = default;

            std::expected<VADResult, VADError> VoiceActivityDetector::processWindow(
                std::span<const float> audio) {
                if (audio.empty()) {
                    return std::unexpected(VADError::INVALID_INPUT);
                }

                float energy = pimpl_->computeEnergy(audio);
                pimpl_->updateAdaptiveThreshold(energy);

                auto result = pimpl_->processStateMachine(energy, audio);

                pimpl_->updateBuffers(audio);
                pimpl_->total_frames_processed_++;

                return result;
            }

            void VoiceActivityDetector::reset() {
                pimpl_->state_ = VADState::SILENCE;
                pimpl_->energy_history_.clear();
                pimpl_->pre_buffer_.clear();
                pimpl_->post_buffer_.clear();
                pimpl_->total_frames_processed_ = 0;
                pimpl_->adaptive_threshold_ = pimpl_->config_.energy_threshold;
            }

            bool VoiceActivityDetector::isVoiceActive() const noexcept {
                [[nodiscard]] bool VoiceActivityDetector::isVoiceActive() const noexcept {
                    return pimpl_->state_ == VADState::VOICE_ACTIVE ||
                           pimpl_->state_ == VADState::VOICE_OFFSET;
                    [[nodiscard]] std::chrono::milliseconds
                    VoiceActivityDetector::getActiveDuration() const noexcept {
                        using namespace std::chrono;
                        if (pimpl_->state_ == VADState::VOICE_ACTIVE) {
                            return duration_cast<milliseconds>(steady_clock::now() -
                                                               pimpl_->voice_start_time_);
                        }
                        return milliseconds{0};
                    }

                }  // namespace huntmaster