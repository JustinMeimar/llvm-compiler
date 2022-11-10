#include "Type.h"
#include "BuiltInScalarTypeSymbol.h"
#include "AST.h"

#include "MatrixType.h"
#include "IntervalType.h"
#include "SymbolTable.h"

#include <iostream>

namespace gazprea {

class TypePromote {
    public:
        std::shared_ptr<SymbolTable> symtab;
        static int arithmeticResultType[16][16];
        static int relationalResultType[16][16];
        static int equalityResultType[16][16];
        static int promotionFromTo[16][16];

        TypePromote(std::shared_ptr<SymbolTable> symtab);
        ~TypePromote();
        std::shared_ptr<Type> getResultType(int typetable[16][16], std::shared_ptr<AST> lhs, std::shared_ptr<AST> rhs);
};

} //namespace gazprea