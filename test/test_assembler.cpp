#include "compiler/Assembler.h"
#include "compiler/SymbolTable.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

class AssemblerTest : public testing::Test {
protected:
    simulator::Assembler assembler;

    void ExpectEncoding(const std::string& source, std::vector<uint8_t> expected) {
        EXPECT_EQ(assembler.Compile(source), expected) << source;
    }
};

TEST_F(AssemblerTest, TokenizesCommentsCaseAndOperandSeparators) {
    ExpectEncoding("  mvi a, 0x2A ; comment\n\nLXI H, 1234H\n",
        {0x3E, 0x2A, 0x21, 0x34, 0x12});
}

TEST_F(AssemblerTest, EncodesAllZeroOperandInstructions) {
    ExpectEncoding(
        "NOP\nHLT\nEI\nDI\nRIM\nSIM\nXCHG\nPCHL\nSPHL\nXTHL\nDAA\nCMA\nCMC\nSTC\nRLC\nRRC\nRAL\nRAR\nRET",
        {0x00, 0x76, 0xFB, 0xF3, 0x20, 0x30, 0xEB, 0xE9, 0xF9, 0xE3,
            0x27, 0x2F, 0x3F, 0x37, 0x07, 0x0F, 0x17, 0x1F, 0xC9});
}

TEST_F(AssemblerTest, EncodesRegisterAndMemoryTransferInstructions) {
    ExpectEncoding(
        "MOV B,C\nMOV M,A\nMVI M,255\nLXI SP,4660\nLXI BC,1\nLXI DE,2\nLXI HL,3\n"
        "LDA 1234H\nSTA 0X1234\n"
        "LHLD 1234H\nSHLD 1234H\nLDAX B\nLDAX D\nSTAX B\nSTAX D\nXCHG",
        {0x41, 0x77, 0x36, 0xFF, 0x31, 0x34, 0x12,
            0x01, 1, 0, 0x11, 2, 0, 0x21, 3, 0,
            0x3A, 0x34, 0x12, 0x32, 0x34, 0x12, 0x2A, 0x34, 0x12,
            0x22, 0x34, 0x12, 0x0A, 0x1A, 0x02, 0x12, 0xEB});
}

TEST_F(AssemblerTest, EncodesArithmeticLogicalAndRegisterPairInstructions) {
    ExpectEncoding(
        "ADD M\nADC A\nSUB B\nSBB C\nANA D\nXRA E\nORA H\nCMP L\n"
        "ADI 1\nACI 2\nSUI 3\nSBI 4\nANI 5\nXRI 6\nORI 7\nCPI 8\n"
        "INR A\nDCR M\nINX B\nDCX DE\nDAD SP",
        {0x86, 0x8F, 0x90, 0x99, 0xA2, 0xAB, 0xB4, 0xBD,
            0xC6, 1, 0xCE, 2, 0xD6, 3, 0xDE, 4, 0xE6, 5, 0xEE, 6, 0xF6, 7, 0xFE, 8,
            0x3C, 0x35, 0x03, 0x1B, 0x39});
}

TEST_F(AssemblerTest, EncodesDirectAndEveryConditionalBranchForm) {
    ExpectEncoding(
        "JMP 1234H\nCALL 5678H\nJ NZ,1234H\nJZ 1234H\nC C,1234H\nCNZ 1234H\n"
        "R M\nRM\nPUSH B\nPUSH DE\nPUSH HL\nPUSH PSW\nPOP B\nPOP DE\nPOP HL\nPOP PSW\n"
        "RST 7\nIN 10B\nOUT 0X10",
        {0xC3, 0x34, 0x12, 0xCD, 0x78, 0x56,
            0xC2, 0x34, 0x12, 0xCA, 0x34, 0x12, 0xDC, 0x34, 0x12, 0xC4, 0x34, 0x12,
            0xF8, 0xF8, 0xC5, 0xD5, 0xE5, 0xF5, 0xC1, 0xD1, 0xE1, 0xF1,
            0xFF, 0xDB, 2, 0xD3, 0x10});
}

