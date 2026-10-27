#include "Lexer.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>

namespace simulator {
namespace {

std::string Upper(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
        return static_cast<char>(std::toupper(character));
    });
    return value;
}

} // namespace

std::vector<std::vector<Token>> Lexer::Tokenize(const std::string& source) const {
    std::vector<std::vector<Token>> lines;
    std::istringstream input(source);
    std::string line;

    while (std::getline(input, line)) {
        const auto comment = line.find(';');
        if (comment != std::string::npos) line.erase(comment);
        for (auto& character : line) {
            if (character == ',') character = ' ';
        }

        std::istringstream stream(line);
        std::vector<Token> tokens;
        std::string token;
        while (stream >> token) tokens.push_back({TokenKind::Word, Upper(token)});
        if (!tokens.empty()) lines.push_back(std::move(tokens));
    }

    return lines;
}

} // namespace simulator