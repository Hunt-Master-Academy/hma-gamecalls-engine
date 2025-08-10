// Huntmaster Engine - PitchTracker YIN Implementation (single clean version)

#include "huntmaster/core/PitchTracker.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <optional>
#include <span>
#include <sstream>
#include <vector>

#include "huntmaster/core/DebugLogger.h"

namespace huntmaster {

class PitchTrackerImpl final : public PitchTracker {
  public:
    using Config = PitchTracker::Config;
    using Error = PitchTracker::Error;
    using PitchResult = PitchTracker::PitchResult;
    template <typename T>
    using ResultT = PitchTracker::Result<T, Error>;

    explicit PitchTrackerImpl(const Config& cfg) : config_(cfg) {
        initialize();
    }
    ~PitchTrackerImpl() override = default;

    ResultT<PitchResult> detectPitch(std::span<const float> audio) override {
        if (audio.empty())
            return ResultT<PitchResult>(unexpected<Error>(Error::INVALID_AUDIO_DATA));
        if (audio.size() < config_.windowSize)
            return ResultT<PitchResult>(unexpected<Error>(Error::INSUFFICIENT_DATA));
        auto pc = yin(audio);
        PitchResult r;
        if (pc.has_value()) {
            r.frequency = pc->first;
            r.confidence = pc->second;
            if (config_.enableSmoothing && hasPitch_)
                r.frequency = smooth(r.frequency);
            r.isVoiced = r.confidence > config_.threshold;
            r.timestamp = static_cast<float>(processedSamples_) / config_.sampleRate;
            updateHistory(r.frequency, r.confidence);
            r.statistics = computeStats();
            if (config_.enableVibratoDetection)
                r.vibrato = computeVibrato();
            r.contour = pitchHistory_;
            currentPitch_ = r.frequency;
            currentConfidence_ = r.confidence;
            hasPitch_ = r.isVoiced;
        }
        return ResultT<PitchResult>(std::move(r));
    }

    ResultT<float> getRealtimePitch() override {
        if (!initialized_)
            return ResultT<float>(unexpected<Error>(Error::INITIALIZATION_FAILED));
        return ResultT<float>(currentPitch_);
    }
    ResultT<float> getRealtimeConfidence() override {
        if (!initialized_)
            return ResultT<float>(unexpected<Error>(Error::INITIALIZATION_FAILED));
        return ResultT<float>(currentConfidence_);
    }

    ResultT<void> processAudioChunk(std::span<const float> audio) override {
        if (audio.empty())
            return ResultT<void>(unexpected<Error>(Error::INVALID_AUDIO_DATA));
        audioBuffer_.insert(audioBuffer_.end(), audio.begin(), audio.end());
        processedSamples_ += audio.size();
        while (audioBuffer_.size() >= config_.windowSize) {
            std::span<const float> window(audioBuffer_.data(), config_.windowSize);
            auto pc = yin(window);
            if (pc.has_value()) {
                float f = pc->first;
                float c = pc->second;
                if (config_.enableSmoothing && hasPitch_)
                    f = smooth(f);
                updateHistory(f, c);
                currentPitch_ = f;
                currentConfidence_ = c;
                hasPitch_ = c > config_.threshold;
            }
            size_t hop = std::min(config_.hopSize, config_.windowSize);
            if (audioBuffer_.size() > hop)
                audioBuffer_.erase(audioBuffer_.begin(), audioBuffer_.begin() + hop);
            else
                audioBuffer_.clear();
        }
        return ResultT<void>();
    }

    ResultT<std::vector<float>> getPitchContour(float /*durationMs*/) override {
        return ResultT<std::vector<float>>(pitchHistory_);
    }

    void reset() override {
        audioBuffer_.clear();
        pitchHistory_.clear();
        confidenceHistory_.clear();
        currentPitch_ = 0.0f;
        currentConfidence_ = 0.0f;
        smoothPitch_ = 0.0f;
        processedSamples_ = 0;
        hasPitch_ = false;
    }

    ResultT<void> updateConfig(const Config& cfg) override {
        if (cfg.sampleRate <= 0.0f || cfg.windowSize == 0 || cfg.hopSize == 0)
            return ResultT<void>(unexpected<Error>(Error::INVALID_WINDOW_SIZE));
        config_ = cfg;
        initialize();
        return ResultT<void>();
    }

    const Config& getConfig() const override {
        return config_;
    }
    bool isActive() const override {
        return hasPitch_;
    }
    std::string getProcessingStats() const override {
        return "PitchTracker(YIN) active";
    }

