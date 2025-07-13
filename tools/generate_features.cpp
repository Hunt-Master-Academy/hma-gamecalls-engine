#include <iostream>
#include <string>
#include <vector>

#include "huntmaster/core/HuntmasterAudioEngine.h"

using huntmaster::HuntmasterAudioEngine;

int main() {
    std::cout << "=== MFCC Feature Generator ===" << std::endl;

    std::vector<std::string> calls = {"breeding_bellow", "buck_grunt",     "buck_rage_grunts",
                                      "buck-bawl",       "contact-bleatr", "doe-grunt",
                                      "doebleat",        "estrus_bleat",   "fawn-bleat",
                                      "sparring_bucks",  "tending_grunts"};

    HuntmasterAudioEngine &engine = HuntmasterAudioEngine::getInstance();
    engine.initialize();

    for (const auto &call : calls) {
        std::cout << "\nProcessing: " << call << std::endl;
        engine.loadMasterCall(call);
    }

    std::cout << "\nAll features generated!" << std::endl;
    engine.shutdown();
    return 0;
}
