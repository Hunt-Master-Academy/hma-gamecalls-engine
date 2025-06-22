#include <iostream>
#include "huntmaster_engine/MFCCProcessor.h"
#include <vector>
#include <cmath>

int main() {
    std::cout << "Creating MFCC processor..." << std::endl;
    
    huntmaster::MFCCProcessor::Config config;
    huntmaster::MFCCProcessor processor(config);
    
    std::cout << "Config: sampleRate=" << config.sampleRate 
              << ", frameSize=" << config.frameSize << std::endl;
    
    // Create test signal
    std::vector<float> testSignal(config.frameSize);
    for (size_t i = 0; i < config.frameSize; ++i) {
        testSignal[i] = std::sin(2.0f * 3.14159f * 440.0f * i / config.sampleRate);
    }
    
    std::cout << "Processing frame..." << std::endl;
    auto frame = processor.processFrame(testSignal.data());
    
    std::cout << "Frame processed successfully!" << std::endl;
    std::cout << "Energy: " << frame.energy << std::endl;
    std::cout << "First coefficient: " << frame.coefficients[0] << std::endl;
    
    return 0;
}
