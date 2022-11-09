#include "Type.h"
#include "BuiltInScalarTypeSymbol.h"
#include "AST.h"

#include <iostream>

namespace gazprea {

class TypePromote {
    public:
        
        static int arithmeticResultType[16][16];
        static int relationalResultType[16][16];
        static int equalityResultType[16][16];
        static int promotionFromTo[16][16];


        TypePromote();
        ~TypePromote();
        std::shared_ptr<Type> getResultType(int typetable[16][16], std::shared_ptr<AST> lhs, std::shared_ptr<AST> rhs);
};

} //namespace gazprea