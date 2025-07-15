#include <iostream>
#include <vector>

float computeEnergy(const std::vector<float>& audio) {
    if (audio.empty()) return 0.0f;
    double sum_sq = 0.0;
    for (float sample : audio) {
        sum_sq += sample * sample;
    }
    return static_cast<float>(sum_sq / audio.size());
}

int main() {
    std::vector<float> voice(320, 0.2f);
    float energy = computeEnergy(voice);
    std::cout << "Energy: " << energy << std::endl;
    std::cout << "Threshold: 0.01" << std::endl;
    std::cout << "Above threshold: " << (energy > 0.01f) << std::endl;
    return 0;
}
