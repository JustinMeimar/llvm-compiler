#include "SymbolTable.h"

namespace gazprea {
    void SymbolTable::initTypeSystem() {
        globals->defineTypeSymbol(std::make_shared<BuiltInScalarTypeSymbol>("integer"));
        globals->defineTypeSymbol(std::make_shared<BuiltInScalarTypeSymbol>("real"));
        globals->defineTypeSymbol(std::make_shared<BuiltInScalarTypeSymbol>("character"));
        globals->defineTypeSymbol(std::make_shared<BuiltInScalarTypeSymbol>("string"));
        globals->defineTypeSymbol(std::make_shared<BuiltInScalarTypeSymbol>("boolean"));
        globals->defineTypeSymbol(std::make_shared<BuiltInScalarTypeSymbol>("interval"));
        globals->defineTypeSymbol(std::make_shared<BuiltInScalarTypeSymbol>("gazprea.type.identityNull"));
    }

    SymbolTable::SymbolTable() : globals(std::make_shared<GlobalScope>()), numTupleIdentifierAccess(0) { 
        initTypeSystem(); 
    }

    std::shared_ptr<Type> SymbolTable::getType(size_t typeEnum) {

        auto booleanBaseType    = std::dynamic_pointer_cast<Type>(this->globals->resolveTypeSymbol("boolean"));
        auto integerBaseType    = std::dynamic_pointer_cast<Type>(this->globals->resolveTypeSymbol("integer"));
        auto realBaseType       = std::dynamic_pointer_cast<Type>(this->globals->resolveTypeSymbol("real"));
        auto characterBaseType  = std::dynamic_pointer_cast<Type>(this->globals->resolveTypeSymbol("character"));
        auto stringBaseType     = std::dynamic_pointer_cast<Type>(this->globals->resolveTypeSymbol("string"));
        auto intervalBaseType   = std::dynamic_pointer_cast<Type>(this->globals->resolveTypeSymbol("interval"));
        auto identityNullBaseType = std::dynamic_pointer_cast<Type>(this->globals->resolveTypeSymbol("gazprea.type.identityNull"));

        switch(typeEnum) {
            case 0:  return nullptr; break;
            case 1:  return intervalBaseType; break;
            case 2:  return booleanBaseType; break;
            case 3:  return characterBaseType; break;
            case 4:  return integerBaseType; break;
            case 5:  return realBaseType; break;
            case 6:  return stringBaseType; break;
            case 7:  return std::dynamic_pointer_cast<Type>(std::make_shared<IntervalType>(integerBaseType)); break;
            case 8:  return std::dynamic_pointer_cast<Type>(std::make_shared<MatrixType>(booleanBaseType, 1)); break;
            case 9:  return std::dynamic_pointer_cast<Type>(std::make_shared<MatrixType>(characterBaseType, 1)); break;
            case 10: return std::dynamic_pointer_cast<Type>(std::make_shared<MatrixType>(integerBaseType, 1)); break;
            case 11: return std::dynamic_pointer_cast<Type>(std::make_shared<MatrixType>(realBaseType, 1)); break;
            case 12: return std::dynamic_pointer_cast<Type>(std::make_shared<MatrixType>(booleanBaseType, 2)); break;
            case 13: return std::dynamic_pointer_cast<Type>(std::make_shared<MatrixType>(characterBaseType, 2)); break;
            case 14: return std::dynamic_pointer_cast<Type>(std::make_shared<MatrixType>(integerBaseType, 2)); break;
            case 15: return std::dynamic_pointer_cast<Type>(std::make_shared<MatrixType>(realBaseType, 2)); break;
            case 16: return identityNullBaseType; break;
        }
        return nullptr;
    }

    std::string SymbolTable::toString() {
        return globals->toString();
    }
}

