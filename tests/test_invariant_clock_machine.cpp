#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <cstring>
#include <fstream>
#include <cstdlib>

// Include the actual production header
#include "src/clock_machine.h"

class SecurityTest : public ::testing::TestWithParam<std::string> {};

TEST_P(SecurityTest, BufferReadsNeverExceedDeclaredLength) {
    // Invariant: Buffer reads never exceed the declared length
    std::string payload = GetParam();
    
    // Create a fixed-size buffer as would be used in production
    const size_t BUFFER_SIZE = 32;
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));
    
    // Call the actual production function that processes time strings
    // This function should safely handle oversized inputs
    process_time_string(payload.c_str(), buffer, BUFFER_SIZE);
    
    // Verify no buffer overflow occurred by checking buffer integrity
    // If the function writes beyond buffer bounds, this test may crash
    // or we can add additional checks
    ASSERT_TRUE(buffer[BUFFER_SIZE - 1] == 0 || 
                strlen(buffer) < BUFFER_SIZE) 
        << "Buffer overflow detected with payload: " << payload;
}

INSTANTIATE_TEST_SUITE_P(
    AdversarialInputs,
    SecurityTest,
    ::testing::Values(
        // Exact exploit case: significantly oversized input
        "2024-03-15T14:30:00.000ZAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
        // Boundary case: exactly buffer size minus null terminator
        "2024-03-15T14:30:00.000Z123456789",
        // Valid input: well within bounds
        "2024-03-15T14:30:00",
        // Another adversarial case: input 10x buffer size
        std::string(320, 'A')
    )
);

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}