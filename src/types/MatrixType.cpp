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
    
    std::string MatrixType::getName() {
        std::string strBaseType = "";
            switch(baseType->getTypeId()) {
                case Type::BOOLEAN:
                case Type::BOOLEAN_1:
                    strBaseType = "boolean"; 
                    break;
                case Type::CHARACTER:
                case Type::CHARACTER_1:
                    strBaseType = "character"; 
                    break;
                case Type::INTEGER:
                case Type::INTEGER_1:
                    strBaseType = "integer"; 
                    break;
                case Type::REAL:
                case Type::REAL_1:
                    strBaseType = "real"; 
                    break;
        } 
        if (dimension == 1) { 
            return strBaseType + " vector";
        }
        return strBaseType + " matrix";
    }

}