#include "../include/huntmaster_engine/MFCCProcessor.h"

#ifdef HAVE_KISSFFT
#include "kiss_fftr.h"
#endif

#include <cmath>
#include <algorithm>
#include <numeric>

// Define M_PI if not defined (common issue with MinGW)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace huntmaster
{
    // Define static constants (needed for C++11)
    const size_t MFCCProcessor::DEFAULT_NUM_COEFFS;
    const size_t MFCCProcessor::DEFAULT_NUM_FILTERS;
    const size_t MFCCProcessor::DEFAULT_FRAME_SIZE;
    const size_t MFCCProcessor::DEFAULT_HOP_SIZE;

    class MFCCProcessor::Impl
    {
    public:
        Config config;

#ifdef HAVE_KISSFFT
        kiss_fftr_cfg fftConfig = nullptr;
        std::vector<kiss_fft_cpx> fftOutput;
#endif

        std::vector<float> window;
        std::vector<float> melFilterBank;
        std::vector<int> filterBankIndices;
        std::vector<float> dctMatrix;
        std::vector<float> powerSpectrum;
        std::vector<float> melEnergies;

        size_t frameCounter = 0;

        Impl(const Config &cfg) : config(cfg)
        {
            // Validate configuration
            if (config.highFreq == 0.0f)
            {
                config.highFreq = config.sampleRate / 2.0f;
            }

            // Initialize FFT
#ifdef HAVE_KISSFFT
            fftConfig = kiss_fftr_alloc(config.frameSize, 0, nullptr, nullptr);
            fftOutput.resize(config.frameSize / 2 + 1);
#endif

            // Initialize window function (Hamming)
            window.resize(config.frameSize);
            for (size_t i = 0; i < config.frameSize; ++i)
            {
                window[i] = 0.54f - 0.46f * cosf(2.0f * M_PI * i / (config.frameSize - 1));
            }

            // Initialize Mel filterbank
            initializeMelFilterBank();

            // Initialize DCT matrix
            initializeDCTMatrix();

            // Allocate working buffers
            powerSpectrum.resize(config.frameSize / 2 + 1);
            melEnergies.resize(config.numFilters);
        }

        ~Impl()
        {
#ifdef HAVE_KISSFFT
            if (fftConfig)
            {
                kiss_fftr_free(fftConfig);
            }
#endif
        }

        void initializeMelFilterBank()
        {
            // Convert frequency to Mel scale
            auto freqToMel = [](float freq)
            {
                return 2595.0f * log10f(1.0f + freq / 700.0f);
            };

            // Convert Mel scale to frequency
            auto melToFreq = [](float mel)
            {
                return 700.0f * (powf(10.0f, mel / 2595.0f) - 1.0f);
            };

            // Create Mel scale points
            float melLow = freqToMel(config.lowFreq);
            float melHigh = freqToMel(config.highFreq);
            float melStep = (melHigh - melLow) / (config.numFilters + 1);

            std::vector<float> melPoints;
            for (size_t i = 0; i < config.numFilters + 2; ++i)
            {
                melPoints.push_back(melLow + i * melStep);
            }

            // Convert back to Hz
            std::vector<float> freqPoints;
            for (float mel : melPoints)
            {
                freqPoints.push_back(melToFreq(mel));
            }

            // Convert to FFT bin indices
            filterBankIndices.clear();
            for (float freq : freqPoints)
            {
                int bin = static_cast<int>(freq * config.frameSize / config.sampleRate);
                filterBankIndices.push_back(bin);
            }

            // Create triangular filters
            size_t numBins = config.frameSize / 2 + 1;
            melFilterBank.resize(config.numFilters * numBins, 0.0f);

            for (size_t i = 0; i < config.numFilters; ++i)
            {
                int startBin = filterBankIndices[i];
                int centerBin = filterBankIndices[i + 1];
                int endBin = filterBankIndices[i + 2];

                // Rising edge
                for (int bin = startBin; bin < centerBin; ++bin)
                {
                    if (bin >= 0 && (size_t)bin < numBins)
                    {
                        float value = static_cast<float>(bin - startBin) / (centerBin - startBin);
                        melFilterBank[i * numBins + bin] = value;
                    }
                }

                // Falling edge
                for (int bin = centerBin; bin < endBin; ++bin)
                {
                    if (bin >= 0 && (size_t)bin < numBins)
                    {
                        float value = static_cast<float>(endBin - bin) / (endBin - centerBin);
                        melFilterBank[i * numBins + bin] = value;
                    }
                }
            }
        }

        void initializeDCTMatrix()
        {
            dctMatrix.resize(config.numCoeffs * config.numFilters);

            for (size_t i = 0; i < config.numCoeffs; ++i)
            {
                for (size_t j = 0; j < config.numFilters; ++j)
                {
                    float value = cosf(M_PI * i * (j + 0.5f) / config.numFilters);
                    value *= sqrtf(2.0f / config.numFilters);
                    if (i == 0)
                    {
                        value *= 1.0f / sqrtf(2.0f);
                    }
                    dctMatrix[i * config.numFilters + j] = value;
                }
            }
        }

        MFCCFrame processFrame(const float *audioFrame)
        {
            MFCCFrame result;
            result.frameIndex = frameCounter++;

#ifdef HAVE_KISSFFT
            // Apply window
            std::vector<float> windowedFrame(config.frameSize);
            for (size_t i = 0; i < config.frameSize; ++i)
            {
                windowedFrame[i] = audioFrame[i] * window[i];
            }

            // Compute FFT
            kiss_fftr(fftConfig, windowedFrame.data(), fftOutput.data());

            // Compute power spectrum
            for (size_t i = 0; i < powerSpectrum.size(); ++i)
            {
                float real = fftOutput[i].r;
                float imag = fftOutput[i].i;
                powerSpectrum[i] = real * real + imag * imag;
            }

            // Apply Mel filterbank
            std::fill(melEnergies.begin(), melEnergies.end(), 0.0f);
            for (size_t i = 0; i < config.numFilters; ++i)
            {
                for (size_t j = 0; j < powerSpectrum.size(); ++j)
                {
                    melEnergies[i] += melFilterBank[i * powerSpectrum.size() + j] * powerSpectrum[j];
                }
                // Log energy (with floor to avoid log(0))
                melEnergies[i] = logf(melEnergies[i] + 1e-10f);
            }

            // Apply DCT to get MFCCs
            // --- FIX: Resize the output vector before use ---
            result.coefficients.resize(config.numCoeffs);
            std::fill(result.coefficients.begin(), result.coefficients.end(), 0.0f);
            for (size_t i = 0; i < config.numCoeffs; ++i)
            {
                for (size_t j = 0; j < config.numFilters; ++j)
                {
                    // This line was causing a crash because result.coefficients was empty.
                    result.coefficients[i] += dctMatrix[i * config.numFilters + j] * melEnergies[j];
                }
            }

            // Calculate frame energy
            result.energy = 0.0f;
            for (size_t i = 0; i < config.frameSize; ++i)
            {
                result.energy += windowedFrame[i] * windowedFrame[i];
            }
            result.energy = logf(result.energy + 1e-10f);

#else
            // No Kiss FFT available - return zeros
            result.coefficients.assign(config.numCoeffs, 0.0f);
            result.energy = 0.0f;
#endif

            return result;
        }

        std::vector<MFCCFrame> processBuffer(const float *audioBuffer, size_t bufferSize)
        {
            std::vector<MFCCFrame> frames;
            reset(); // Reset frame counter before processing a new buffer.

            // Process overlapping frames
            size_t numFrames = (bufferSize >= config.frameSize) ? (bufferSize - config.frameSize) / config.hopSize + 1 : 0;

            for (size_t i = 0; i < numFrames; ++i)
            {
                size_t offset = i * config.hopSize;
                if (offset + config.frameSize <= bufferSize)
                {
                    frames.push_back(processFrame(audioBuffer + offset));
                }
            }

            return frames;
        }

        void reset()
        {
            frameCounter = 0;
        }
    };

    // Public interface implementation

    MFCCProcessor::MFCCProcessor()
        : pImpl(std::make_unique<Impl>(Config()))
    {
    }

    MFCCProcessor::MFCCProcessor(const Config &config)
        : pImpl(std::make_unique<Impl>(config))
    {
    }
    MFCCProcessor::~MFCCProcessor() = default;

    // --- Move Semantics ---
    MFCCProcessor::MFCCProcessor(MFCCProcessor &&other) noexcept = default;
    MFCCProcessor &MFCCProcessor::operator=(MFCCProcessor &&other) noexcept = default;

    MFCCProcessor::MFCCFrame MFCCProcessor::processFrame(const float *audioFrame)
    {
        return pImpl->processFrame(audioFrame);
    }

    std::vector<MFCCProcessor::MFCCFrame> MFCCProcessor::processBuffer(
        const float *audioBuffer, size_t bufferSize)
    {
        return pImpl->processBuffer(audioBuffer, bufferSize);
    }

    const MFCCProcessor::Config &MFCCProcessor::getConfig() const
    {
        return pImpl->config;
    }

    void MFCCProcessor::reset()
    {
        pImpl->reset();
    }

} // namespace huntmaster
