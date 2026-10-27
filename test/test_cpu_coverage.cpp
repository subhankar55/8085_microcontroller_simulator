#include "cpu/Cpu8085.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <iomanip>
#include <stdexcept>
#include <string>

namespace {

bool IsImplementedOpcode(uint8_t opcode) {
    if (opcode >= 0x40 && opcode <= 0xBF) {
        return true;
    }
    if (opcode <= 0x3F) {
        return opcode != 0x08 && opcode != 0x10 && opcode != 0x18 &&
            opcode != 0x28 && opcode != 0x38;
    }
    return opcode != 0xCB && opcode != 0xD9 && opcode != 0xDD &&
        opcode != 0xED && opcode != 0xFD;
}

class CpuOpcodeCoverageTest : public testing::Test {
protected:
    simulator::Memory memory;
    simulator::Cpu8085 cpu{memory};

    void Prepare(uint8_t opcode) {
        cpu.Reset();
        memory.Reset();
        cpu.SetRegister("SP", 0x8000);
        cpu.SetRegister("H", 0x10);
        cpu.SetRegister("L", 0x00);
        cpu.SetRegister("A", 0x55);
        cpu.SetRegister("B", 0x33);
        cpu.SetRegister("C", 0x22);
        cpu.SetRegister("D", 0x11);
        cpu.SetRegister("E", 0x44);
        memory.Write(0, opcode);
        memory.Write(1, 0x00);
        memory.Write(2, 0x20);
        memory.Write(0x1000, 0x66);
        memory.Write(0x8000, 0x00);
        memory.Write(0x8001, 0x00);
    }
};

TEST_F(CpuOpcodeCoverageTest, ExecutesEveryImplementedOpcodeAndRejectsReservedOpcodes) {
    for (unsigned int value = 0; value <= 0xFF; ++value) {
        const auto opcode = static_cast<uint8_t>(value);
        Prepare(opcode);

        if (IsImplementedOpcode(opcode)) {
            EXPECT_NO_THROW(cpu.Step()) << "opcode 0x" << std::hex << value;
        } else {
            try {
                (void)cpu.Step();
                FAIL() << "reserved opcode 0x" << std::hex << value << " was accepted";
            } catch (const std::runtime_error& error) {
                EXPECT_NE(std::string(error.what()).find("Invalid 8085 opcode"), std::string::npos);
            }
        }
    }
}

TEST_F(CpuOpcodeCoverageTest, CoversConditionalTransferTakenAndNotTakenPaths) {
    struct ConditionCase {
        uint8_t opcode;
        uint8_t left;
        uint8_t right;
        bool taken;
    };
    constexpr ConditionCase conditions[] = {
        {0xC2, 1, 1, false}, {0xC2, 1, 0, true},
        {0xCA, 1, 0, false}, {0xCA, 1, 1, true},
        {0xD2, 0, 1, false}, {0xD2, 1, 0, true},
        {0xDA, 1, 0, false}, {0xDA, 0, 1, true},
        {0xE2, 0, 0, false}, {0xE2, 1, 0, true},
        {0xEA, 1, 0, false}, {0xEA, 0, 0, true},
        {0xF2, 0, 1, false}, {0xF2, 1, 0, true},
        {0xFA, 1, 0, false}, {0xFA, 0, 1, true},
        {0xC4, 1, 1, false}, {0xC4, 1, 0, true},
        {0xCC, 1, 0, false}, {0xCC, 1, 1, true},
        {0xD4, 0, 1, false}, {0xD4, 1, 0, true},
        {0xDC, 1, 0, false}, {0xDC, 0, 1, true},
        {0xE4, 0, 0, false}, {0xE4, 1, 0, true},
        {0xEC, 1, 0, false}, {0xEC, 0, 0, true},
        {0xF4, 0, 1, false}, {0xF4, 1, 0, true},
        {0xFC, 1, 0, false}, {0xFC, 0, 1, true},
        {0xC0, 1, 0, true}, {0xC0, 1, 1, false},
        {0xC8, 1, 1, true}, {0xC8, 1, 0, false},
        {0xD0, 0, 1, false}, {0xD0, 1, 0, true},
        {0xD8, 1, 0, false}, {0xD8, 0, 1, true},
        {0xE0, 0, 0, false}, {0xE0, 1, 0, true},
        {0xE8, 1, 0, false}, {0xE8, 0, 0, true},
        {0xF0, 0, 1, false}, {0xF0, 1, 0, true},
        {0xF8, 1, 0, false}, {0xF8, 0, 1, true}
    };

    for (const auto& testCase : conditions) {
        Prepare(testCase.opcode);
        cpu.SetRegister("A", testCase.left);
        cpu.SetRegister("B", testCase.right);
        memory.Write(0, 0xB8); // CMP B
        cpu.Step();
        memory.Write(1, testCase.opcode);
        memory.Write(2, 0x34);
        memory.Write(3, 0x12);
        cpu.SetRegister("PC", 1);
        const auto state = cpu.Step();

        if (testCase.opcode == 0xC0 || testCase.opcode == 0xC8 ||
            testCase.opcode == 0xD0 || testCase.opcode == 0xD8 ||
            testCase.opcode == 0xE0 || testCase.opcode == 0xE8 ||
            testCase.opcode == 0xF0 || testCase.opcode == 0xF8) {
            EXPECT_EQ(state.pc, testCase.taken ? 0x0000 : 0x0002);
        } else {
            EXPECT_EQ(state.pc, testCase.taken ? 0x1234 : 0x0004);
        }
    }
}

TEST_F(CpuOpcodeCoverageTest, CoversControlStateBreakpointAndIoFallbacks) {
    Prepare(0x76);
    EXPECT_TRUE(cpu.Step().halted);
    EXPECT_TRUE(cpu.Step().halted);

    Prepare(0xDB);
    memory.Write(1, 0x42);
    EXPECT_EQ(cpu.Step().a, 0);

    uint8_t observedPort = 0;
    uint8_t observedValue = 0;
    cpu.Reset();
    Prepare(0xDB);
    memory.Write(1, 0x42);
    cpu.SetIoHandlers(
        [](uint8_t port) { return static_cast<uint8_t>(port + 1); },
        [&](uint8_t port, uint8_t value) { observedPort = port; observedValue = value; });
    EXPECT_EQ(cpu.Step().a, 0x43);

    cpu.Reset();
    Prepare(0xD3);
    memory.Write(1, 0x42);
    cpu.SetRegister("A", 0xA5);
    cpu.Step();
    EXPECT_EQ(observedPort, 0x42);
    EXPECT_EQ(observedValue, 0xA5);

    cpu.Reset();
    Prepare(0x20);
    cpu.SetRegister("A", 0xFF);
    memory.Write(0, 0x30); // SIM sets the mask and serial output.
    cpu.Step();
    memory.Write(1, 0x20); // RIM reads the stored mask and serial output.
    cpu.SetRegister("PC", 1);
    const auto state = cpu.Step();
    EXPECT_EQ(state.a, 0xFF);

    cpu.Reset();
    Prepare(0x00);
    cpu.AddBreakpoint(0);
    EXPECT_EQ(cpu.Step().pc, 0);
}

TEST_F(CpuOpcodeCoverageTest, CoversRegisterValidationAndReset) {
    for (const auto* name : {"a", "B", "c", "D", "e", "H", "l", "pc", "sp"}) {
        EXPECT_NO_THROW(cpu.SetRegister(name, 0x1234));
    }
    EXPECT_THROW(cpu.SetRegister("AF", 0), std::invalid_argument);

    cpu.SetRegister("A", 0x77);
    cpu.Reset();
    EXPECT_EQ(cpu.GetState().a, 0);
    EXPECT_FALSE(cpu.GetState().halted);
}

TEST_F(CpuOpcodeCoverageTest, CoversDecimalAdjustCorrectionBranches) {
    Prepare(0x27);
    cpu.SetRegister("A", 0x0A);
    cpu.Step();
    EXPECT_EQ(cpu.GetState().a, 0x10);

    cpu.Reset();
    Prepare(0x80);
    cpu.SetRegister("A", 0x08);
    cpu.SetRegister("B", 0x08);
    cpu.Step();
    memory.Write(1, 0x27);
    cpu.SetRegister("PC", 1);
    EXPECT_EQ(cpu.Step().a, 0x16);

    cpu.Reset();
    Prepare(0x27);
    cpu.SetRegister("A", 0xA0);
    cpu.Step();
    EXPECT_EQ(cpu.GetState().a, 0x00);
    EXPECT_TRUE(cpu.GetState().carry);

    cpu.Reset();
    Prepare(0x37);
    cpu.Step();
    memory.Write(1, 0x27);
    cpu.SetRegister("PC", 1);
    cpu.SetRegister("A", 0x00);
    EXPECT_TRUE(cpu.Step().carry);
}

TEST_F(CpuOpcodeCoverageTest, CoversImmediateCarryArithmeticAndRotationOutcomes) {
    Prepare(0x37);
    memory.Write(1, 0xCE); // STC; ACI 00H
    memory.Write(2, 0x00);
    cpu.SetRegister("A", 0xFF);
    cpu.Step();
    const auto adc = cpu.Step();
    EXPECT_EQ(adc.a, 0);
    EXPECT_TRUE(adc.carry);

    cpu.Reset();
    Prepare(0x37);
    memory.Write(1, 0xDE); // STC; SBI 00H
    memory.Write(2, 0x00);
    cpu.SetRegister("A", 0);
    cpu.Step();
    const auto sbb = cpu.Step();
    EXPECT_EQ(sbb.a, 0xFF);
    EXPECT_TRUE(sbb.carry);

    cpu.Reset();
    Prepare(0x0F);
    cpu.SetRegister("A", 0x54);
    EXPECT_FALSE(cpu.Step().carry);

    cpu.Reset();
    Prepare(0x37);
    memory.Write(1, 0x1F); // STC; RAR
    cpu.SetRegister("A", 0);
    cpu.Step();
    const auto rotate = cpu.Step();
    EXPECT_EQ(rotate.a, 0x80);
    EXPECT_FALSE(rotate.carry);
}

} // namespace
