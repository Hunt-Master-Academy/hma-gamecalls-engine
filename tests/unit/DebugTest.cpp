#include <gtest/gtest.h>

#include <iostream>

TEST(DebugTest, CheckStdoutInTests) {
    std::cout << "This is a test cout message" << std::endl;
    std::cout.flush();
    EXPECT_TRUE(true);
}
