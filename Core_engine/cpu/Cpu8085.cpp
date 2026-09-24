#include "Cpu8085.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>
#include <utility>

namespace simulator {
namespace {

std::string Upper(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
        return static_cast<char>(std::toupper(character));
    });
    return value;
}

bool EvenParity(uint8_t value) {
    bool parity = true;
    while (value != 0) {
        parity = !parity;
        value = static_cast<uint8_t>(value & static_cast<uint8_t>(value - 1));
    }
    return parity;
}

}

Cpu8085::Cpu8085(Memory& memory) : memory_(memory) {}

CpuState Cpu8085::Step() {
    std::lock_guard lock(mutex_);
    if (isHalted_.load() || breakpoints_.find(state_.pc) != breakpoints_.end()) {
        return state_;
    }
    Execute(FetchByte());
    return state_;
}

uint8_t Cpu8085::FetchByte() {
    return memory_.Read(state_.pc++);
}

uint16_t Cpu8085::FetchWord() {
    const auto low = FetchByte();
    return static_cast<uint16_t>(low | (static_cast<uint16_t>(FetchByte()) << 8));
}

uint16_t Cpu8085::GetPair(uint8_t pair) const {
    switch (pair & 3) {
    case 0: return static_cast<uint16_t>((state_.b << 8) | state_.c);
    case 1: return static_cast<uint16_t>((state_.d << 8) | state_.e);
    case 2: return static_cast<uint16_t>((state_.h << 8) | state_.l);
    default: return state_.sp;
    }
}

void Cpu8085::SetPair(uint8_t pair, uint16_t value) {
    switch (pair & 3) {
    case 0: state_.b = static_cast<uint8_t>(value >> 8); state_.c = static_cast<uint8_t>(value); break;
    case 1: state_.d = static_cast<uint8_t>(value >> 8); state_.e = static_cast<uint8_t>(value); break;
    case 2: state_.h = static_cast<uint8_t>(value >> 8); state_.l = static_cast<uint8_t>(value); break;
    default: state_.sp = value; break;
    }
}

uint8_t Cpu8085::ReadRegister(uint8_t code) const {
    switch (code & 7) {
    case 0: return state_.b;
    case 1: return state_.c;
    case 2: return state_.d;
    case 3: return state_.e;
    case 4: return state_.h;
    case 5: return state_.l;
    case 7: return state_.a;
    default: return memory_.Read(static_cast<uint16_t>((state_.h << 8) | state_.l));
    }
}

void Cpu8085::WriteRegister(uint8_t code, uint8_t value) {
    switch (code & 7) {
    case 0: state_.b = value; break;
    case 1: state_.c = value; break;
    case 2: state_.d = value; break;
    case 3: state_.e = value; break;
    case 4: state_.h = value; break;
    case 5: state_.l = value; break;
    case 7: state_.a = value; break;
    default: memory_.Write(static_cast<uint16_t>((state_.h << 8) | state_.l), value); break;
    }
}

uint8_t Cpu8085::ReadOperand(uint8_t code) const {
    return ReadRegister(code);
}

void Cpu8085::WriteOperand(uint8_t code, uint8_t value) {
    WriteRegister(code, value);
}

void Cpu8085::UpdateFlags(uint8_t result) {
    state_.sign = (result & 0x80) != 0;
    state_.zero = result == 0;
    state_.parity = EvenParity(result);
}

void Cpu8085::UpdateFlagsArithmetic(uint8_t reg, uint8_t operand, uint16_t result, bool subtraction) {
    const auto value = static_cast<uint8_t>(result);
    UpdateFlags(value);
    if (subtraction) {
        state_.auxiliaryCarry = (reg & 0x0F) < (operand & 0x0F);
        state_.carry = reg < operand;
    } else {
        state_.auxiliaryCarry = ((reg & 0x0F) + (operand & 0x0F)) > 0x0F;
        state_.carry = result > 0xFF;
    }
}

void Cpu8085::Push(uint16_t value) {
    state_.sp = static_cast<uint16_t>(state_.sp - 2);
    memory_.Write(state_.sp, static_cast<uint8_t>(value >> 8));
    memory_.Write(static_cast<uint16_t>(state_.sp + 1), static_cast<uint8_t>(value));
}

uint16_t Cpu8085::Pop() {
    const auto value = static_cast<uint16_t>((memory_.Read(state_.sp) << 8) |
        memory_.Read(static_cast<uint16_t>(state_.sp + 1)));
    state_.sp = static_cast<uint16_t>(state_.sp + 2);
    return value;
}

bool Cpu8085::ConditionMet(uint8_t condition) const {
    switch (condition & 7) {
    case 0: return !state_.zero;
    case 1: return state_.zero;
    case 2: return !state_.carry;
    case 3: return state_.carry;
    case 4: return !state_.parity;
    case 5: return state_.parity;
    case 6: return !state_.sign;
    default: return state_.sign;
    }
}

