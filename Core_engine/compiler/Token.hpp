#pragma once

#include <string>
#include <vector>

namespace simulator {

enum class TokenKind {
    Word
};

struct Token {
    TokenKind kind;
    std::string value;
};

struct Instruction {
    std::vector<Token> tokens;
};

} // namespace simulator