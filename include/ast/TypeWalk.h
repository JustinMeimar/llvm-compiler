#pragma once
#include "GazpreaBaseVisitor.h"

#include "GazpreaParser.h"

#include "AST.h"
#include "SymbolTable.h"
#include "Symbol.h"
#include "LocalScope.h"

namespace gazprea {

class TypeWalk {
    private:
        std::shared_ptr<SymbolTable> symtab;
        std::shared_ptr<Scope> currentScope;
    public:
        TypeWalk (std::shared_ptr<SymbolTable> symtab);
        ~TypeWalk ();

        void visit(std::shared_ptr<AST> t); 
        void visitChildren(std::shared_ptr<AST> t);

        void visitExpression(std::shared_ptr<AST> t);
        void visitBinaryOp(std::shared_ptr<AST> t);

        void visitIntegerConstant(std::shared_ptr<AST> t);
        void visitRealConstant(std::shared_ptr<AST> t);

};

} // namespace gazprea