#include <iostream>

#include "huntmaster/core/DebugLogger.h"

int main() {
    std::cout << "Testing debug macros..." << std::endl;

    // Test the macro directly
    DTW_LOG_DEBUG("This is a test message");
    std::cout << "DTW_LOG_DEBUG macro executed successfully" << std::endl;

    return 0;
}
