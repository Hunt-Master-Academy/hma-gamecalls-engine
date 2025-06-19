#include "huntmaster_engine/MFCCProcessor.h"

#ifdef HAVE_KISSFFT
#include "kiss_fftr.h"
#endif

#include <cmath>
#include <algorithm>
#include <numeric>

namespace huntmaster {

// Define static constexpr variables to resolve linker errors.
constexpr size_t MFCCProcessor::DEFAULT_NUM_COEFFS;
constexpr size_t MFCCProcessor::DEFAULT_NUM_FILTERS;
constexpr size_t MFCCProcessor::DEFAULT_FRAME_SIZE;
constexpr size_t MFCCProcessor::DEFAULT_HOP_SIZE;

class MFCCProcessor::Impl {
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
    
    Impl(const Config& cfg) : config(cfg) {
        if (config.highFreq == 0.0f) {
            config.highFreq = config.sampleRate / 2.0f;
        }
        
#ifdef HAVE_KISSFFT
        fftConfig = kiss_fftr_alloc(config.frameSize, 0, nullptr, nullptr);
        fftOutput.resize(config.frameSize / 2 + 1);
#endif
        
        window.resize(config.frameSize);
        for (size_t i = 0; i < config.frameSize; ++i) {
            window[i] = 0.54f - 0.46f * cosf(2.0f * M_PI * i / (config.frameSize - 1));
        }
        
        initializeMelFilterBank();
        initializeDCTMatrix();
        
        powerSpectrum.resize(config.frameSize / 2 + 1);
        melEnergies.resize(config.numFilters);
    }
    
    ~Impl() {
#ifdef HAVE_KISSFFT
        if (fftConfig) {
            kiss_fftr_free(fftConfig);
        }
#endif
    }
    
    void initializeMelFilterBank() {
        auto freqToMel = [](float freq) { return 2595.0f * log10f(1.0f + freq / 700.0f); };
        auto melToFreq = [](float mel) { return 700.0f * (powf(10.0f, mel / 2595.0f) - 1.0f); };
        
        float melLow = freqToMel(config.lowFreq);
        float melHigh = freqToMel(config.highFreq);
        float melStep = (melHigh - melLow) / (config.numFilters + 1);
        
        std::vector<float> melPoints(config.numFilters + 2);
        for (size_t i = 0; i < melPoints.size(); ++i) melPoints[i] = melLow + i * melStep;
        
        std::vector<float> freqPoints(melPoints.size());
        for (size_t i = 0; i < freqPoints.size(); ++i) freqPoints[i] = melToFreq(melPoints[i]);
        
        filterBankIndices.resize(freqPoints.size());
        for (size_t i = 0; i < filterBankIndices.size(); ++i) filterBankIndices[i] = static_cast<int>(freqPoints[i] * config.frameSize / config.sampleRate);
        
        size_t numBins = config.frameSize / 2 + 1;
        melFilterBank.assign(config.numFilters * numBins, 0.0f);
        
        for (size_t i = 0; i < config.numFilters; ++i) {
            for (int bin = filterBankIndices[i]; bin < filterBankIndices[i+1]; ++bin) {
                if(bin >= 0 && bin < numBins) melFilterBank[i*numBins + bin] = (float)(bin - filterBankIndices[i]) / (float)(filterBankIndices[i+1] - filterBankIndices[i]);
            }
            for (int bin = filterBankIndices[i+1]; bin < filterBankIndices[i+2]; ++bin) {
                 if(bin >= 0 && bin < numBins) melFilterBank[i*numBins + bin] = (float)(filterBankIndices[i+2] - bin) / (float)(filterBankIndices[i+2] - filterBankIndices[i+1]);
            }
        }
    }
    
    void initializeDCTMatrix() {
        dctMatrix.resize(config.numCoeffs * config.numFilters);
        float scale = sqrtf(2.0f / config.numFilters);
        for (size_t i = 0; i < config.numCoeffs; ++i) {
            for (size_t j = 0; j < config.numFilters; ++j) {
                float val = cosf(M_PI * i * (j + 0.5f) / config.numFilters);
                dctMatrix[i * config.numFilters + j] = (i == 0) ? val * (1.0f / sqrtf(2.0f)) : val * scale;
            }
        }
    }
    
    MFCCFrame processFrame(const float* audioFrame) {
        MFCCFrame result;
        result.frameIndex = frameCounter++;
        
#ifdef HAVE_KISSFFT
        std::vector<float> windowedFrame(config.frameSize);
        for (size_t i = 0; i < config.frameSize; ++i) windowedFrame[i] = audioFrame[i] * window[i];
        
        kiss_fftr(fftConfig, windowedFrame.data(), fftOutput.data());
        
        for (size_t i = 0; i < powerSpectrum.size(); ++i) powerSpectrum[i] = fftOutput[i].r * fftOutput[i].r + fftOutput[i].i * fftOutput[i].i;
        
        std::fill(melEnergies.begin(), melEnergies.end(), 0.0f);
        for (size_t i = 0; i < config.numFilters; ++i) {
            for (size_t j = 0; j < powerSpectrum.size(); ++j) melEnergies[i] += melFilterBank[i * powerSpectrum.size() + j] * powerSpectrum[j];
            melEnergies[i] = logf(melEnergies[i] + 1e-10f);
        }
        
        std::fill(result.coefficients.begin(), result.coefficients.end(), 0.0f);
        for (size_t i = 0; i < config.numCoeffs; ++i) {
            for (size_t j = 0; j < config.numFilters; ++j) result.coefficients[i] += dctMatrix[i * config.numFilters + j] * melEnergies[j];
        }
        
        result.energy = 0.0f;
        for (size_t i = 0; i < config.frameSize; ++i) result.energy += windowedFrame[i] * windowedFrame[i];
        result.energy = logf(result.energy + 1e-10f);
#else
        std::fill(result.coefficients.begin(), result.coefficients.end(), 0.0f);
        result.energy = 0.0f;
#endif
        
        return result;
    }
    
    std::vector<MFCCFrame> processBuffer(const float* audioBuffer, size_t bufferSize) {
        std::vector<MFCCFrame> frames;
        size_t numFrames = (bufferSize >= config.frameSize) ? (bufferSize - config.frameSize) / config.hopSize + 1 : 0;
        for (size_t i = 0; i < numFrames; ++i) {
            size_t offset = i * config.hopSize;
            frames.push_back(processFrame(audioBuffer + offset));
        }
        return frames;
    }
};

MFCCProcessor::MFCCProcessor(const Config& config) : pImpl(std::make_unique<Impl>(config)) {}
MFCCProcessor::~MFCCProcessor() = default;
MFCCProcessor::MFCCFrame MFCCProcessor::processFrame(const float* audioFrame) { return pImpl->processFrame(audioFrame); }
std::vector<MFCCProcessor::MFCCFrame> MFCCProcessor::processBuffer(const float* audioBuffer, size_t bufferSize) { return pImpl->processBuffer(audioBuffer, bufferSize); }
const MFCCProcessor::Config& MFCCProcessor::getConfig() const { return pImpl->config; }
void MFCCProcessor::reset() { pImpl->frameCounter = 0; }

} // namespace huntmaster
