#pragma once
#include "GazpreaBaseVisitor.h"
#include "AST.h"
#include "SymbolTable.h"
#include "Symbol.h"
#include "VariableSymbol.h"
#include "TypedefTypeSymbol.h"
#include "LocalScope.h"
#include "SubroutineSymbol.h"
#include "TupleType.h"
#include "exceptions.h"

namespace gazprea {

class DefWalk {
    private:
        std::shared_ptr<SymbolTable> symtab;
        std::shared_ptr<Scope> currentScope;
        std::shared_ptr<SubroutineSymbol> currentSubroutineScope;
    public:
        bool hasMainProcedure;
        int numLoopAncestors;
        int numSubroutineAncestors;
        std::vector<std::string> reservedSbrtNames = {
            "length",
            "columns",
            "rows", 
            "reverse",
            "stream_state"
        };
        DefWalk(std::shared_ptr<SymbolTable> symtab);
        ~DefWalk();

        void visit(std::shared_ptr<AST> t); 
        void visitChildren(std::shared_ptr<AST> t);
        
        void visitSubroutineDeclDef(std::shared_ptr<AST> t);
        
        void visitTypedefStatement(std::shared_ptr<AST> t);  
        
        void visitBlock(std::shared_ptr<AST> t);        

        void visitIdentifier(std::shared_ptr<AST> t);

        void visitVariableDeclaration(std::shared_ptr<AST> t);

        void visitParameterAtom(std::shared_ptr<AST> t);

        void visitTupleType(std::shared_ptr<AST> t);

        void visitBreak(std::shared_ptr<AST> t);
        
        void visitContinue(std::shared_ptr<AST> t);

        void visitReturn(std::shared_ptr<AST> t);

        void visitTupleAccess(std::shared_ptr<AST> t);

        void visitDomainExpression(std::shared_ptr<AST> t);

        void visitIteratorLoop(std::shared_ptr<AST> t);

        void vistInfiniteLoop(std::shared_ptr<AST> t);

        void visitPrePredicatedLoop(std::shared_ptr<AST> t);
        
        void visitPostPredicatedLoop(std::shared_ptr<AST> t);

        void visitConditionalStatement(std::shared_ptr<AST> t);

        void visitGenerator(std::shared_ptr<AST> t);

        void visitFilter(std::shared_ptr<AST> t);
};

} // namespace gazrepa