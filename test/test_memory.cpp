#include "memory/Memory.h"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace {

TEST(MemoryTest, ReadsWritesAndResetsIndividualBytesAndRanges) {
    simulator::Memory memory;
    EXPECT_EQ(memory.Read(0), 0);
    memory.Write(0x1234, 0xAB);
    EXPECT_EQ(memory.Read(0x1234), 0xAB);

    constexpr std::array<uint8_t, 3> bytes = {0x12, 0x34, 0x56};
    memory.WriteRange(0xFFFF - 2, bytes.data(), bytes.size());
    EXPECT_EQ(memory.ReadRange(0xFFFF - 2, bytes.size()),
        (std::vector<uint8_t>{0x12, 0x34, 0x56}));
    EXPECT_EQ(memory.ReadRange(0xFFFF, 1).size(), 1);

    memory.Reset();
    EXPECT_EQ(memory.Read(0x1234), 0);
    EXPECT_EQ(memory.Read(0xFFFF), 0);
}

TEST(MemoryTest, RejectsRangesThatCrossTheAddressSpaceBoundary) {
    simulator::Memory memory;
    const uint8_t value = 0xFF;
    EXPECT_THROW(memory.ReadRange(0xFFFF, 2), std::out_of_range);
    EXPECT_THROW(memory.WriteRange(0xFFFF, &value, 2), std::out_of_range);
}

} // namespace
