#pragma once

#include "Token.hpp"

#include <string>
#include <vector>

namespace simulator {

class Lexer {
public:
    std::vector<std::vector<Token>> Tokenize(const std::string& source) const;
};

} // namespace simulator