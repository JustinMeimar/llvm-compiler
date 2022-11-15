#pragma once
#include "GazpreaBaseVisitor.h"
#include "GazpreaParser.h"
#include "AST.h"
#include "SymbolTable.h"
#include "Symbol.h"
#include "VariableSymbol.h"
#include "SubroutineSymbol.h"
#include "LocalScope.h"

#include "TypePromote.h"
#include "MatrixType.h"
#include "TupleType.h"

#include "Exceptions.h"

namespace gazprea {

class TypeWalk {
    private:
        std::shared_ptr<SymbolTable> symtab;
        std::shared_ptr<Scope> currentScope;
        std::shared_ptr<TypePromote> tp;

    public:
        TypeWalk(std::shared_ptr<SymbolTable> symtab);
        ~TypeWalk();

        void visit(std::shared_ptr<AST> t); 
        void visitChildren(std::shared_ptr<AST> t);

        //Top level exprs
        void visitIndex(std::shared_ptr<AST> t);
        void visitFilter(std::shared_ptr<AST> t);
        void visitGenerator(std::shared_ptr<AST> t);
        void visitCast(std::shared_ptr<AST> t);
        void visitExpression(std::shared_ptr<AST> t);
        void visitTupleAccess(std::shared_ptr<AST> t);
        void visitStringConcat(std::shared_ptr<AST> t);
        void visitVariableDeclaration(std::shared_ptr<AST> t);
        void visitAssignment(std::shared_ptr<AST> t);
        void visitCallInExpr(std::shared_ptr<AST> t);

        //Operations        
        void visitBinaryOp(std::shared_ptr<AST> t);
        void visitUnaryOp(std::shared_ptr<AST> t);

        //Compound Types
        void visitVectorLiteral(std::shared_ptr<AST> t); //9,10,11,12 
        void visitTupleLiteral(std::shared_ptr<AST> t); //0
        void visitIntervalLiteral(std::shared_ptr<AST> t);

        //Terminal Types
        void visitIntegerConstant(std::shared_ptr<AST> t);
        void visitRealConstant(std::shared_ptr<AST> t);
        void visitCharacterConstant(std::shared_ptr<AST> t);
        void visitBooleanConstant(std::shared_ptr<AST> t);
        void visitStringLiteral(std::shared_ptr<AST> t);
        void visitIdentifier(std::shared_ptr<AST> t);
        void visitIdentityOrNull(std::shared_ptr<AST> t);

        //Other Statements
        void visitStreamStatement(std::shared_ptr<AST> t);
        void visitTypedefStatement(std::shared_ptr<AST> t);
};

} // namespace gazprea