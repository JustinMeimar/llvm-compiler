#pragma once

#include "Symbol.h"
#include "Type.h"
#include "AST.h"

namespace gazprea {
    class IdentityNullType : public Type {
    public:
        std::shared_ptr<Type> baseType;
        IdentityNullType(std::shared_ptr<Type> baseType);
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
        bool isIdentityNullType() {
            return true;
        }
        std::string getName() {
            return "identityNull";
        }

        int getTypeId();
    };
}