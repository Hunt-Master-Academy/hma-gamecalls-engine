// File: DTWComparator.h
#pragma once

#include <memory>
#include <optional>
#include <span>
#include <vector>

#include "Expected.h"

namespace huntmaster {

class DTWComparator {
  public:
    struct Config {
        float window_ratio{0.1f};  // Sakoe-Chiba band width
        bool use_window{true};
        float distance_weight{1.0f};
        bool normalize_distance{true};
        bool enable_simd{true};
    };

    explicit DTWComparator(const Config& config);
    ~DTWComparator();

    DTWComparator(DTWComparator&&) noexcept;
    DTWComparator& operator=(DTWComparator&&) noexcept;

    [[nodiscard]] float compare(const std::vector<std::vector<float>>& sequence1,
                                const std::vector<std::vector<float>>& sequence2);

    [[nodiscard]] float compareWithPath(const std::vector<std::vector<float>>& sequence1,
                                        const std::vector<std::vector<float>>& sequence2,
                                        std::vector<std::pair<size_t, size_t>>& alignment_path);

    void setWindowRatio(float ratio);

  private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

}  // namespace huntmaster