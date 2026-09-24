#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace simulator {

class Assembler {
public:
    std::vector<uint8_t> Compile(const std::string& source) const;
};

} // namespace simulator