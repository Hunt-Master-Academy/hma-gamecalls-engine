#pragma once
#include <cmath>
#include <vector>

inline std::vector<float> makeSineBuf(float freq, float seconds, float sr) {
    size_t n = static_cast<size_t>(seconds * sr);
    std::vector<float> out(n);
    const double step = 2.0 * M_PI * freq / sr;
    for (size_t i = 0; i < n; ++i)
        out[i] = static_cast<float>(std::sin(step * i));
    return out;
}
