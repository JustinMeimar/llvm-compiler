#pragma once

#include "Symbol.h"
#include "Type.h"
#include "AST.h"

namespace gazprea {
    class MatrixType : public Type {
    public:
        std::shared_ptr<Type> baseType;
        int dimension;  // 1 for Vector, 2 for 2D-matrix
        std::shared_ptr<AST> def;

        MatrixType(std::shared_ptr<Type> baseType, int dimension);
        MatrixType(std::shared_ptr<Type> baseType, int dimension, std::shared_ptr<AST> def);
        
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
            return false;
        }
        bool isIdentityType() {
            return false;
        }
        std::string getName() {
            return "matrix";
        }
        int getTypeId();
    };
}
