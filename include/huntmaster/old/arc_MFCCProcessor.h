#ifndef HUNTMASTER_MFCC_PROCESSOR_H
#define HUNTMASTER_MFCC_PROCESSOR_H

#include <vector>
#include <array>
#include <memory>
#include <cstddef>

namespace huntmaster
{

    /**
     * MFCC (Mel-Frequency Cepstral Coefficients) processor for audio feature extraction
     */
    class MFCCProcessor
    {
    public:
        static constexpr size_t DEFAULT_NUM_COEFFS = 13;
        static constexpr size_t DEFAULT_NUM_FILTERS = 26;
        static constexpr size_t DEFAULT_FRAME_SIZE = 2048;
        static constexpr size_t DEFAULT_HOP_SIZE = 512;

        struct Config
        {
            float sampleRate;
            size_t frameSize;
            size_t hopSize;
            size_t numCoeffs;
            size_t numFilters;
            float lowFreq;  // Hz
            float highFreq; // Hz (0 = sampleRate/2)
            bool useEnergy; // Include energy as 0th coefficient

            // Default constructor
            Config()
                : sampleRate(44100.0f),
                  frameSize(DEFAULT_FRAME_SIZE),
                  hopSize(DEFAULT_HOP_SIZE),
                  numCoeffs(DEFAULT_NUM_COEFFS),
                  numFilters(DEFAULT_NUM_FILTERS),
                  lowFreq(0.0f),
                  highFreq(0.0f),
                  useEnergy(true)
            {
            }
        };

        struct MFCCFrame
        {
            std::vector<float> coefficients;
            float energy;
            size_t frameIndex;
        };

        MFCCProcessor(); // Default constructor
        explicit MFCCProcessor(const Config &config);
        ~MFCCProcessor(); // Needs to be defined in the .cpp file

        // Enable move semantics, disable copy semantics
        MFCCProcessor(MFCCProcessor &&) noexcept;
        MFCCProcessor &operator=(MFCCProcessor &&) noexcept;

        // Process a single frame of audio
        MFCCFrame processFrame(const float *audioFrame);

        // Process a buffer of audio and return multiple frames
        std::vector<MFCCFrame> processBuffer(const float *audioBuffer, size_t bufferSize);

        // Get configuration
        const Config &getConfig() const;

        // Reset internal state
        void reset();

    private:
        class Impl;
        std::unique_ptr<Impl> pImpl;
    };

} // namespace huntmaster

#endif // HUNTMASTER_MFCC_PROCESSOR_H