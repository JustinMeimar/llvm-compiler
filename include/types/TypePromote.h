#pragma once

#include <iostream>
#include "Type.h"
#include "BuiltInScalarTypeSymbol.h"
#include "AST.h"

#include "MatrixType.h"
#include "IntervalType.h"
#include "SymbolTable.h"
#include "Exceptions.h"

namespace gazprea {

class TypePromote {
    public:
        std::shared_ptr<SymbolTable> symtab;
        static int logicalResultType[16][16];
        static int arithmeticResultType[16][16];
        static int relationalResultType[16][16];
        static int equalityResultType[16][16];
        static int promotionFromTo[17][17];
        static int concatenationResultType[16][16];

        TypePromote(std::shared_ptr<SymbolTable> symtab);
        ~TypePromote();
        std::shared_ptr<Type> getResultType(
            int typetable[16][16], 
            std::shared_ptr<AST> lhs, 
            std::shared_ptr<AST> rhs,
            std::shared_ptr<AST> t      //pass parent to get line & char pos for error
        );
        std::shared_ptr<Type> getConcatenationResultType(
            int typetable[16][16], 
            std::shared_ptr<AST> lhs, 
            std::shared_ptr<AST> rhs,
            std::shared_ptr<AST> t      //pass parent to get line & char pos for error
        );
};

} //namespace gazprea