#include "TypedefTypeSymbol.h"

namespace gazprea {
    TypedefTypeSymbol::TypedefTypeSymbol(std::string name) : Symbol(name) {}

    std::string TypedefTypeSymbol::getName() {
        return Symbol::getName();
    }
}