TEST_F(AssemblerTest, EncodesAllConditionalMnemonicsAndNumericFormats) {
    constexpr const char* conditions[] = {"NZ", "Z", "NC", "C", "PO", "PE", "P", "M"};
    constexpr uint8_t jumpOpcodes[] = {0xC2, 0xCA, 0xD2, 0xDA, 0xE2, 0xEA, 0xF2, 0xFA};
    constexpr uint8_t callOpcodes[] = {0xC4, 0xCC, 0xD4, 0xDC, 0xE4, 0xEC, 0xF4, 0xFC};
    constexpr uint8_t returnOpcodes[] = {0xC0, 0xC8, 0xD0, 0xD8, 0xE0, 0xE8, 0xF0, 0xF8};

    for (std::size_t i = 0; i < 8; ++i) {
        ExpectEncoding(std::string("J ") + conditions[i] + ",1",
            {jumpOpcodes[i], 1, 0});
        ExpectEncoding(std::string("C ") + conditions[i] + ",1",
            {callOpcodes[i], 1, 0});
        ExpectEncoding(std::string("R ") + conditions[i],
            {returnOpcodes[i]});
    }

    ExpectEncoding("MVI A,0XFF\nMVI B,10H\nMVI C,11B\nLXI H,65535",
        {0x3E, 0xFF, 0x06, 0x10, 0x0E, 3, 0x21, 0xFF, 0xFF});
}

TEST_F(AssemblerTest, EncodesEveryFixedConditionalJumpCallAndReturnMnemonic) {
    constexpr const char* conditions[] = {"NZ", "Z", "NC", "C", "PO", "PE", "P", "M"};
    constexpr uint8_t jumpOpcodes[] = {0xC2, 0xCA, 0xD2, 0xDA, 0xE2, 0xEA, 0xF2, 0xFA};
    constexpr uint8_t callOpcodes[] = {0xC4, 0xCC, 0xD4, 0xDC, 0xE4, 0xEC, 0xF4, 0xFC};
    constexpr uint8_t returnOpcodes[] = {0xC0, 0xC8, 0xD0, 0xD8, 0xE0, 0xE8, 0xF0, 0xF8};

    for (std::size_t i = 0; i < 8; ++i) {
        ExpectEncoding(std::string("J") + conditions[i] + " 1",
            {jumpOpcodes[i], 1, 0});
        ExpectEncoding(std::string("C") + conditions[i] + " 1",
            {callOpcodes[i], 1, 0});
        ExpectEncoding(std::string("R") + conditions[i],
            {returnOpcodes[i]});
    }
}

TEST_F(AssemblerTest, ReportsMalformedInstructionsAndOperands) {
    constexpr const char* invalidPrograms[] = {
        "UNKNOWN",
        "NOP A",
        "MOV A",
        "MOV M,M",
        "MOV A,X",
        "MOV X,A",
        "MVI X,1",
        "MVI A,H",
        "MVI A,256",
        "MVI A,12G",
        "LXI X,1",
        "LXI B,65536",
        "LDA",
        "LDAX H",
        "STAX SP",
        "ADD X",
        "ADI",
        "INR X",
        "DCX X",
        "JQ,1",
        "J NZ",
        "J NZ,1,2",
        "J NZ,H",
        "J Q,1",
        "RM X",
        "PUSH SP",
        "POP X",
        "RST 8",
        "IN 256",
        "MVI A,B"
    };

    for (const auto* program : invalidPrograms) {
        EXPECT_THROW(assembler.Compile(program), std::exception) << program;
    }
}

TEST(SymbolTableTest, DefinesFindsAndRejectsDuplicateLabels) {
    simulator::SymbolTable symbols;
    uint16_t address = 0xFFFF;

    EXPECT_FALSE(symbols.Find("START", address));
    EXPECT_EQ(address, 0xFFFF);
    symbols.Define("START", 0x1234);
    EXPECT_TRUE(symbols.Find("START", address));
    EXPECT_EQ(address, 0x1234);
    EXPECT_THROW(symbols.Define("START", 0x4321), std::invalid_argument);
    EXPECT_FALSE(symbols.Find("MISSING", address));
}

} // namespace
