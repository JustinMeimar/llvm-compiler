#include "BuiltInScalarTypeSymbol.h"

namespace gazprea {
    BuiltInScalarTypeSymbol::BuiltInScalarTypeSymbol(std::string name) : Symbol(name) {}

    std::string BuiltInScalarTypeSymbol::getName() {
        return Symbol::getName();
    }
}

