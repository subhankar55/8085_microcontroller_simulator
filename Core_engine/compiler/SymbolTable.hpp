#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

namespace simulator {

class SymbolTable {
public:
    void Define(const std::string& label, uint16_t address);
    bool Find(const std::string& label, uint16_t& address) const;

private:
    std::unordered_map<std::string, uint16_t> labels_;
};

} // namespace simulator