  private:
    Config config_{};
    bool initialized_ = false;
    bool hasPitch_ = false;
    float currentPitch_ = 0.0f;
    float currentConfidence_ = 0.0f;
    float smoothPitch_ = 0.0f;
    size_t processedSamples_ = 0;
    std::vector<float> audioBuffer_;
    std::vector<float> pitchHistory_;
    std::vector<float> confidenceHistory_;

    void initialize() {
        audioBuffer_.reserve(config_.windowSize * 2);
        size_t hist =
            static_cast<size_t>(10.0f * config_.sampleRate / std::max<size_t>(1, config_.hopSize));
        pitchHistory_.reserve(hist);
        confidenceHistory_.reserve(hist);
        initialized_ = true;
    }

    std::optional<std::pair<float, float>> yin(std::span<const float> audio) const {
        const size_t N = config_.windowSize;
        if (audio.size() < N)
            return std::nullopt;
        size_t minTau = static_cast<size_t>(config_.sampleRate / config_.maxFrequency);
        if (minTau < 2)
            minTau = 2;
        size_t maxTau =
            static_cast<size_t>(config_.sampleRate / std::max(config_.minFrequency, 1.0f));
        if (maxTau >= N / 2)
            maxTau = N / 2 - 1;
        if (minTau >= maxTau)
            return std::nullopt;

        std::vector<float> diff(maxTau + 1, 0.0f);
        for (size_t tau = 1; tau <= maxTau; ++tau) {
            double s = 0.0;
            for (size_t i = 0; i + tau < N; ++i) {
                float d = audio[i] - audio[i + tau];
                s += static_cast<double>(d) * d;
            }
            diff[tau] = static_cast<float>(s);
        }

        std::vector<float> cmnd(maxTau + 1, 0.0f);
        cmnd[0] = 1.0f;
        double run = 0.0;
        for (size_t tau = 1; tau <= maxTau; ++tau) {
            run += diff[tau];
            cmnd[tau] =
                run > 0.0 ? diff[tau] * static_cast<float>(tau) / static_cast<float>(run) : 1.0f;
        }

        size_t tauEstimate = 0;
        for (size_t tau = minTau; tau <= maxTau; ++tau) {
            if (cmnd[tau] < config_.threshold) {
                while (tau + 1 <= maxTau && cmnd[tau + 1] < cmnd[tau])
                    ++tau;
                tauEstimate = tau;
                break;
            }
        }
        if (!tauEstimate)
            return std::nullopt;

        float confidence = 1.0f - cmnd[tauEstimate];
        float pitch = static_cast<float>(config_.sampleRate / static_cast<float>(tauEstimate));
        if (pitch < config_.minFrequency || pitch > config_.maxFrequency)
            return std::nullopt;
        return std::make_pair(pitch, confidence);
    }

    float smooth(float p) {
        float a = std::clamp(config_.smoothingFactor, 0.0f, 1.0f);
        if (!hasPitch_) {
            smoothPitch_ = p;
            return p;
        }
        smoothPitch_ = (1.0f - a) * smoothPitch_ + a * p;
        return smoothPitch_;
    }

    void updateHistory(float pitch, float conf) {
        pitchHistory_.push_back(pitch);
        confidenceHistory_.push_back(conf);
        size_t maxEntries =
            static_cast<size_t>(10.0f * config_.sampleRate / std::max<size_t>(1, config_.hopSize));
        if (pitchHistory_.size() > maxEntries) {
            pitchHistory_.erase(pitchHistory_.begin());
            confidenceHistory_.erase(confidenceHistory_.begin());
        }
    }

    PitchResult::PitchStatistics computeStats() const {
        PitchResult::PitchStatistics st{};
        if (pitchHistory_.empty())
            return st;
        auto [minIt, maxIt] = std::minmax_element(pitchHistory_.begin(), pitchHistory_.end());
        double sum = std::accumulate(pitchHistory_.begin(), pitchHistory_.end(), 0.0);
        st.mean = static_cast<float>(sum / pitchHistory_.size());
        double var = 0.0;
        for (float v : pitchHistory_)
            var += (v - st.mean) * (v - st.mean);
        var /= pitchHistory_.size();
        st.standardDeviation = static_cast<float>(std::sqrt(var));
        st.range = *maxIt - *minIt;
        st.stability = (st.mean > 0.0f) ? 1.0f / (1.0f + (st.standardDeviation / st.mean)) : 0.0f;
        return st;
    }

