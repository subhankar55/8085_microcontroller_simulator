#include "Memory.h"

#include <algorithm>
#include <stdexcept>

namespace simulator {

Memory::Memory() {
    Reset();
}

uint8_t Memory::Read(uint16_t address) const {
    std::lock_guard lock(mutex_);
    return bytes_[address];
}

void Memory::Write(uint16_t address, uint8_t value) {
    std::lock_guard lock(mutex_);
    bytes_[address] = value;
}

std::vector<uint8_t> Memory::ReadRange(uint16_t address, std::size_t length) const {
    if (length > kSize - address) {
        throw std::out_of_range("Memory read exceeds 64KB address space");
    }

    std::lock_guard lock(mutex_);
    return {bytes_.begin() + address, bytes_.begin() + address + length};
}

void Memory::WriteRange(uint16_t address, const uint8_t* data, std::size_t length) {
    if (length > kSize - address) {
        throw std::out_of_range("Memory write exceeds 64KB address space");
    }

    std::lock_guard lock(mutex_);
    std::copy_n(data, length, bytes_.begin() + address);
}

void Memory::Reset() {
    std::lock_guard lock(mutex_);
    bytes_.fill(0);
}

} // namespace simulator