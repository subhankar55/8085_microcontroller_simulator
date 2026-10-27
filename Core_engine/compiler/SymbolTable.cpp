#include "SymbolTable.hpp"

#include <stdexcept>
#include <string>

namespace simulator {

void SymbolTable::Define(const std::string& label, uint16_t address) {
    const auto result = labels_.emplace(label, address);
    if (!result.second) throw std::invalid_argument("Duplicate label: " + label);
}

bool SymbolTable::Find(const std::string& label, uint16_t& address) const {
    const auto entry = labels_.find(label);
    if (entry == labels_.end()) return false;
    address = entry->second;
    return true;
}

} // namespace simulator