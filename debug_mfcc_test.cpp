#include <cmath>
#include <iostream>
#include <vector>

#include "huntmaster/core/MFCCProcessor.h"

int main() {
    std::cout << "=== Direct MFCC Processor Test ===" << std::endl;

    // Create a simple test signal (440 Hz sine wave)
    const size_t sampleRate = 44100;
    const size_t frameSize = 512;
    const float frequency = 440.0f;
    const float duration = 0.1f;  // 100ms

    std::vector<float> testSignal(static_cast<size_t>(sampleRate * duration));
    for (size_t i = 0; i < testSignal.size(); ++i) {
        testSignal[i] = 0.5f * sin(2.0f * M_PI * frequency * i / sampleRate);
    }

    std::cout << "Generated test signal: " << testSignal.size() << " samples" << std::endl;

    // Create MFCC processor
    huntmaster::MFCCProcessor::Config config{.sample_rate = sampleRate,
                                             .frame_size = frameSize,
                                             .num_coefficients = 13,
                                             .num_filters = 26};

    try {
        huntmaster::MFCCProcessor processor(config);
        std::cout << "MFCC processor created successfully" << std::endl;

        // Test single frame extraction
        if (testSignal.size() >= frameSize) {
            std::span<const float> frame(testSignal.data(), frameSize);
            auto result = processor.extractFeatures(frame);

            if (result) {
                auto& features = result.value();
                std::cout << "Single frame extraction: SUCCESS" << std::endl;
                std::cout << "Features extracted: " << features.size() << std::endl;

                if (!features.empty()) {
                    std::cout << "First few MFCC coefficients: ";
                    for (size_t i = 0; i < std::min(size_t(5), features.size()); ++i) {
                        std::cout << features[i] << " ";
                    }
                    std::cout << std::endl;
                } else {
                    std::cout << "ERROR: No features in result vector!" << std::endl;
                }
            } else {
                std::cout << "Single frame extraction: FAILED" << std::endl;
                std::cout << "Error code: " << static_cast<int>(result.error()) << std::endl;
            }

            // Test buffer extraction with different hop sizes
            std::cout << "\n--- Testing buffer extraction ---" << std::endl;

            const size_t hopSize = frameSize / 2;  // 50% overlap
            auto bufferResult = processor.extractFeaturesFromBuffer(testSignal, hopSize);

            if (bufferResult) {
                auto& allFeatures = bufferResult.value();
                std::cout << "Buffer extraction: SUCCESS" << std::endl;
                std::cout << "Total frames processed: " << allFeatures.size() << std::endl;

                if (!allFeatures.empty() && !allFeatures[0].empty()) {
                    std::cout << "Features per frame: " << allFeatures[0].size() << std::endl;
                    std::cout << "First frame MFCC[0]: " << allFeatures[0][0] << std::endl;
                } else {
                    std::cout << "ERROR: No features extracted from buffer!" << std::endl;
                }
            } else {
                std::cout << "Buffer extraction: FAILED" << std::endl;
                std::cout << "Error code: " << static_cast<int>(bufferResult.error()) << std::endl;
            }
        } else {
            std::cout << "ERROR: Test signal too short for frame size" << std::endl;
        }

    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "\n=== Test Complete ===" << std::endl;
    return 0;
}
