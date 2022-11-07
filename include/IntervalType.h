#pragma once

#include "Symbol.h"
#include "Type.h"
#include "AST.h"

namespace gazprea {
    class IntervalType : public Type {
    public:
        IntervalType(std::shared_ptr<Type> baseType, std::shared_ptr<Type> intervalType);
        bool isTypedefType() {
            return false;
        }
        bool isMatrixType() {
            return false;
        }
        bool isIntervalType() {
            return true;
        }
        bool isTupleType() {
            return false;
        }
        std::string getName() {
            return "interval";
        }
        std::shared_ptr<Type> baseType;
        std::shared_ptr<Type> intervalType;  // Should always be BuiltInScalarTypeSymbol("interval")
    };
}
