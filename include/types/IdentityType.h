#pragma once

#include "Symbol.h"
#include "Type.h"
#include "AST.h"

namespace gazprea {
    class IdentityType : public Type {
    public:
        std::shared_ptr<Type> baseType;
        IdentityType(std::shared_ptr<Type> baseType);
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
        std::string getName() {
            return "identity";
        }

        int getTypeId();
    };
}