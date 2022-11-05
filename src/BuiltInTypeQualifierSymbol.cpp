#include "BuiltInTypeQualifierSymbol.h"

namespace gazprea {
    BuiltInTypeQualifierSymbol::BuiltInTypeQualifierSymbol(std::string name) : Symbol(name) {}

    std::string BuiltInTypeQualifierSymbol::getName() {
        return Symbol::getName();
    }
}
