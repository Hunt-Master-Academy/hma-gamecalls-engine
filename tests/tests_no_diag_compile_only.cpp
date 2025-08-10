#include "huntmaster/core/UnifiedAudioEngine.h"
using namespace huntmaster;
int main() {
    auto eng = UnifiedAudioEngine::create();
    if (!eng.isOk())
        return 0;
    auto engine = std::move(eng.value);
    auto s = engine->createSession(44100.0f);
    if (!s.isOk())
        return 0;
    auto snap = engine->getSimilarityScores(s.value);
    (void)snap;
    auto ds = engine->destroySession(s.value);
    (void)ds;  // explicitly ignore result (nodiscard handled)
    return 0;
}
