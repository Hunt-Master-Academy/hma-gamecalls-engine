#ifndef HUNTMASTER_MFCC_PROCESSOR_H
#define HUNTMASTER_MFCC_PROCESSOR_H

#include <vector>
#include <array>
#include <memory>
#include <cstddef>

namespace huntmaster {

/**
 * MFCC (Mel-Frequency Cepstral Coefficients) processor for audio feature extraction
 */
class MFCCProcessor {
public:
    static constexpr size_t DEFAULT_NUM_COEFFS = 13;
    static constexpr size_t DEFAULT_NUM_FILTERS = 26;
    static constexpr size_t DEFAULT_FRAME_SIZE = 2048;
    static constexpr size_t DEFAULT_HOP_SIZE = 512;
    
    struct Config {
        float sampleRate = 44100.0f;
        size_t frameSize = DEFAULT_FRAME_SIZE;
        size_t hopSize = DEFAULT_HOP_SIZE;
        size_t numCoeffs = DEFAULT_NUM_COEFFS;
        size_t numFilters = DEFAULT_NUM_FILTERS;
        float lowFreq = 0.0f;      // Hz
        float highFreq = 0.0f;     // Hz (0 = sampleRate/2)
        bool useEnergy = true;     // Include energy as 0th coefficient
    };
    
    struct MFCCFrame {
        std::array<float, DEFAULT_NUM_COEFFS> coefficients;
        float energy;
        size_t frameIndex;
    };
    
    // Updated Constructor: Removed the default argument to support older compilers.
    explicit MFCCProcessor(const Config& config);
    ~MFCCProcessor();
    
    // Process a single frame of audio
    MFCCFrame processFrame(const float* audioFrame);
    
    // Process a buffer of audio and return multiple frames
    std::vector<MFCCFrame> processBuffer(const float* audioBuffer, size_t bufferSize);
    
    // Get configuration
    const Config& getConfig() const;
    
    // Reset internal state
    void reset();
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace huntmaster

#endif // HUNTMASTER_MFCC_PROCESSOR_H
