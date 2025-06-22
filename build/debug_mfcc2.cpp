#include <iostream>
#include "huntmaster_engine/MFCCProcessor.h"
#include <vector>
#include <cmath>

int main() {
    std::cout << "Creating config..." << std::endl;
    huntmaster::MFCCProcessor::Config config;
    
    std::cout << "Config values:" << std::endl;
    std::cout << "  sampleRate: " << config.sampleRate << std::endl;
    std::cout << "  frameSize: " << config.frameSize << std::endl;
    std::cout << "  hopSize: " << config.hopSize << std::endl;
    std::cout << "  numCoeffs: " << config.numCoeffs << std::endl;
    std::cout << "  numFilters: " << config.numFilters << std::endl;
    std::cout << "  lowFreq: " << config.lowFreq << std::endl;
    std::cout << "  highFreq: " << config.highFreq << std::endl;
    
    // Set lowFreq to avoid potential log(0) issues
    config.lowFreq = 20.0f;  // 20 Hz is a common lower bound
    
    std::cout << "\nCreating MFCC processor..." << std::endl;
    huntmaster::MFCCProcessor processor(config);
    
    std::cout << "Processor created successfully!" << std::endl;
    
    // Create test signal
    std::vector<float> testSignal(config.frameSize, 0.0f);
    
    std::cout << "Processing frame..." << std::endl;
    auto frame = processor.processFrame(testSignal.data());
    
    std::cout << "Frame processed successfully!" << std::endl;
    
    return 0;
}
