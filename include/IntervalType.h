#pragma once

#include "Symbol.h"
#include "Type.h"
#include "AST.h"

namespace gazprea {
    class IntervalType : public Type {
    public:
        std::shared_ptr<Type> baseType;
        IntervalType(std::shared_ptr<Type> baseType);
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

        int getTypeId() {
            return Type::INTEGER_INTERVAL;
        }
    };
}
