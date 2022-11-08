#pragma once
#include "GazpreaBaseVisitor.h"
#include "GazpreaParser.h"
#include "AST.h"
#include "SymbolTable.h"
#include "Symbol.h"
#include "LocalScope.h"
#include "TypePromote.h"

namespace gazprea {

class TypeWalk {
    private:
        std::shared_ptr<SymbolTable> symtab;
        std::shared_ptr<Scope> currentScope;
        std::shared_ptr<TypePromote> tp;

    public:
        TypeWalk (std::shared_ptr<SymbolTable> symtab);
        ~TypeWalk ();

        void visit(std::shared_ptr<AST> t); 
        void visitChildren(std::shared_ptr<AST> t);

        void visitSubroutineDeclDef(std::shared_ptr<AST> t);
        void visitBlock(std::shared_ptr<AST> t);
        void visitVariableDeclaration(std::shared_ptr<AST> t);
        void visitExpression(std::shared_ptr<AST> t);
        void visitBinaryOp(std::shared_ptr<AST> t);

        void visitIntegerConstant(std::shared_ptr<AST> t);
        void visitRealConstant(std::shared_ptr<AST> t);
        void visitIdentifier(std::shared_ptr<AST> t);
};

} // namespace gazprea