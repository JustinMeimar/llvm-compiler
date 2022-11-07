#pragma once
#include "GazpreaBaseVisitor.h"
#include "AST.h"
#include "SymbolTable.h"
#include "Symbol.h"
#include "VariableSymbol.h"
#include "TypedefTypeSymbol.h"
#include "LocalScope.h"
#include "SubroutineSymbol.h"

#include "MatrixType.h"
#include "IntervalType.h"
#include "TupleType.h"

namespace gazprea {

class RefWalk {
    private:
        std::shared_ptr<SymbolTable> symtab;
        std::shared_ptr<Scope> currentScope;
    public:
        RefWalk(std::shared_ptr<SymbolTable> symtab);
        ~RefWalk();

        void visit(std::shared_ptr<AST> t); 
        void visitChildren(std::shared_ptr<AST> t);
        
        void visitSubroutineDeclDef(std::shared_ptr<AST> t);
        
        void visitTypedefStatement(std::shared_ptr<AST> t);  
        
        void visitIdentifier(std::shared_ptr<AST> t);

        void visitVariableDeclaration(std::shared_ptr<AST> t);

        void visitVectorMatrixType(std::shared_ptr<AST> t);

        void visitSingleTokenType(std::shared_ptr<AST> t);

        void visitUnqualifiedType(std::shared_ptr<AST> t);
};

} // namespace gazprea