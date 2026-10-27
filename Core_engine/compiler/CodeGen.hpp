#pragma once

#include "Token.hpp"

#include <cstdint>
#include <vector>

namespace simulator {

class CodeGen {
public:
    std::vector<uint8_t> Encode(const Instruction& instruction) const;
};

} // namespace simulator