#include "cpu/Cpu8085.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>

namespace {

struct ExpectedFlags {
    bool sign;
    bool zero;
    bool auxiliaryCarry;
    bool parity;
    bool carry;
};

struct AluCase {
    uint8_t opcode;
    uint8_t regA_init;
    uint8_t regB_init;
    uint8_t expected_A;
    ExpectedFlags expected_flags;
};

class ArithmeticInstructionsTest : public testing::TestWithParam<AluCase> {
protected:
    simulator::Memory memory;
    simulator::Cpu8085 cpu{memory};

    void ExpectFlags(const ExpectedFlags& expected) {
        const auto state = cpu.GetState();
        EXPECT_EQ(state.sign, expected.sign);
        EXPECT_EQ(state.zero, expected.zero);
        EXPECT_EQ(state.auxiliaryCarry, expected.auxiliaryCarry);
        EXPECT_EQ(state.parity, expected.parity);
        EXPECT_EQ(state.carry, expected.carry);
    }
};

TEST_P(ArithmeticInstructionsTest, ComputesAccumulatorAndFlags) {
    const auto& testCase = GetParam();
    cpu.SetRegister("A", testCase.regA_init);
    cpu.SetRegister("B", testCase.regB_init);

    const bool usesCarry = (testCase.opcode & 0x08) != 0;
    const uint16_t instructionAddress = usesCarry ? 1 : 0;
    if (usesCarry) {
        memory.Write(0, 0x37); // STC establishes ADC/SBB carry-in.
    }
    memory.Write(instructionAddress, testCase.opcode);

    if (usesCarry) {
        cpu.Step();
    }
    cpu.Step();

    EXPECT_EQ(cpu.GetState().a, testCase.expected_A);
    ExpectFlags(testCase.expected_flags);
}

INSTANTIATE_TEST_SUITE_P(
    AddAdcSubSbb,
    ArithmeticInstructionsTest,
    testing::Values(
        AluCase{0x80, 0x01, 0x02, 0x03, {false, false, false, true, false}},
        AluCase{0x80, 0x0F, 0x01, 0x10, {false, false, true, false, false}},
        AluCase{0x80, 0xFF, 0x01, 0x00, {false, true, true, true, true}},
        AluCase{0x88, 0x0F, 0x00, 0x10, {false, false, true, false, false}},
        AluCase{0x88, 0xFF, 0x00, 0x00, {false, true, true, true, true}},
        AluCase{0x90, 0x05, 0x03, 0x02, {false, false, false, false, false}},
        AluCase{0x90, 0x00, 0x01, 0xFF, {true, false, true, true, true}},
        AluCase{0x98, 0x10, 0x0F, 0x00, {false, true, true, true, false}},
        AluCase{0x98, 0x00, 0xFF, 0x00, {false, true, true, true, true}}),
    [](const testing::TestParamInfo<AluCase>& info) {
        return "Opcode" + std::to_string(info.param.opcode) +
            "_A" + std::to_string(info.param.regA_init) +
            "_B" + std::to_string(info.param.regB_init);
    });

} // namespace
