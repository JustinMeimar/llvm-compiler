#pragma once

#include "Symbol.h"
#include "Type.h"

namespace gazprea {
    class TypedefTypeSymbol : public Symbol, public Type {
    public:
        TypedefTypeSymbol(std::string name);
        std::string getName();
    };
}

