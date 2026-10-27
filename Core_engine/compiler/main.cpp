#include "Assembler.h"

#include "CodeGen.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace simulator {

std::vector<uint8_t> Assembler::Compile(const std::string& source) const {
    const Lexer lexer;
    const Parser parser;
    const CodeGen codeGen;

    const auto tokenizedLines = lexer.Tokenize(source);
    std::vector<Instruction> instructions;
    instructions.reserve(tokenizedLines.size());
    for (const auto& tokens : tokenizedLines) instructions.push_back(parser.ParseLine(tokens));

    std::vector<uint8_t> machineCode;
    for (const auto& instruction : instructions) {
        auto bytes = codeGen.Encode(instruction);
        machineCode.insert(machineCode.end(), bytes.begin(), bytes.end());
    }
    return machineCode;
}

} // namespace simulator