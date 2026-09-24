#include "Assembler.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>
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

std::vector<std::string> Tokens(std::string line) {
    for (auto& character : line) {
        if (character == ',') character = ' ';
    }
    std::istringstream stream(line);
    std::vector<std::string> tokens;
    std::string token;
    while (stream >> token) tokens.push_back(Upper(token));
    return tokens;
}

void Require(const std::vector<std::string>& tokens, std::size_t count, const std::string& mnemonic) {
    if (tokens.size() != count) {
        throw std::invalid_argument(mnemonic + " expects " + std::to_string(count - 1) + " operand(s)");
    }
}

uint32_t ParseNumber(const std::string& token, uint32_t maximum) {
    std::string number = token;
    int base = 10;
    if (number.size() > 2 && number[0] == '0' && number[1] == 'X') {
        number.erase(0, 2);
        base = 16;
    } else if (!number.empty() && number.back() == 'H') {
        number.pop_back();
        base = 16;
    } else if (!number.empty() && number.back() == 'B') {
        number.pop_back();
        base = 2;
    }

    if (number.empty()) throw std::invalid_argument("Invalid numeric operand: " + token);
    std::size_t offset = 0;
    const auto value = std::stoul(number, &offset, base);
    if (offset != number.size() || value > maximum) {
        throw std::invalid_argument("Numeric operand out of range: " + token);
    }
    return static_cast<uint32_t>(value);
}

uint8_t ParseByte(const std::string& token) {
    return static_cast<uint8_t>(ParseNumber(token, 0xFF));
}

uint16_t ParseWord(const std::string& token) {
    return static_cast<uint16_t>(ParseNumber(token, 0xFFFF));
}

int RegisterCode(const std::string& name) {
    static const char* names[] = {"B", "C", "D", "E", "H", "L", "M", "A"};
    for (int code = 0; code < 8; ++code) {
        if (name == names[code]) return code;
    }
    return -1;
}

int PairCode(const std::string& name) {
    if (name == "B" || name == "BC") return 0;
    if (name == "D" || name == "DE") return 1;
    if (name == "H" || name == "HL") return 2;
    if (name == "SP") return 3;
    return -1;
}

int ConditionCode(const std::string& name) {
    if (name == "NZ") return 0;
    if (name == "Z") return 1;
    if (name == "NC") return 2;
    if (name == "C") return 3;
    if (name == "PO") return 4;
    if (name == "PE") return 5;
    if (name == "P") return 6;
    if (name == "M") return 7;
    return -1;
}

void AppendWord(std::vector<uint8_t>& output, uint16_t value) {
    output.push_back(static_cast<uint8_t>(value));
    output.push_back(static_cast<uint8_t>(value >> 8));
}

uint8_t ConditionalOpcode(const std::string& mnemonic, int condition) {
    static const uint8_t jumpBases[] = {0xC2, 0xCA, 0xD2, 0xDA, 0xE2, 0xEA, 0xF2, 0xFA};
    static const uint8_t callBases[] = {0xC4, 0xCC, 0xD4, 0xDC, 0xE4, 0xEC, 0xF4, 0xFC};
    static const uint8_t returnBases[] = {0xC0, 0xC8, 0xD0, 0xD8, 0xE0, 0xE8, 0xF0, 0xF8};
    if (mnemonic == "J") return jumpBases[condition];
    if (mnemonic == "C") return callBases[condition];
    return returnBases[condition];
}

}

