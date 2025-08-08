/**
 * @file verify_processed_calls.cpp
 * @brief Verification script to test that processed master calls can be loaded
 */

#include <filesystem>
#include <iostream>

#include "huntmaster/core/UnifiedAudioEngine.h"

using namespace huntmaster;

int main() {
    std::cout << "🔍 Verifying processed master calls..." << std::endl;

    // Initialize engine
    auto engineResult = UnifiedAudioEngine::create();
    if (!engineResult.isOk()) {
        std::cerr << "❌ Failed to initialize UnifiedAudioEngine" << std::endl;
        return 1;
    }

    auto engine = std::move(engineResult.value);

    // Create session
    auto sessionResult = engine->createSession(44100.0f);
    if (!sessionResult.isOk()) {
        std::cerr << "❌ Failed to create session" << std::endl;
        return 1;
    }

    SessionId sessionId = sessionResult.value;

    // Test loading processed .mfc files
    std::string mfcDir = "data/processed_calls/mfc";
    int successCount = 0;
    int totalCount = 0;

    std::cout << "\n📁 Testing .mfc file loading:" << std::endl;

    for (const auto& entry : std::filesystem::directory_iterator(mfcDir)) {
        if (entry.path().extension() == ".mfc") {
            totalCount++;
            std::string filename = entry.path().filename().stem().string();

            auto result = engine->loadMasterCall(sessionId, filename);
            if (result == UnifiedAudioEngine::Status::OK) {
                successCount++;
                std::cout << "✅ " << filename << std::endl;
            } else {
                std::cout << "❌ " << filename << " - Load failed" << std::endl;
            }
        }
    }

    std::cout << "\n📊 Results Summary:" << std::endl;
    std::cout << "=================" << std::endl;
    std::cout << "Total files: " << totalCount << std::endl;
    std::cout << "Successfully loaded: " << successCount << std::endl;
    std::cout << "Success rate: " << (successCount * 100.0 / totalCount) << "%" << std::endl;

    // Test getting feature count after loading
    auto featureCountResult = engine->getFeatureCount(sessionId);
    if (featureCountResult.isOk()) {
        std::cout << "Feature count: " << featureCountResult.value << std::endl;
    }

    // Cleanup
    auto status = engine->destroySession(sessionId);
    (void)status;  // Acknowledge we're intentionally ignoring return

    if (successCount == totalCount) {
        std::cout << "\n🎉 All processed master calls verified successfully!" << std::endl;
        return 0;
    } else {
        std::cout << "\n⚠️ Some master calls failed to load." << std::endl;
        return 1;
    }
}