    PitchResult::Vibrato computeVibrato() const {
        PitchResult::Vibrato v{};
        // Require a minimum number of pitch estimates to attempt vibrato analysis
        if (pitchHistory_.size() < 12 || config_.hopSize == 0 || config_.sampleRate <= 0.0f) {
            return v;  // defaults zero
        }

        // Use at most the last few seconds of history to avoid stale influence
        size_t maxSamples = static_cast<size_t>(
            std::max(1.0f, 2.0f * config_.sampleRate / std::max<size_t>(1, config_.hopSize)));
        size_t startIndex =
            pitchHistory_.size() > maxSamples ? pitchHistory_.size() - maxSamples : 0;
        std::span<const float> recent(&pitchHistory_[startIndex],
                                      pitchHistory_.size() - startIndex);

        // Compute mean and variance (extent as std dev around mean)
        double mean = std::accumulate(recent.begin(), recent.end(), 0.0) / recent.size();
        double var = 0.0;
        for (float p : recent)
            var += (p - mean) * (p - mean);
        var /= recent.size();
        v.extent = static_cast<float>(std::sqrt(var));

        // Centered series for zero crossing / cycle detection
        std::vector<float> centered;
        centered.reserve(recent.size());
        for (float p : recent)
            centered.push_back(p - static_cast<float>(mean));

        // Simple amplitude gate: require overall extent above small threshold
        if (v.extent < 0.1f) {
            return v;  // negligible modulation
        }

        // Detect zero crossings for vibrato rate estimation
        std::vector<size_t> zeroCrossIdx;
        zeroCrossIdx.reserve(centered.size() / 2);
        for (size_t i = 1; i < centered.size(); ++i) {
            if ((centered[i - 1] <= 0.0f && centered[i] > 0.0f)
                || (centered[i - 1] >= 0.0f && centered[i] < 0.0f)) {
                zeroCrossIdx.push_back(i);
            }
        }
        if (zeroCrossIdx.size() < 4) {  // not enough cycles
            return v;
        }

        // Vibrato rate: zero crossings / (2 * durationSeconds)
        double hopSeconds =
            static_cast<double>(config_.hopSize) / static_cast<double>(config_.sampleRate);
        double durationSeconds = centered.size() * hopSeconds;
        if (durationSeconds > 0.0) {
            v.rate = static_cast<float>((zeroCrossIdx.size() / 2.0) / durationSeconds);
        }

        // Regularity: based on std dev of cycle lengths (zero-cross interval pairs)
        std::vector<double> cycleLengths;
        for (size_t i = 2; i < zeroCrossIdx.size(); i += 2) {
            size_t prev = zeroCrossIdx[i - 2];
            size_t cur = zeroCrossIdx[i];
            cycleLengths.push_back((cur - prev) * hopSeconds);
        }
        if (cycleLengths.size() >= 2) {
            double cMean = std::accumulate(cycleLengths.begin(), cycleLengths.end(), 0.0)
                           / cycleLengths.size();
            double cVar = 0.0;
            for (double c : cycleLengths)
                cVar += (c - cMean) * (c - cMean);
            cVar /= cycleLengths.size();
            double cStd = std::sqrt(cVar);
            if (cMean > 0.0) {
                double cv = cStd / cMean;  // coefficient of variation
                v.regularity =
                    static_cast<float>(1.0 / (1.0 + cv));  // maps 0 (irregular) .. 1 (regular)
            }
        }
        return v;
    }
};

PitchTracker::Result<std::unique_ptr<PitchTracker>, PitchTracker::Error>
PitchTracker::create(const Config& config) {
    if (config.sampleRate <= 0.0f || config.windowSize == 0 || config.hopSize == 0)
        return PitchTracker::Result<std::unique_ptr<PitchTracker>, PitchTracker::Error>(
            unexpected<Error>(Error::INVALID_WINDOW_SIZE));
    try {
        auto ptr = std::unique_ptr<PitchTracker>(new PitchTrackerImpl(config));
        return PitchTracker::Result<std::unique_ptr<PitchTracker>, Error>(std::move(ptr));
    } catch (...) {
        return PitchTracker::Result<std::unique_ptr<PitchTracker>, Error>(
            unexpected<Error>(Error::INITIALIZATION_FAILED));
    }
}

std::string PitchTracker::exportToJson(const PitchResult& result) {
    std::ostringstream oss;
    oss << '{' << "\"frequency\":" << result.frequency << ",\"confidence\":" << result.confidence
        << ",\"isVoiced\":" << (result.isVoiced ? 1 : 0) << ",\"timestamp\":" << result.timestamp
        << ",\"statistics\":{\"mean\":" << result.statistics.mean
        << ",\"standardDeviation\":" << result.statistics.standardDeviation
        << ",\"range\":" << result.statistics.range
        << ",\"stability\":" << result.statistics.stability << '}' << ",\"contour\":[";
    for (size_t i = 0; i < result.contour.size(); ++i) {
        if (i)
            oss << ',';
        oss << result.contour[i];
    }
    oss << "]}";
    return oss.str();
}

}  // namespace huntmaster
