#include "Parser.hpp"

#include <stdexcept>
#include <string>
#include <vector>

namespace simulator {

Instruction Parser::ParseLine(const std::vector<Token>& tokens) const {
    return {tokens};
}

void Parser::Require(const std::vector<std::string>& tokens, std::size_t count, const std::string& mnemonic) {
    if (tokens.size() != count) {
        throw std::invalid_argument(mnemonic + " expects " + std::to_string(count - 1) + " operand(s)");
    }
}

} // namespace simulator