#pragma once

#include "Symbol.h"
#include "Type.h"
#include "AST.h"
#include <memory>

namespace gazprea {
    class TypedefTypeSymbol : public Symbol, public Type {
    public:
        TypedefTypeSymbol(std::string name);
        std::string getName();
        void resolveTargetType();
        bool isTypedefType() {
            return true;
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

