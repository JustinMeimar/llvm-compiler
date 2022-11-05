#pragma once

#include "Symbol.h"
#include "Type.h"

namespace gazprea {
    class BuiltInScalarTypeSymbol : public Symbol, public Type {
    public:
        BuiltInScalarTypeSymbol(std::string name);
        std::string getName();
    };
}

