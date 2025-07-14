#include <cmath>
#include <iostream>
#include <vector>

#include "huntmaster/core/AudioLevelProcessor.h"

int main() {
    std::cout << "Testing AudioLevelProcessor..." << std::endl;

    try {
        // Create processor with default config
        huntmaster::AudioLevelProcessor processor;

        if (!processor.isInitialized()) {
            std::cerr << "ERROR: Processor failed to initialize!" << std::endl;
            return 1;
        }

        std::cout << "✓ Processor initialized successfully" << std::endl;

        // Test with silent audio
        std::vector<float> silentAudio(1024, 0.0f);
        auto result = processor.processAudio(silentAudio, 1);

        if (!result.isOk()) {
            std::cerr << "ERROR: Failed to process silent audio!" << std::endl;
            return 1;
        }

        auto measurement = *result;
        std::cout << "✓ Silent audio processed - RMS: " << measurement.rmsLinear
                  << ", Peak: " << measurement.peakLinear << std::endl;

        // Test with sine wave
        const float frequency = 440.0f;
        const float amplitude = 0.5f;
        const size_t numSamples = 1024;
        const float sampleRate = 44100.0f;

        std::vector<float> sineWave(numSamples);
        for (size_t i = 0; i < numSamples; ++i) {
            const float t = static_cast<float>(i) / sampleRate;
            sineWave[i] = amplitude * std::sin(2.0f * M_PI * frequency * t);
        }

        result = processor.processAudio(sineWave, 1);

        if (!result.isOk()) {
            std::cerr << "ERROR: Failed to process sine wave!" << std::endl;
            return 1;
        }

        measurement = *result;
        std::cout << "✓ Sine wave processed - RMS: " << measurement.rmsLinear
                  << " (dB: " << measurement.rmsDb << "), Peak: " << measurement.peakLinear
                  << " (dB: " << measurement.peakDb << ")" << std::endl;

        // Test JSON export
        std::string json = processor.exportToJson();
        std::cout << "✓ JSON export: " << json << std::endl;

        // Test configuration
        auto config = processor.getConfig();
        std::cout << "✓ Config - Sample Rate: " << config.sampleRate
                  << ", History Size: " << config.historySize << std::endl;

        std::cout << "\n🎉 All AudioLevelProcessor tests passed!" << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "EXCEPTION: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "UNKNOWN EXCEPTION occurred!" << std::endl;
        return 1;
    }
}
