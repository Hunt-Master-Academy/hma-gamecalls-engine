#include <iostream>
#include "kiss_fft.h"
#include "kiss_fftr.h"
#include <vector>

int main() {
    std::cout << "Testing Kiss FFT directly..." << std::endl;
    
    const int fftSize = 2048;
    std::cout << "Creating Kiss FFT config for size " << fftSize << std::endl;
    
    kiss_fftr_cfg cfg = kiss_fftr_alloc(fftSize, 0, nullptr, nullptr);
    if (!cfg) {
        std::cout << "ERROR: Failed to allocate Kiss FFT config!" << std::endl;
        return 1;
    }
    
    std::cout << "Kiss FFT config created successfully" << std::endl;
    
    std::vector<float> input(fftSize, 0.0f);
    std::vector<kiss_fft_cpx> output(fftSize/2 + 1);
    
    std::cout << "Running FFT..." << std::endl;
    kiss_fftr(cfg, input.data(), output.data());
    std::cout << "FFT completed successfully!" << std::endl;
    
    kiss_fftr_free(cfg);
    
    return 0;
}
