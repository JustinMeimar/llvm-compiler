#pragma once

#include "Symbol.h"
#include "Type.h"

namespace gazprea {
    class BuiltInScalarTypeSymbol : public Symbol, public Type {
    public:
        BuiltInScalarTypeSymbol(std::string name);
        std::string getName();
        bool isTypedefType() {
            return false;
        }
        bool isMatrixType() {
            return false;
        }
        bool isIntervalType() {
            return false;
        }
        bool isTupleType() {
            return false;
        }
    };
}

