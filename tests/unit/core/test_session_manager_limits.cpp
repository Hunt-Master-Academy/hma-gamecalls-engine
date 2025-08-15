// Session / engine capacity & limits tests
// Focus: ensure hard session cap (currently 1000) is enforced and that freeing a
// session allows a new one to be created without leaking or crashing.
// NOTE: Uses only create/destroy (no audio processing) to keep runtime & memory bounded.

#include <chrono>
#include <vector>

#include <gtest/gtest.h>

#include "huntmaster/core/UnifiedAudioEngine.h"

using namespace huntmaster;

namespace {
constexpr int kExpectedSessionLimit =
    1000;  // Mirrors hard-coded limit in UnifiedAudioEngine::Impl::createSession
}

TEST(SessionManagerLimits, MaxSessionsEnforcedAndRecovery) {
    // Create engine
    auto engR = UnifiedAudioEngine::create();
    ASSERT_TRUE(engR.isOk());
    auto engine = std::move(engR.value);

    std::vector<SessionId> sessions;
    sessions.reserve(kExpectedSessionLimit);

    const float kSampleRate = 44100.0f;

    // Create up to the documented limit
    for (int i = 0; i < kExpectedSessionLimit; ++i) {
        auto sR = engine->createSession(kSampleRate);
        ASSERT_TRUE(sR.isOk()) << "Creation failed early at index " << i;
        sessions.push_back(sR.value);
    }

    // Next creation should fail with OUT_OF_MEMORY (resource limit) status
    auto overLimit = engine->createSession(kSampleRate);
    ASSERT_FALSE(overLimit.isOk()) << "Expected failure creating session beyond limit";
    // We expect OUT_OF_MEMORY per implementation when size() >= limit
    // (Using ASSERT_EQ on enum value to harden behavior; adjust if implementation changes)
    EXPECT_EQ(overLimit.error(), UnifiedAudioEngine::Status::OUT_OF_MEMORY);

    // Destroy one session (LIFO for simplicity)
    ASSERT_FALSE(sessions.empty());
    SessionId toDestroy = sessions.back();
    auto destroyStatus = engine->destroySession(toDestroy);
    EXPECT_EQ(destroyStatus, UnifiedAudioEngine::Status::OK);
    sessions.pop_back();

    // Creation should now succeed again (capacity recovered)
    auto recovered = engine->createSession(kSampleRate);
    ASSERT_TRUE(recovered.isOk()) << "Expected creation to succeed after freeing capacity";

    // Cleanup: destroy remaining sessions (include newly created one)
    sessions.push_back(recovered.value);
    for (SessionId sid : sessions) {
        auto st = engine->destroySession(sid);
        EXPECT_EQ(st, UnifiedAudioEngine::Status::OK);
    }
}

TEST(SessionManagerLimits, InvalidSampleRateRejected) {
    auto engR = UnifiedAudioEngine::create();
    ASSERT_TRUE(engR.isOk());
    auto engine = std::move(engR.value);

    auto r = engine->createSession(-1.0f);
    ASSERT_FALSE(r.isOk());
    EXPECT_EQ(r.error(), UnifiedAudioEngine::Status::INVALID_PARAMS);
}
