#include "MatrixType.h"

namespace gazprea {
    
    MatrixType::MatrixType(std::shared_ptr<Type> baseType, int dimension) : baseType(baseType), dimension(dimension) {}
    MatrixType::MatrixType(std::shared_ptr<Type> baseType, int dimension, std::shared_ptr<AST> def) : baseType(baseType), dimension(dimension), def(def) {}
    
    int MatrixType::getTypeId() {
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
            case Type::STRING:
                return Type::STRING;
            break;
        }
        return -1;
    }
}