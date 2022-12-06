#include "SymbolTable.h"

namespace gazprea {
    void SymbolTable::initTypeSystem() {
        booleanType = std::make_shared<BuiltInScalarTypeSymbol>("boolean");
        characterType = std::make_shared<BuiltInScalarTypeSymbol>("character");
        integerType = std::make_shared<BuiltInScalarTypeSymbol>("integer");
        realType = std::make_shared<BuiltInScalarTypeSymbol>("real");
        stringType = std::make_shared<BuiltInScalarTypeSymbol>("string");
        intervalType = std::make_shared<BuiltInScalarTypeSymbol>("interval");
        identityNullType = std::make_shared<BuiltInScalarTypeSymbol>("gazprea.type.identityNull");

        integerIntervalType = std::make_shared<IntervalType>(integerType);
        booleanVectorType = std::make_shared<MatrixType>(booleanType, 1);
        characterVectorType = std::make_shared<MatrixType>(characterType, 1);
        integerVectorType = std::make_shared<MatrixType>(integerType, 1);
        realVectorType = std::make_shared<MatrixType>(realType, 1);
        booleanMatrixType = std::make_shared<MatrixType>(booleanType, 2);
        characterMatrixType = std::make_shared<MatrixType>(characterType, 2);
        integerMatrixType = std::make_shared<MatrixType>(integerType, 2);
        realMatrixType = std::make_shared<MatrixType>(realType, 2);

        globals->defineTypeSymbol(std::dynamic_pointer_cast<BuiltInScalarTypeSymbol>(booleanType));
        globals->defineTypeSymbol(std::dynamic_pointer_cast<BuiltInScalarTypeSymbol>(characterType));
        globals->defineTypeSymbol(std::dynamic_pointer_cast<BuiltInScalarTypeSymbol>(integerType));
        globals->defineTypeSymbol(std::dynamic_pointer_cast<BuiltInScalarTypeSymbol>(realType));
        globals->defineTypeSymbol(std::dynamic_pointer_cast<BuiltInScalarTypeSymbol>(stringType));
        globals->defineTypeSymbol(std::dynamic_pointer_cast<BuiltInScalarTypeSymbol>(intervalType));
        globals->defineTypeSymbol(std::dynamic_pointer_cast<BuiltInScalarTypeSymbol>(identityNullType));
    }

    SymbolTable::SymbolTable() : globals(std::make_shared<GlobalScope>()), numTupleIdentifierAccess(0) { 
        initTypeSystem();
    }

    std::shared_ptr<Type> SymbolTable::getType(size_t typeEnum) {
        switch(typeEnum) {
            case 0:  return nullptr; break;
            case 1:  return intervalType; break;
            
            case 2:  return booleanType; break;
            case 3:  return characterType; break;
            case 4:  return integerType; break;
            case 5:  return realType; break;
            
            case 6:  return stringType; break;
            case 7:  return integerIntervalType; break;
            
            case 8:  return booleanVectorType; break;
            case 9:  return characterVectorType; break;
            case 10: return integerVectorType; break;
            case 11: return realVectorType; break;
            
            case 12: return booleanVectorType; break;
            case 13: return characterVectorType; break;
            case 14: return integerVectorType; break;
            case 15: return realVectorType; break;
            
            case 16: return identityNullType; break;
        }
        return nullptr;
    }

    std::string SymbolTable::toString() {
        return globals->toString();
    }
}

