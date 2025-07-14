/**
 * @file simple_unified_test.cpp
 * @brief Simple test program to verify UnifiedAudioEngine functionality
 *
 * This is a standalone test program that can be built and run to verify
 * that the UnifiedAudioEngine is working correctly.
 */

#include <huntmaster/core/UnifiedAudioEngine.h>

#include <iostream>
#include <vector>

using huntmaster::SessionId;
using huntmaster::UnifiedAudioEngine;
using Status = huntmaster::UnifiedAudioEngine::Status;

void test_single_session_lifecycle() {
    std::cout << "\n--- Testing Single Session Lifecycle ---\n";
    auto engineResult = UnifiedAudioEngine::create();
    if (!engineResult) {
        std::cerr << "✗ Failed to create engine: " << static_cast<int>(engineResult.error())
                  << std::endl;
        return;
    }
    auto engine = std::move(*engineResult);

    // 1. Create Session
    auto sessionResult = engine->createSession();
    if (!sessionResult) {
        std::cerr << "✗ Failed to create session: " << static_cast<int>(sessionResult.error())
                  << std::endl;
        return;
    }
    SessionId sessionId = *sessionResult;
    std::cout << "✓ Session created with ID: " << sessionId << std::endl;

    // 2. Verify Session Exists
    if (!engine->isSessionActive(sessionId)) {
        std::cerr << "✗ Session " << sessionId << " should be active but is not." << std::endl;
    } else {
        std::cout << "✓ Session " << sessionId << " is active." << std::endl;
    }

    // 3. Destroy Session
    Status destroyResult = engine->destroySession(sessionId);
    if (destroyResult != Status::OK) {
        std::cerr << "✗ Failed to destroy session: " << static_cast<int>(destroyResult)
                  << std::endl;
    } else {
        std::cout << "✓ Session destroyed." << std::endl;
    }

    // 4. Verify Session is Gone
    if (engine->isSessionActive(sessionId)) {
        std::cerr << "✗ Session " << sessionId << " should be inactive but is not." << std::endl;
    } else {
        std::cout << "✓ Session " << sessionId << " is inactive as expected." << std::endl;
    }
}

void test_multiple_sessions() {
    std::cout << "\n--- Testing Multiple Sessions ---\n";
    auto engineResult = UnifiedAudioEngine::create();
    if (!engineResult) return;
    auto engine = std::move(*engineResult);

    auto session1Result = engine->createSession();
    auto session2Result = engine->createSession();

    if (!session1Result || !session2Result) {
        std::cerr << "✗ Failed to create one or more sessions." << std::endl;
        return;
    }

    SessionId session1 = *session1Result;
    SessionId session2 = *session2Result;
    std::cout << "✓ Created two sessions with IDs: " << session1 << " and " << session2
              << std::endl;

    auto activeSessions = engine->getActiveSessions();
    if (activeSessions.size() == 2) {
        std::cout << "✓ getActiveSessions reports 2 sessions." << std::endl;
    } else {
        std::cerr << "✗ getActiveSessions reports " << activeSessions.size()
                  << " sessions, expected 2." << std::endl;
    }

    engine->destroySession(session1);
    engine->destroySession(session2);
    std::cout << "✓ Destroyed both sessions." << std::endl;
}

void test_invalid_session() {
    std::cout << "\n--- Testing Invalid Session ---\n";
    auto engineResult = UnifiedAudioEngine::create();
    if (!engineResult) return;
    auto engine = std::move(*engineResult);

    Status invalidResult = engine->destroySession(999);
    if (invalidResult == Status::SESSION_NOT_FOUND) {
        std::cout << "✓ Correctly failed to destroy non-existent session." << std::endl;
    } else {
        std::cerr << "✗ Incorrect status when destroying non-existent session." << std::endl;
    }
}

int main() {
    try {
        test_single_session_lifecycle();
        test_multiple_sessions();
        test_invalid_session();
    } catch (const std::exception& e) {
        std::cerr << "An unexpected error occurred: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
