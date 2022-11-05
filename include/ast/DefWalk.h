#pragma once
#include "GazpreaBaseVisitor.h"
#include "AST.h"
#include "SymbolTable.h"
#include "Symbol.h"
#include "VariableSymbol.h"
#include "TypedefTypeSymbol.h"
#include "LocalScope.h"
#include "SubroutineSymbol.h"

namespace gazprea {

class DefWalk {
    private:
        std::shared_ptr<SymbolTable> symtab;
        std::shared_ptr<Scope> currentScope;
    public:
        DefWalk(std::shared_ptr<SymbolTable> symtab);
        ~DefWalk();

        void visit(std::shared_ptr<AST> t); 
        void visitChildren(std::shared_ptr<AST> t);
        
        void visitSubroutineDeclDef(std::shared_ptr<AST> t);
        
        void visitTypedefStatement(std::shared_ptr<AST> t);  
        
        void visitBlock(std::shared_ptr<AST> t);        

        void visitIdentifier(std::shared_ptr<AST> t);

        void visitVariableDeclaration(std::shared_ptr<AST> t);
};

} // namespace gazrepa