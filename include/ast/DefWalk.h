#pragma once
#include "GazpreaBaseVisitor.h"
#include "AST.h"

namespace gazprea {

class DefWalk {
    public:
        DefWalk();
        ~DefWalk();

        void visit(std::shared_ptr<AST> t); 
        void visitChildren(std::shared_ptr<AST> t);

        //statements
        void visitVarDeclarationStatement(std::shared_ptr<AST> t);

        void visitInferedType(std::shared_ptr<AST> t);

        void visitExplicitType(std::shared_ptr<AST> t);

        void visitTypeQualifier(std::shared_ptr<AST> t);

        void visitAssignmentStatement(std::shared_ptr<AST> t);    

        void visitConditionalStatement(std::shared_ptr<AST> t);      

        void visitInfiniteLoopStatement(std::shared_ptr<AST> t);    
        
        void visitPrePredicatedLoopStatement(std::shared_ptr<AST> t);      
        
        void visitPostPredicatedLoopStatement(std::shared_ptr<AST> t);     
 
        void visitBreakStatement(std::shared_ptr<AST> t);      
        
        void visitContinueStatement(std::shared_ptr<AST> t);
 
        void visitOutputStream(std::shared_ptr<AST> t);      
        
        void visitInputStream(std::shared_ptr<AST> t);      

        void visitProcedureDeclDef(std::shared_ptr<AST> t);
        
        void visitFunctionDeclDef(std::shared_ptr<AST> t);

        void visitFormalParamaterList(std::shared_ptr<AST> t);

        void visitUnqualifiedType(std::shared_ptr<AST> t);

        void visitRealType(std::shared_ptr<AST> t);
        
        void visitSubroutineBlockBody(std::shared_ptr<AST> t);
            
        void visitReturnStatement(std::shared_ptr<AST> t);  
        
        void visitCallProcedure(std::shared_ptr<AST> t); 

        void visitCallProcedureFunctionFromExpression(std::shared_ptr<AST> t);  
        
        void visitTypedefStatement(std::shared_ptr<AST> t);  
        
        void visitBlock(std::shared_ptr<AST> t);        

        void visitExpr(std::shared_ptr<AST> t);

        void visitIdentifier(std::shared_ptr<AST> t);

        void visitRealConstant(std::shared_ptr<AST> t);
};

} // namespace gazrepa