void Cpu8085::Call(uint16_t address) {
    Push(state_.pc);
    state_.pc = address;
}

uint8_t Cpu8085::Input(uint8_t port) {
    return inputHandler_ ? inputHandler_(port) : 0;
}

void Cpu8085::Output(uint8_t port, uint8_t value) {
    if (outputHandler_) outputHandler_(port, value);
}

void Cpu8085::Execute(uint8_t opcode) {
    switch (opcode) {
    // Part 1: data transfer.
    case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47:
    case 0x48: case 0x49: case 0x4A: case 0x4B: case 0x4C: case 0x4D: case 0x4E: case 0x4F:
    case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57:
    case 0x58: case 0x59: case 0x5A: case 0x5B: case 0x5C: case 0x5D: case 0x5E: case 0x5F:
    case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x66: case 0x67:
    case 0x68: case 0x69: case 0x6A: case 0x6B: case 0x6C: case 0x6D: case 0x6E: case 0x6F:
    case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x77: case 0x78:
    case 0x79: case 0x7A: case 0x7B: case 0x7C: case 0x7D: case 0x7E: case 0x7F:
        WriteOperand((opcode >> 3) & 7, ReadOperand(opcode & 7));
        break;
    case 0x06: case 0x0E: case 0x16: case 0x1E: case 0x26: case 0x2E: case 0x36: case 0x3E:
        WriteOperand((opcode >> 3) & 7, FetchByte());
        break;
    case 0x01: case 0x11: case 0x21: case 0x31:
        SetPair((opcode >> 4) & 3, FetchWord());
        break;
    case 0x3A: state_.a = memory_.Read(FetchWord()); break;
    case 0x32: memory_.Write(FetchWord(), state_.a); break;
    case 0x2A: { const auto address = FetchWord(); state_.l = memory_.Read(address); state_.h = memory_.Read(static_cast<uint16_t>(address + 1)); break; }
    case 0x22: { const auto address = FetchWord(); memory_.Write(address, state_.l); memory_.Write(static_cast<uint16_t>(address + 1), state_.h); break; }
    case 0x0A: state_.a = memory_.Read(GetPair(0)); break;
    case 0x1A: state_.a = memory_.Read(GetPair(1)); break;
    case 0x02: memory_.Write(GetPair(0), state_.a); break;
    case 0x12: memory_.Write(GetPair(1), state_.a); break;
    case 0xEB: std::swap(state_.d, state_.h); std::swap(state_.e, state_.l); break;

    // Part 2: arithmetic.
    case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x86: case 0x87:
    case 0x88: case 0x89: case 0x8A: case 0x8B: case 0x8C: case 0x8D: case 0x8E: case 0x8F: {
        const auto operand = static_cast<uint8_t>(ReadOperand(opcode & 7) + ((opcode & 8) != 0 && state_.carry));
        const auto result = static_cast<uint16_t>(state_.a + operand);
        UpdateFlagsArithmetic(state_.a, operand, result, false);
        state_.a = static_cast<uint8_t>(result);
        break;
    }
    case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97:
    case 0x98: case 0x99: case 0x9A: case 0x9B: case 0x9C: case 0x9D: case 0x9E: case 0x9F: {
        const auto operand = static_cast<uint8_t>(ReadOperand(opcode & 7) + ((opcode & 8) != 0 && state_.carry));
        const auto result = static_cast<uint8_t>(state_.a - operand);
        UpdateFlagsArithmetic(state_.a, operand, result, true);
        state_.a = result;
        break;
    }
    case 0xC6: case 0xCE: { const auto operand = static_cast<uint8_t>(FetchByte() + ((opcode & 8) != 0 && state_.carry)); const auto result = static_cast<uint16_t>(state_.a + operand); UpdateFlagsArithmetic(state_.a, operand, result, false); state_.a = static_cast<uint8_t>(result); break; }
    case 0xD6: case 0xDE: { const auto operand = static_cast<uint8_t>(FetchByte() + ((opcode & 8) != 0 && state_.carry)); const auto result = static_cast<uint8_t>(state_.a - operand); UpdateFlagsArithmetic(state_.a, operand, result, true); state_.a = result; break; }
    case 0x04: case 0x0C: case 0x14: case 0x1C: case 0x24: case 0x2C: case 0x34: case 0x3C: { const auto oldCarry = state_.carry; const auto old = ReadOperand((opcode >> 3) & 7); const auto result = static_cast<uint8_t>(old + 1); WriteOperand((opcode >> 3) & 7, result); UpdateFlagsArithmetic(old, 1, static_cast<uint16_t>(old + 1), false); state_.carry = oldCarry; break; }
    case 0x05: case 0x0D: case 0x15: case 0x1D: case 0x25: case 0x2D: case 0x35: case 0x3D: { const auto oldCarry = state_.carry; const auto old = ReadOperand((opcode >> 3) & 7); const auto result = static_cast<uint8_t>(old - 1); WriteOperand((opcode >> 3) & 7, result); UpdateFlagsArithmetic(old, 1, result, true); state_.carry = oldCarry; break; }
    case 0x03: case 0x13: case 0x23: case 0x33: SetPair((opcode >> 4) & 3, static_cast<uint16_t>(GetPair((opcode >> 4) & 3) + 1)); break;
    case 0x0B: case 0x1B: case 0x2B: case 0x3B: SetPair((opcode >> 4) & 3, static_cast<uint16_t>(GetPair((opcode >> 4) & 3) - 1)); break;
    case 0x09: case 0x19: case 0x29: case 0x39: { const auto left = GetPair(2); const auto right = GetPair((opcode >> 4) & 3); const auto result = static_cast<uint32_t>(left) + right; SetPair(2, static_cast<uint16_t>(result)); state_.carry = result > 0xFFFF; break; }
    case 0x27: { uint16_t correction = 0; if ((state_.a & 0x0F) > 9 || state_.auxiliaryCarry) correction |= 0x06; if (state_.a > 0x99 || state_.carry || (state_.a & 0xF0) > 0x90) correction |= 0x60; const auto result = static_cast<uint16_t>(state_.a + correction); state_.carry = state_.carry || result > 0xFF; state_.a = static_cast<uint8_t>(result); UpdateFlags(state_.a); break; }

    // Part 3: logical.
    case 0xA0: case 0xA1: case 0xA2: case 0xA3: case 0xA4: case 0xA5: case 0xA6: case 0xA7: state_.a &= ReadOperand(opcode & 7); state_.auxiliaryCarry = true; state_.carry = false; UpdateFlags(state_.a); break;
    case 0xB0: case 0xB1: case 0xB2: case 0xB3: case 0xB4: case 0xB5: case 0xB6: case 0xB7: state_.a |= ReadOperand(opcode & 7); state_.auxiliaryCarry = false; state_.carry = false; UpdateFlags(state_.a); break;
    case 0xA8: case 0xA9: case 0xAA: case 0xAB: case 0xAC: case 0xAD: case 0xAE: case 0xAF: state_.a ^= ReadOperand(opcode & 7); state_.auxiliaryCarry = false; state_.carry = false; UpdateFlags(state_.a); break;
    case 0xB8: case 0xB9: case 0xBA: case 0xBB: case 0xBC: case 0xBD: case 0xBE: case 0xBF: { const auto operand = ReadOperand(opcode & 7); const auto result = static_cast<uint8_t>(state_.a - operand); UpdateFlagsArithmetic(state_.a, operand, result, true); break; }
    case 0xE6: case 0xEE: case 0xF6: { const auto operand = FetchByte(); if (opcode == 0xE6) state_.a &= operand; else if (opcode == 0xF6) state_.a |= operand; else state_.a ^= operand; state_.auxiliaryCarry = opcode == 0xE6; state_.carry = false; UpdateFlags(state_.a); break; }
    case 0xFE: { const auto operand = FetchByte(); const auto result = static_cast<uint8_t>(state_.a - operand); UpdateFlagsArithmetic(state_.a, operand, result, true); break; }
    case 0x07: { const auto bit = static_cast<uint8_t>((state_.a >> 7) & 1); state_.a = static_cast<uint8_t>((state_.a << 1) | bit); state_.carry = bit != 0; break; }
    case 0x0F: { const auto bit = static_cast<uint8_t>(state_.a & 1); state_.a = static_cast<uint8_t>((state_.a >> 1) | (bit ? 0x80 : 0)); state_.carry = bit != 0; break; }
    case 0x17: { const auto oldCarry = static_cast<uint8_t>(state_.carry); state_.carry = (state_.a & 0x80) != 0; state_.a = static_cast<uint8_t>((state_.a << 1) | oldCarry); break; }
    case 0x1F: { const auto oldCarry = static_cast<uint8_t>(state_.carry); state_.carry = (state_.a & 1) != 0; state_.a = static_cast<uint8_t>((state_.a >> 1) | (oldCarry ? 0x80 : 0)); break; }
    case 0x2F: state_.a = static_cast<uint8_t>(~state_.a); break;
    case 0x3F: state_.carry = !state_.carry; break;
    case 0x37: state_.carry = true; break;

    // Part 4: branching and stack.
    case 0xC3: state_.pc = FetchWord(); break;
    case 0xCA: case 0xC2: case 0xDA: case 0xD2: case 0xF2: case 0xFA: case 0xEA: case 0xE2: { const auto address = FetchWord(); if (ConditionMet((opcode >> 3) & 7)) state_.pc = address; break; }
    case 0xCD: Call(FetchWord()); break;
    case 0xC4: case 0xCC: case 0xD4: case 0xDC: case 0xF4: case 0xFC: case 0xEC: case 0xE4: { const auto address = FetchWord(); if (ConditionMet((opcode >> 3) & 7)) Call(address); break; }
    case 0xC9: state_.pc = Pop(); break;
    case 0xC0: case 0xC8: case 0xD0: case 0xD8: case 0xE0: case 0xE8: case 0xF0: case 0xF8: if (ConditionMet((opcode >> 3) & 7)) state_.pc = Pop(); break;
    case 0xC5: case 0xD5: case 0xE5: Push(GetPair((opcode >> 4) & 3)); break;
    case 0xF5: { const auto flags = static_cast<uint8_t>((state_.sign ? 0x80 : 0) | (state_.zero ? 0x40 : 0) | (state_.auxiliaryCarry ? 0x10 : 0) | (state_.parity ? 0x04 : 0) | 0x02 | (state_.carry ? 1 : 0)); Push(static_cast<uint16_t>((state_.a << 8) | flags)); break; }
    case 0xC1: case 0xD1: case 0xE1: SetPair((opcode >> 4) & 3, Pop()); break;
    case 0xF1: { const auto flags = Pop(); state_.a = static_cast<uint8_t>(flags >> 8); const auto value = static_cast<uint8_t>(flags); state_.sign = value & 0x80; state_.zero = value & 0x40; state_.auxiliaryCarry = value & 0x10; state_.parity = value & 0x04; state_.carry = value & 1; break; }
    case 0xE9: state_.pc = GetPair(2); break;
    case 0xF9: state_.sp = GetPair(2); break;
    case 0xE3: { const auto oldL = memory_.Read(state_.sp); const auto oldH = memory_.Read(static_cast<uint16_t>(state_.sp + 1)); memory_.Write(state_.sp, state_.l); memory_.Write(static_cast<uint16_t>(state_.sp + 1), state_.h); state_.l = oldL; state_.h = oldH; break; }
    case 0xC7: case 0xCF: case 0xD7: case 0xDF: case 0xE7: case 0xEF: case 0xF7: case 0xFF: Call(static_cast<uint16_t>(opcode & 0x38)); break;

    // Part 5: machine control and I/O.
    case 0x00: break;
    case 0x76: isHalted_.store(true); state_.halted = true; break;
    case 0xDB: state_.a = Input(FetchByte()); break;
    case 0xD3: { const auto port = FetchByte(); Output(port, state_.a); break; }
    case 0xFB: state_.interruptEnabled = true; break;
    case 0xF3: state_.interruptEnabled = false; break;
    case 0x20: state_.a = static_cast<uint8_t>((state_.a & 0x0F) | (state_.interruptMask << 4) | (state_.serialInput ? 0x40 : 0) | (state_.serialOutput ? 0x80 : 0)); break;
    case 0x30: state_.interruptMask = static_cast<uint8_t>((state_.a >> 4) & 7); state_.serialOutput = (state_.a & 0x40) != 0; break;
    default: throw std::runtime_error("Invalid 8085 opcode: 0x" + std::to_string(opcode));
    }
}

