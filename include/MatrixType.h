#pragma once

#include "Symbol.h"
#include "Type.h"
#include "AST.h"

namespace gazprea {
    class MatrixType : public Type {
    public:
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
        std::string getName() {
            return "matrix";
        }
        std::shared_ptr<Type> baseType;
        int dimension;  // 1 for Vector, 2 for 2D-matrix
        std::shared_ptr<AST> def;
        
        int getTypeId() {
            int baseTypeId = baseType->getTypeId();
            switch (baseTypeId) {
                case Type::BOOLEAN: {
                    if (dimension == 1) {
                        return Type::BOOLEAN_1;
                    }
                    return Type::BOOLEAN_2;
                }
                break;
                case Type::CHARACTER: {
                    if (dimension == 1) {
                        return Type::CHARACTER_1;
                    }
                    return Type::CHARACTER_2;
                }
                break;
                case Type::INTEGER: {
                    if (dimension == 1) {
                        return Type::INTEGER_1;
                    }
                    return Type::INTEGER_2;
                }
                break;
                case Type::REAL: {
                    if (dimension == 1) {
                        return Type::REAL_1;
                    }
                    return Type::REAL_2;
                }
                break;
            }
            return -1;
        }
    };
}
