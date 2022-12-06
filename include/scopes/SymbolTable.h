#pragma once

#include <map>
#include <string>
#include <memory>

#include "Scope.h"
#include "Symbol.h"
#include "GlobalScope.h"
#include "BuiltInScalarTypeSymbol.h"
#include "Type.h"
#include "MatrixType.h"
#include "TupleType.h"
#include "IntervalType.h"

namespace gazprea {
    class SymbolTable { // single-scope symtab
    protected:
        void initTypeSystem();
    public:	
        std::shared_ptr<GlobalScope> globals;
        SymbolTable();

        std::shared_ptr<Type> getType(size_t typeEnum);
        std::string toString();
        std::map<std::string, int> tupleIdentifierAccess;
        int numTupleIdentifierAccess;

        std::shared_ptr<Type> booleanType;
        std::shared_ptr<Type> characterType;
        std::shared_ptr<Type> integerType;
        std::shared_ptr<Type> realType;
        std::shared_ptr<Type> integerIntervalType;
        std::shared_ptr<Type> booleanVectorType;
        std::shared_ptr<Type> characterVectorType;
        std::shared_ptr<Type> integerVectorType;
        std::shared_ptr<Type> realVectorType;
        std::shared_ptr<Type> booleanMatrixType;
        std::shared_ptr<Type> characterMatrixType;
        std::shared_ptr<Type> integerMatrixType;
        std::shared_ptr<Type> realMatrixType;
        std::shared_ptr<Type> stringType;
        std::shared_ptr<Type> intervalType;
        std::shared_ptr<Type> identityNullType;
    };
}