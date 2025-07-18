#include <iostream>
#include <memory>
#include <vector>

#include "huntmaster/core/RealtimeScorer.h"

using namespace huntmaster;

int main() {
    std::cout << "Creating RealtimeScorer..." << std::endl;
    auto scorer = std::make_unique<RealtimeScorer>();
    std::cout << "RealtimeScorer created successfully" << std::endl;

    std::cout << "Setting master call..." << std::endl;
    bool result = scorer->setMasterCall("test_master_call.wav");
    std::cout << "setMasterCall result: " << (result ? "SUCCESS" : "FAILED") << std::endl;

    if (result) {
        std::cout << "Processing audio chunks..." << std::endl;
        for (int i = 0; i < 3; ++i) {
            std::cout << "Processing chunk " << i << std::endl;
            std::vector<float> audio(1024, 0.5f);
            auto audioResult = scorer->processAudio(audio, 1);
            std::cout << "processAudio result: " << (audioResult.has_value() ? "SUCCESS" : "FAILED")
                      << std::endl;
        }

        std::cout << "Calling reset..." << std::endl;
        scorer->reset();
        std::cout << "Reset completed successfully" << std::endl;
    }

    return 0;
}
