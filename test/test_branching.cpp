#include "cpu/Cpu8085.h"

#include <gtest/gtest.h>

#include <initializer_list>

namespace {

class BranchingTest : public testing::Test {
protected:
    simulator::Memory memory;
    simulator::Cpu8085 cpu{memory};

    void Load(std::initializer_list<uint8_t> program) {
        memory.WriteRange(0, program.begin(), program.size());
        cpu.SetRegister("PC", 0);
    }

    void Compare(uint8_t accumulator, uint8_t operand) {
        cpu.SetRegister("A", accumulator);
        cpu.SetRegister("B", operand);
        memory.Write(0, 0xB8); // CMP B establishes the condition flags.
        cpu.Step();
    }
};

TEST_F(BranchingTest, ConditionalJumpTakesAndSkipsBasedOnZeroFlag) {
    Compare(0x01, 0x01);
    Load({0xCA, 0x34, 0x12}); // JZ 1234H

    EXPECT_EQ(cpu.Step().pc, 0x1234);

    cpu.Reset();
    Compare(0x01, 0x00);
    Load({0xCA, 0x34, 0x12});
    EXPECT_EQ(cpu.Step().pc, 0x0003);
}

TEST_F(BranchingTest, UnconditionalJumpCallAndReturnTransferControl) {
    cpu.SetRegister("SP", 0x4000);
    Load({0xC3, 0x20, 0x00}); // JMP 0020H
    EXPECT_EQ(cpu.Step().pc, 0x0020);

    memory.Write(0x20, 0xCD);
    memory.Write(0x21, 0x30);
    memory.Write(0x22, 0x00);
    EXPECT_EQ(cpu.Step().pc, 0x0030);
    EXPECT_EQ(cpu.GetState().sp, 0x3FFE);
    EXPECT_EQ(memory.Read(0x3FFE), 0x23);
    EXPECT_EQ(memory.Read(0x3FFF), 0x00);

    memory.Write(0x30, 0xC9); // RET
    const auto state = cpu.Step();
    EXPECT_EQ(state.pc, 0x0023);
    EXPECT_EQ(state.sp, 0x4000);
}

TEST_F(BranchingTest, EveryConditionalJumpTestsItsAssociatedFlag) {
    struct JumpCase {
        uint8_t opcode;
        uint8_t accumulator;
        uint8_t operand;
    };
    const JumpCase cases[] = {
        {0xC2, 0x01, 0x00}, // JNZ
        {0xCA, 0x01, 0x01}, // JZ
        {0xD2, 0x01, 0x00}, // JNC
        {0xDA, 0x00, 0x01}, // JC
        {0xE2, 0x01, 0x00}, // JPO
        {0xEA, 0x00, 0x00}, // JPE
        {0xF2, 0x01, 0x00}, // JP
        {0xFA, 0x00, 0x01}, // JM
    };

    for (const auto& testCase : cases) {
        cpu.Reset();
        cpu.SetRegister("A", testCase.accumulator);
        cpu.SetRegister("B", testCase.operand);
        memory.Write(0, 0xB8); // CMP B
        cpu.Step();
        memory.Write(0, testCase.opcode);
        memory.Write(1, 0x34);
        memory.Write(2, 0x12);
        cpu.SetRegister("PC", 0);

        EXPECT_EQ(cpu.Step().pc, 0x1234) << "opcode 0x" << std::hex
            << static_cast<unsigned>(testCase.opcode);
    }
}

TEST_F(BranchingTest, ConditionalCallAndReturnRoundTripReturnAddress) {
    Compare(0x01, 0x01);
    cpu.SetRegister("SP", 0x1000);
    Load({0xCC, 0x20, 0x00}); // CZ 0020H

    auto state = cpu.Step();
    EXPECT_EQ(state.pc, 0x0020);
    EXPECT_EQ(state.sp, 0x0FFE);
    EXPECT_EQ(memory.Read(0x0FFE), 0x03);
    EXPECT_EQ(memory.Read(0x0FFF), 0x00);

    memory.Write(0x20, 0xC9); // RET
    state = cpu.Step();
    EXPECT_EQ(state.pc, 0x0003);
    EXPECT_EQ(state.sp, 0x1000);
}

TEST_F(BranchingTest, ConditionalCallDoesNotPushWhenConditionFails) {
    Compare(0x01, 0x00);
    cpu.SetRegister("SP", 0x1000);
    Load({0xCC, 0x20, 0x00}); // CZ is not taken

    const auto state = cpu.Step();

    EXPECT_EQ(state.pc, 0x0003);
    EXPECT_EQ(state.sp, 0x1000);
}

TEST_F(BranchingTest, PushAndPopPreserveRegisterPair) {
    cpu.SetRegister("B", 0x12);
    cpu.SetRegister("C", 0x34);
    cpu.SetRegister("SP", 0x2000);
    Load({0xC5, 0x01, 0x00, 0x00, 0xC1}); // PUSH B; LXI B,0000H; POP B

    cpu.Step();
    EXPECT_EQ(memory.Read(0x1FFE), 0x34);
    EXPECT_EQ(memory.Read(0x1FFF), 0x12);
    cpu.Step();
    cpu.Step();
    const auto state = cpu.Step();

    EXPECT_EQ(state.b, 0x12);
    EXPECT_EQ(state.c, 0x34);
    EXPECT_EQ(state.sp, 0x2000);
}

TEST_F(BranchingTest, PushPopPswRoundTripsAccumulatorAndFlags) {
    cpu.SetRegister("A", 0xA5);
    Load({0x37, 0xF5, 0x3E, 0x00, 0xF1}); // STC; PUSH PSW; MVI A,00H; POP PSW

    cpu.Step();
    cpu.Step();
    cpu.Step();
    const auto state = cpu.Step();

    EXPECT_EQ(state.a, 0xA5);
    EXPECT_TRUE(state.carry);
    EXPECT_EQ(memory.Read(0xFFFD), 0x03);
}

TEST_F(BranchingTest, ConditionalReturnUsesTheZeroFlag) {
    Compare(0x01, 0x01);
    cpu.SetRegister("SP", 0x3000);
    memory.Write(0x3000, 0x56);
    memory.Write(0x3001, 0x34);
    Load({0xC8}); // RZ

    const auto state = cpu.Step();

    EXPECT_EQ(state.pc, 0x3456);
    EXPECT_EQ(state.sp, 0x3002);
}

TEST_F(BranchingTest, PchlSphlAndXthlUseTheStackPointerAndHlPair) {
    cpu.SetRegister("H", 0x12);
    cpu.SetRegister("L", 0x34);
    Load({0xE9}); // PCHL
    EXPECT_EQ(cpu.Step().pc, 0x1234);

    cpu.Reset();
    cpu.SetRegister("H", 0xAB);
    cpu.SetRegister("L", 0xCD);
    cpu.SetRegister("SP", 0x2000);
    memory.Write(0, 0xE3); // XTHL
    memory.Write(0x2000, 0x78);
    memory.Write(0x2001, 0x56);
    cpu.Step();
    auto state = cpu.GetState();
    EXPECT_EQ(state.h, 0x56);
    EXPECT_EQ(state.l, 0x78);
    EXPECT_EQ(memory.Read(0x2000), 0xCD);
    EXPECT_EQ(memory.Read(0x2001), 0xAB);

    memory.Write(0, 0xF9); // SPHL
    cpu.SetRegister("PC", 0);
    cpu.Step();
    state = cpu.GetState();
    EXPECT_EQ(state.sp, 0x5678);
}

} // namespace
