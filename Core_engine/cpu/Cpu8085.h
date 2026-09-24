#pragma once

#include "../memory/Memory.h"

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <set>
#include <string>

namespace simulator {

struct CpuState {
    uint8_t a = 0;
    uint8_t b = 0;
    uint8_t c = 0;
    uint8_t d = 0;
    uint8_t e = 0;
    uint8_t h = 0;
    uint8_t l = 0;
    uint16_t pc = 0;
    uint16_t sp = 0xFFFF;
    bool sign = false;
    bool zero = false;
    bool auxiliaryCarry = false;
    bool parity = false;
    bool carry = false;
    bool halted = false;
    bool interruptEnabled = false;
    uint8_t interruptMask = 0;
    bool serialInput = false;
    bool serialOutput = false;
};

class Cpu8085 {
public:
    explicit Cpu8085(Memory& memory);

    CpuState Step();
    void Reset();
    void AddBreakpoint(uint16_t address);
    void SetRegister(const std::string& name, uint16_t value);
    CpuState GetState() const;
    void SetIoHandlers(std::function<uint8_t(uint8_t)> inputHandler,
        std::function<void(uint8_t, uint8_t)> outputHandler);

private:
    void Execute(uint8_t opcode);
    uint8_t FetchByte();
    uint16_t FetchWord();
    uint16_t GetPair(uint8_t pair) const;
    void SetPair(uint8_t pair, uint16_t value);
    uint8_t ReadRegister(uint8_t code) const;
    void WriteRegister(uint8_t code, uint8_t value);
    uint8_t ReadOperand(uint8_t code) const;
    void WriteOperand(uint8_t code, uint8_t value);
    void UpdateFlags(uint8_t result);
    void UpdateFlagsArithmetic(uint8_t reg, uint8_t operand, uint16_t result, bool subtraction);
    void Push(uint16_t value);
    uint16_t Pop();
    bool ConditionMet(uint8_t condition) const;
    void Call(uint16_t address);
    uint8_t Input(uint8_t port);
    void Output(uint8_t port, uint8_t value);

    Memory& memory_;
    CpuState state_;
    std::set<uint16_t> breakpoints_;
    std::function<uint8_t(uint8_t)> inputHandler_;
    std::function<void(uint8_t, uint8_t)> outputHandler_;
    std::atomic<bool> isHalted_{false};
    mutable std::mutex mutex_;
};

} // namespace simulator