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
        int getTypeId() {
            if (name == "boolean") {
                return Type::BOOLEAN;
            } else if (name == "chracter") {
                return Type::CHARACTER;
            } else if (name == "integer") {
                return Type::INTEGER;
            } else if (name == "real") {
                return Type::REAL;
            } else if (name == "interval") {
                return -1;  // will not be used
            }
            return Type::STRING;
        }
    };
}

