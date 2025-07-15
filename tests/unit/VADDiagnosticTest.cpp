#include <gtest/gtest.h>

#include <chrono>
#include <vector>

#include "huntmaster/core/VoiceActivityDetector.h"

using namespace huntmaster;
using namespace std::chrono_literals;

class VADDiagnosticTest : public ::testing::Test {
   protected:
    std::vector<float> generateSignalChunk(size_t samples, float level) {
        return std::vector<float>(samples, level);
    }
};

TEST_F(VADDiagnosticTest, DiagnoseFrameCounting) {
    VoiceActivityDetector::Config config;
    config.energy_threshold = 0.01f;
    config.window_duration = 10ms;
    config.min_sound_duration = 30ms;
    config.post_buffer = 50ms;

    VoiceActivityDetector vad(config);

    auto signal_chunk = generateSignalChunk(160, 0.2f);  // 10ms at 16kHz

    // Check what happens with each frame
    for (int i = 1; i <= 4; i++) {
        auto result = vad.processWindow(signal_chunk);
        ASSERT_TRUE(result.has_value());

        // Print diagnostic info
        std::cout << "Frame " << i << ": energy=" << result.value().energy_level
                  << ", is_active=" << result.value().is_active
                  << ", vad.isVoiceActive()=" << vad.isVoiceActive() << std::endl;

        if (i == 3) {
            // After 3 frames (30ms), it should be active
            EXPECT_TRUE(result.value().is_active) << "Frame " << i << " should be active";
            EXPECT_TRUE(vad.isVoiceActive()) << "VAD should report active after frame " << i;
        }
    }
}
