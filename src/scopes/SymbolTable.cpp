#include "SymbolTable.h"

namespace gazprea {
    void SymbolTable::initTypeSystem() {
        globals->define(std::make_shared<BuiltInScalarTypeSymbol>("integer"));
        globals->define(std::make_shared<BuiltInScalarTypeSymbol>("real"));
        globals->define(std::make_shared<BuiltInScalarTypeSymbol>("character"));
        globals->define(std::make_shared<BuiltInScalarTypeSymbol>("string"));
        globals->define(std::make_shared<BuiltInScalarTypeSymbol>("boolean"));
        globals->define(std::make_shared<BuiltInScalarTypeSymbol>("interval"));
    }

    SymbolTable::SymbolTable() : globals(std::make_shared<GlobalScope>()) { 
        initTypeSystem(); 
    }

    std::shared_ptr<Type> SymbolTable::getType(size_t typeEnum) {

        auto booleanBaseType    = std::dynamic_pointer_cast<Type>(this->globals->resolve("boolean"));
        auto integerBaseType    = std::dynamic_pointer_cast<Type>(this->globals->resolve("integer"));
        auto realBaseType       = std::dynamic_pointer_cast<Type>(this->globals->resolve("real"));
        auto characterBaseType  = std::dynamic_pointer_cast<Type>(this->globals->resolve("character"));
        auto stringBaseType     = std::dynamic_pointer_cast<Type>(this->globals->resolve("string"));
        auto intervalBaseType   = std::dynamic_pointer_cast<Type>(this->globals->resolve("interval"));

        switch(typeEnum) {
            case 0:  return nullptr; break;
            case 1:  return intervalBaseType; break;
            case 2:  return booleanBaseType; break;
            case 3:  return characterBaseType; break;
            case 4:  return integerBaseType; break;
            case 5:  return realBaseType; break;
            case 6:  return stringBaseType; break;
            case 7:  return nullptr; break;
            case 8:  return std::dynamic_pointer_cast<Type>(std::make_shared<MatrixType>(MatrixType(booleanBaseType, 1))); break;
            case 9:  return std::dynamic_pointer_cast<Type>(std::make_shared<MatrixType>(MatrixType(characterBaseType, 1))); break;
            case 10: return std::dynamic_pointer_cast<Type>(std::make_shared<MatrixType>(MatrixType(integerBaseType, 1))); break;
            case 11: return std::dynamic_pointer_cast<Type>(std::make_shared<MatrixType>(MatrixType(realBaseType, 1)));break;
            case 12: return std::dynamic_pointer_cast<Type>(std::make_shared<MatrixType>(MatrixType(booleanBaseType, 2))); break;
            case 13: return std::dynamic_pointer_cast<Type>(std::make_shared<MatrixType>(MatrixType(characterBaseType, 2))); break;
            case 14: return std::dynamic_pointer_cast<Type>(std::make_shared<MatrixType>(MatrixType(integerBaseType, 2))); break;
            case 15: return std::dynamic_pointer_cast<Type>(std::make_shared<MatrixType>(MatrixType(realBaseType, 2))); break;
        }
    }

    std::string SymbolTable::toString() {
        return globals->toString();
    }
}

