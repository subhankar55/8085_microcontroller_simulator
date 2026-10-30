#include "Assembler.h"

#include "CodeGen.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace simulator {

std::vector<uint8_t> Assembler::Compile(const std::string& source) const {
    const Lexer lexer;
    const Parser parser;
    const CodeGen codeGen;

    const auto tokenizedLines = lexer.Tokenize(source);
    std::unordered_map<std::string, uint16_t> labels;
    std::vector<std::vector<Token>> sourceLines;
    uint32_t address = 0;

    for (const auto& originalTokens : tokenizedLines) {
        auto tokens = originalTokens;
        if (!tokens.empty() && tokens.front().value.back() == ':') {
            const auto label = tokens.front().value.substr(0, tokens.front().value.size() - 1);
            if (label.empty() || labels.find(label) != labels.end()) {
                throw std::invalid_argument("Invalid or duplicate label: " + label);
            }
            if (address > 0xFFFF) throw std::invalid_argument("Program exceeds 64K address space");
            labels[label] = static_cast<uint16_t>(address);
            tokens.erase(tokens.begin());
        }
        if (tokens.empty()) continue;

        auto sizingTokens = tokens;
        const auto& mnemonic = sizingTokens.front().value;
        const bool branch = mnemonic == "JMP" || mnemonic == "CALL" ||
            mnemonic == "JNZ" || mnemonic == "JZ" || mnemonic == "JNC" ||
            mnemonic == "JC" || mnemonic == "JPO" || mnemonic == "JPE" ||
            mnemonic == "JP" || mnemonic == "JM";
        if (branch) {
            sizingTokens.back().value = "0";
        }
        const auto bytes = codeGen.Encode(parser.ParseLine(sizingTokens));
        address += static_cast<uint32_t>(bytes.size());
        sourceLines.push_back(std::move(tokens));
    }

    std::vector<uint8_t> machineCode;
    for (auto tokens : sourceLines) {
        for (std::size_t index = 1; index < tokens.size(); ++index) {
            const auto label = labels.find(tokens[index].value);
            if (label != labels.end()) {
                tokens[index].value = std::to_string(label->second);
            } else if (tokens[index].value.find_first_not_of("0123456789") != std::string::npos &&
                tokens[index].value != "H" && tokens[index].value != "L") {
                const auto& mnemonic = tokens.front().value;
                const bool branch = mnemonic == "JMP" || mnemonic == "CALL" ||
                    mnemonic == "JNZ" || mnemonic == "JZ" || mnemonic == "JNC" ||
                    mnemonic == "JC" || mnemonic == "JPO" || mnemonic == "JPE" ||
                    mnemonic == "JP" || mnemonic == "JM";
                if (branch) throw std::invalid_argument("Unknown label: " + tokens[index].value);
            }
        }
        auto bytes = codeGen.Encode(parser.ParseLine(tokens));
        machineCode.insert(machineCode.end(), bytes.begin(), bytes.end());
    }
    return machineCode;
}

} // namespace simulator