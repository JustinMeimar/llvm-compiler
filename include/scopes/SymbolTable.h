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
    };
}