#pragma once

#include "Token.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace simulator {

class Parser {
public:
    Instruction ParseLine(const std::vector<Token>& tokens) const;
    static void Require(const std::vector<std::string>& tokens, std::size_t count, const std::string& mnemonic);
};

} // namespace simulator