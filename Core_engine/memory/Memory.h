#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <vector>

namespace simulator {

class Memory {
public:
    static constexpr std::size_t kSize = 0x10000;

    Memory();

    uint8_t Read(uint16_t address) const;
    void Write(uint16_t address, uint8_t value);
    std::vector<uint8_t> ReadRange(uint16_t address, std::size_t length) const;
    void WriteRange(uint16_t address, const uint8_t* data, std::size_t length);
    void Reset();

private:
    std::array<uint8_t, kSize> bytes_{};
    mutable std::mutex mutex_;
};

} // namespace simulator