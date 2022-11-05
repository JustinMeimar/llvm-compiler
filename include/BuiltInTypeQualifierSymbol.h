#pragma once

#include "Symbol.h"
#include "Type.h"

namespace gazprea {
    class BuiltInTypeQualifierSymbol : public Symbol, public Type {
    public:
        BuiltInTypeQualifierSymbol(std::string name);
        std::string getName();
    };
}
