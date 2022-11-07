#pragma once

#include "Symbol.h"
#include "Type.h"
#include "AST.h"

namespace gazprea {
    class TupleType : public Type {
    public:
        TupleType(std::vector<std::shared_ptr<Type>> listType, std::shared_ptr<AST> def);
        bool isTypedefType() {
            return false;
        }
        bool isMatrixType() {
            return true;
        }
        bool isIntervalType() {
            return false;
        }
        bool isTupleType() {
            return true;
        }
        std::string getName() {
            return "tuple";
        }
        std::vector<std::shared_ptr<Type>> listType;
        std::shared_ptr<AST> def;
    };
}