std::vector<uint8_t> Assembler::Compile(const std::string& source) const {
    std::vector<uint8_t> machineCode;
    std::string line;
    std::istringstream input(source);

    while (std::getline(input, line)) {
        const auto comment = line.find(';');
        if (comment != std::string::npos) line.erase(comment);
        auto tokens = Tokens(line);
        if (tokens.empty()) continue;

        const auto& mnemonic = tokens[0];
        const auto unaryRegister = RegisterCode(tokens.size() > 1 ? tokens[1] : "");
        const auto pair = PairCode(tokens.size() > 1 ? tokens[1] : "");

        if (mnemonic == "NOP") { Require(tokens, 1, mnemonic); machineCode.push_back(0x00); }
        else if (mnemonic == "HLT") { Require(tokens, 1, mnemonic); machineCode.push_back(0x76); }
        else if (mnemonic == "EI") { Require(tokens, 1, mnemonic); machineCode.push_back(0xFB); }
        else if (mnemonic == "DI") { Require(tokens, 1, mnemonic); machineCode.push_back(0xF3); }
        else if (mnemonic == "RIM") { Require(tokens, 1, mnemonic); machineCode.push_back(0x20); }
        else if (mnemonic == "SIM") { Require(tokens, 1, mnemonic); machineCode.push_back(0x30); }
        else if (mnemonic == "XCHG") { Require(tokens, 1, mnemonic); machineCode.push_back(0xEB); }
        else if (mnemonic == "PCHL") { Require(tokens, 1, mnemonic); machineCode.push_back(0xE9); }
        else if (mnemonic == "SPHL") { Require(tokens, 1, mnemonic); machineCode.push_back(0xF9); }
        else if (mnemonic == "XTHL") { Require(tokens, 1, mnemonic); machineCode.push_back(0xE3); }
        else if (mnemonic == "DAA") { Require(tokens, 1, mnemonic); machineCode.push_back(0x27); }
        else if (mnemonic == "CMA") { Require(tokens, 1, mnemonic); machineCode.push_back(0x2F); }
        else if (mnemonic == "CMC") { Require(tokens, 1, mnemonic); machineCode.push_back(0x3F); }
        else if (mnemonic == "STC") { Require(tokens, 1, mnemonic); machineCode.push_back(0x37); }
        else if (mnemonic == "RLC") { Require(tokens, 1, mnemonic); machineCode.push_back(0x07); }
        else if (mnemonic == "RRC") { Require(tokens, 1, mnemonic); machineCode.push_back(0x0F); }
        else if (mnemonic == "RAL") { Require(tokens, 1, mnemonic); machineCode.push_back(0x17); }
        else if (mnemonic == "RAR") { Require(tokens, 1, mnemonic); machineCode.push_back(0x1F); }
        else if (mnemonic == "MOV") {
            Require(tokens, 3, mnemonic);
            const auto destination = RegisterCode(tokens[1]);
            const auto sourceRegister = RegisterCode(tokens[2]);
            if (destination < 0 || sourceRegister < 0 || (destination == 6 && sourceRegister == 6)) throw std::invalid_argument("Invalid MOV operands");
            machineCode.push_back(static_cast<uint8_t>(0x40 | (destination << 3) | sourceRegister));
        } else if (mnemonic == "MVI") {
            Require(tokens, 3, mnemonic);
            if (unaryRegister < 0) throw std::invalid_argument("Invalid MVI register");
            machineCode.push_back(static_cast<uint8_t>(0x06 | (unaryRegister << 3)));
            machineCode.push_back(ParseByte(tokens[2]));
        } else if (mnemonic == "LXI") {
            Require(tokens, 3, mnemonic);
            if (pair < 0) throw std::invalid_argument("Invalid LXI register pair");
            machineCode.push_back(static_cast<uint8_t>(0x01 | (pair << 4)));
            AppendWord(machineCode, ParseWord(tokens[2]));
        } else if (mnemonic == "LDA" || mnemonic == "STA" || mnemonic == "LHLD" || mnemonic == "SHLD") {
            Require(tokens, 2, mnemonic);
            const uint8_t opcodes[] = {0x3A, 0x32, 0x2A, 0x22};
            const int index = mnemonic == "LDA" ? 0 : mnemonic == "STA" ? 1 : mnemonic == "LHLD" ? 2 : 3;
            machineCode.push_back(opcodes[index]);
            AppendWord(machineCode, ParseWord(tokens[1]));
        } else if (mnemonic == "LDAX" || mnemonic == "STAX") {
            Require(tokens, 2, mnemonic);
            if (pair != 0 && pair != 1) throw std::invalid_argument(mnemonic + " only accepts B or D");
            machineCode.push_back(static_cast<uint8_t>((mnemonic == "LDAX" ? 0x0A : 0x02) | (pair << 4)));
        } else if (mnemonic == "ADD" || mnemonic == "ADC" || mnemonic == "SUB" || mnemonic == "SBB" || mnemonic == "ANA" || mnemonic == "XRA" || mnemonic == "ORA" || mnemonic == "CMP") {
            Require(tokens, 2, mnemonic);
            if (unaryRegister < 0) throw std::invalid_argument("Invalid " + mnemonic + " register");
            const uint8_t bases[] = {0x80, 0x88, 0x90, 0x98, 0xA0, 0xA8, 0xB0, 0xB8};
            const std::string names[] = {"ADD", "ADC", "SUB", "SBB", "ANA", "XRA", "ORA", "CMP"};
            int index = 0; while (mnemonic != names[index]) ++index;
            machineCode.push_back(static_cast<uint8_t>(bases[index] | unaryRegister));
        } else if (mnemonic == "ADI" || mnemonic == "ACI" || mnemonic == "SUI" || mnemonic == "SBI" || mnemonic == "ANI" || mnemonic == "XRI" || mnemonic == "ORI" || mnemonic == "CPI") {
            Require(tokens, 2, mnemonic);
            const uint8_t opcodes[] = {0xC6, 0xCE, 0xD6, 0xDE, 0xE6, 0xEE, 0xF6, 0xFE};
            const std::string names[] = {"ADI", "ACI", "SUI", "SBI", "ANI", "XRI", "ORI", "CPI"};
            int index = 0; while (mnemonic != names[index]) ++index;
            machineCode.push_back(opcodes[index]);
            machineCode.push_back(ParseByte(tokens[1]));
        } else if (mnemonic == "INR" || mnemonic == "DCR") {
            Require(tokens, 2, mnemonic);
            if (unaryRegister < 0) throw std::invalid_argument("Invalid " + mnemonic + " register");
            machineCode.push_back(static_cast<uint8_t>((mnemonic == "INR" ? 0x04 : 0x05) | (unaryRegister << 3)));
        } else if (mnemonic == "INX" || mnemonic == "DCX" || mnemonic == "DAD") {
            Require(tokens, 2, mnemonic);
            if (pair < 0) throw std::invalid_argument("Invalid " + mnemonic + " register pair");
            const uint8_t bases[] = {0x03, 0x0B, 0x09};
            const int index = mnemonic == "INX" ? 0 : mnemonic == "DCX" ? 1 : 2;
            machineCode.push_back(static_cast<uint8_t>(bases[index] | (pair << 4)));
        } else if (mnemonic == "JMP" || mnemonic == "CALL") {
            Require(tokens, 2, mnemonic);
            machineCode.push_back(mnemonic == "JMP" ? 0xC3 : 0xCD);
            AppendWord(machineCode, ParseWord(tokens[1]));
        } else if (mnemonic == "RET") {
            Require(tokens, 1, mnemonic); machineCode.push_back(0xC9);
        } else if (mnemonic == "J" || mnemonic == "C" || mnemonic == "R" ||
            mnemonic == "JNZ" || mnemonic == "JZ" || mnemonic == "JNC" || mnemonic == "JC" ||
            mnemonic == "JPO" || mnemonic == "JPE" || mnemonic == "JP" || mnemonic == "JM" ||
            mnemonic == "CNZ" || mnemonic == "CZ" || mnemonic == "CNC" || mnemonic == "CC" ||
            mnemonic == "CPO" || mnemonic == "CPE" || mnemonic == "CP" || mnemonic == "CM" ||
            mnemonic == "RNZ" || mnemonic == "RZ" || mnemonic == "RNC" || mnemonic == "RC" ||
            mnemonic == "RPO" || mnemonic == "RPE" || mnemonic == "RP" || mnemonic == "RM") {
            const bool splitCondition = mnemonic == "J" || mnemonic == "C" || mnemonic == "R";
            const bool isReturn = (!splitCondition && mnemonic[0] == 'R') || mnemonic == "R";
            Require(tokens, splitCondition ? (isReturn ? 2 : 3) : 2, mnemonic);
            const auto conditionName = splitCondition ? tokens[1] : mnemonic.substr(1);
            const auto condition = ConditionCode(conditionName);
            if (condition < 0) throw std::invalid_argument("Invalid condition: " + tokens[1]);
            const auto kind = splitCondition ? mnemonic : std::string(1, mnemonic[0]);
            machineCode.push_back(ConditionalOpcode(kind, condition));
            if (!isReturn) AppendWord(machineCode, ParseWord(splitCondition ? tokens[2] : tokens[1]));
        } else if (mnemonic == "PUSH" || mnemonic == "POP") {
            Require(tokens, 2, mnemonic);
            const auto name = tokens[1];
            const uint8_t bases[] = {static_cast<uint8_t>(mnemonic == "PUSH" ? 0xC5 : 0xC1), static_cast<uint8_t>(mnemonic == "PUSH" ? 0xD5 : 0xD1), static_cast<uint8_t>(mnemonic == "PUSH" ? 0xE5 : 0xE1)};
            if (name == "PSW") machineCode.push_back(mnemonic == "PUSH" ? 0xF5 : 0xF1);
            else if (pair >= 0 && pair < 3) machineCode.push_back(bases[pair]);
            else throw std::invalid_argument("Invalid " + mnemonic + " register pair");
        } else if (mnemonic == "RST") {
            Require(tokens, 2, mnemonic);
            const auto vector = ParseNumber(tokens[1], 7);
            machineCode.push_back(static_cast<uint8_t>(0xC7 | (vector << 3)));
        } else if (mnemonic == "IN" || mnemonic == "OUT") {
            Require(tokens, 2, mnemonic);
            machineCode.push_back(mnemonic == "IN" ? 0xDB : 0xD3);
            machineCode.push_back(ParseByte(tokens[1]));
        } else {
            throw std::invalid_argument("Unsupported mnemonic: " + mnemonic);
        }
    }

    return machineCode;
}

} // namespace simulator