void Cpu8085::Reset() {
    std::lock_guard lock(mutex_);
    state_ = CpuState{};
    isHalted_.store(false);
}

void Cpu8085::AddBreakpoint(uint16_t address) {
    std::lock_guard lock(mutex_);
    breakpoints_.insert(address);
}

void Cpu8085::SetRegister(const std::string& name, uint16_t value) {
    std::lock_guard lock(mutex_);
    const auto registerName = Upper(name);
    if (registerName == "A") state_.a = static_cast<uint8_t>(value);
    else if (registerName == "B") state_.b = static_cast<uint8_t>(value);
    else if (registerName == "C") state_.c = static_cast<uint8_t>(value);
    else if (registerName == "D") state_.d = static_cast<uint8_t>(value);
    else if (registerName == "E") state_.e = static_cast<uint8_t>(value);
    else if (registerName == "H") state_.h = static_cast<uint8_t>(value);
    else if (registerName == "L") state_.l = static_cast<uint8_t>(value);
    else if (registerName == "PC") state_.pc = value;
    else if (registerName == "SP") state_.sp = value;
    else throw std::invalid_argument("Unknown register: " + name);
}

void Cpu8085::SetIoHandlers(std::function<uint8_t(uint8_t)> inputHandler, std::function<void(uint8_t, uint8_t)> outputHandler) {
    std::lock_guard lock(mutex_);
    inputHandler_ = std::move(inputHandler);
    outputHandler_ = std::move(outputHandler);
}

CpuState Cpu8085::GetState() const {
    std::lock_guard lock(mutex_);
    return state_;
}

} // namespace simulator
