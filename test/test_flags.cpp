#include "cpu/Cpu8085.h"

#include <gtest/gtest.h>

#include <initializer_list>

namespace {

class FlagsTest : public testing::Test {
protected:
    simulator::Memory memory;
    simulator::Cpu8085 cpu{memory};

    void Load(std::initializer_list<uint8_t> program) {
        memory.WriteRange(0, program.begin(), program.size());
    }
};

TEST_F(FlagsTest, AdcCarryInCanOverflowAccumulatorAndAuxiliaryCarry) {
    cpu.SetRegister("A", 0xFF);
    cpu.SetRegister("B", 0x00);
    Load({0x37, 0x88}); // STC; ADC B

    cpu.Step();
    const auto state = cpu.Step();

    EXPECT_EQ(state.a, 0x00);
    EXPECT_TRUE(state.zero);
    EXPECT_TRUE(state.parity);
    EXPECT_TRUE(state.auxiliaryCarry);
    EXPECT_TRUE(state.carry);
}

TEST_F(FlagsTest, SbbCarryInProducesBorrowEvenWhenResultWrapsToZero) {
    cpu.SetRegister("A", 0x00);
    cpu.SetRegister("B", 0xFF);
    Load({0x37, 0x98}); // STC; SBB B

    cpu.Step();
    const auto state = cpu.Step();

    EXPECT_EQ(state.a, 0x00);
    EXPECT_TRUE(state.zero);
    EXPECT_TRUE(state.parity);
    EXPECT_TRUE(state.auxiliaryCarry);
    EXPECT_TRUE(state.carry);
}

TEST_F(FlagsTest, IncrementSetsZeroAndAuxiliaryCarryButPreservesCarry) {
    cpu.SetRegister("B", 0x0F);
    Load({0x37, 0x04}); // STC; INR B

    cpu.Step();
    const auto state = cpu.Step();

    EXPECT_EQ(state.b, 0x10);
    EXPECT_TRUE(state.auxiliaryCarry);
    EXPECT_TRUE(state.carry);
    EXPECT_FALSE(state.zero);
    EXPECT_FALSE(state.sign);
}

TEST_F(FlagsTest, DecrementSetsSignAndPreservesCarry) {
    cpu.SetRegister("B", 0x00);
    Load({0x37, 0x05}); // STC; DCR B

    cpu.Step();
    const auto state = cpu.Step();

    EXPECT_EQ(state.b, 0xFF);
    EXPECT_TRUE(state.sign);
    EXPECT_TRUE(state.auxiliaryCarry);
    EXPECT_TRUE(state.parity);
    EXPECT_TRUE(state.carry);
}

TEST_F(FlagsTest, CompareUpdatesFlagsWithoutChangingAccumulator) {
    cpu.SetRegister("A", 0x00);
    cpu.SetRegister("B", 0x01);
    Load({0xB8}); // CMP B

    const auto state = cpu.Step();

    EXPECT_EQ(state.a, 0x00);
    EXPECT_TRUE(state.sign);
    EXPECT_FALSE(state.zero);
    EXPECT_TRUE(state.auxiliaryCarry);
    EXPECT_TRUE(state.parity);
    EXPECT_TRUE(state.carry);
}

} // namespace
