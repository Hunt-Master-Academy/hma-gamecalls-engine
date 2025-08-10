#include <chrono>
#include <iostream>
#include <vector>

#include "huntmaster/core/UnifiedAudioEngine.h"

using namespace huntmaster;

static std::vector<float> sine(float f, float sr, float dur) {
    size_t n = static_cast<size_t>(dur * sr);
    std::vector<float> b(n);
    for (size_t i = 0; i < n; ++i) {
        b[i] = std::sin(2.0 * M_PI * f * (double)i / sr);
    }
    return b;
}

int main() {
    auto engRes = UnifiedAudioEngine::create();
    if (!engRes.isOk()) {
        std::cerr << "{\"error\":\"engine_create_failed\"}\n";
        return 1;
    }
    auto engine = std::move(engRes.value);
    auto sessRes = engine->createSession(44100.0f);
    if (!sessRes.isOk()) {
        std::cerr << "{\"error\":\"session_create_failed\"}\n";
        return 1;
    }
    SessionId s = sessRes.value;

    auto start = std::chrono::high_resolution_clock::now();
    // Generate synthetic audio chunks
    auto buf = sine(440.0f, 44100.0f, 0.5f);
    engine->processAudioChunk(s, std::span<const float>(buf.data(), buf.size()));
    auto scoreRes = engine->getSimilarityScore(s);
    auto end = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(end - start).count();
    double score = scoreRes.isOk() ? scoreRes.value : -1.0;
    std::cout << "{\"processing_ms\":" << ms << ",\"similarity\":" << score << "}" << std::endl;
    engine->destroySession(s);
    return 0;
}
