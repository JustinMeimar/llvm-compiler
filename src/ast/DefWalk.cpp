#include "DefWalk.h"

namespace gazprea {

DefWalk::DefWalk() {}
DefWalk::~DefWalk() {}

void DefWalk::visit(std::shared_ptr<AST> t) {
    if(!t->isNil()){
        switch(t->getNodeType()){
            //Statements
            case GazpreaParser::TYPEDEF_VAR_DECLARATION_TOKEN: visitVarDeclarationStatement(t); break;
            case GazpreaParser::ASSIGNMENT_TOKEN: visitAssignmentStatement(t); break;
            case GazpreaParser::CONDITIONAL_STATEMENT_TOKEN: visitConditionalStatement(t); break;
            case GazpreaParser::INFINITE_LOOP_TOKEN: visitInfiniteLoopStatement(t); break;
            case GazpreaParser::PRE_PREDICATE_LOOP_TOKEN: visitPrePredicatedLoopStatement(t);  break;
            case GazpreaParser::POST_PREDICATE_LOOP_TOKEN: visitPostPredicatedLoopStatement(t); break;
            case GazpreaParser::BREAK: visitBreakStatement(t); break;
            case GazpreaParser::CONTINUE: visitContinueStatement(t); break;
            case GazpreaParser::OUTPUT_STREAM_TOKEN: visitOutputStream(t); break;
            case GazpreaParser::INPUT_STREAM_TOKEN: visitInputStream(t); break;
            case GazpreaParser::PROCEDURE: visitProcedureDeclDef(t); break;
            case GazpreaParser::FUNCTION: visitFunctionDeclDef(t); break;
            case GazpreaParser::RETURN: visitReturnStatement(t); break;
            case GazpreaParser::CALL_PROCEDURE_STATEMENT_TOKEN: visitCallProcedure(t); break;            
            case GazpreaParser::CALL_PROCEDURE_FUNCTION_IN_EXPRESSION: visitCallProcedureFunctionFromExpression(t); break;
            case GazpreaParser::TYPEDEF: visitTypedefStatement(t); break;
            case GazpreaParser::BLOCK_TOKEN: visitBlock(t); break;
            default: return;
        };
    }
    else {
        visitChildren(t); // t must be the null root node 
    }
}

void DefWalk::visitChildren(std::shared_ptr<AST> t) { 
    for(std::shared_ptr<AST> stmt : t->children){
        visit(stmt);
    }
}

void DefWalk::visitVarDeclarationStatement(std::shared_ptr<AST> t) {
    visitChildren(t); 
}
void DefWalk::visitAssignmentStatement(std::shared_ptr<AST> t) {
    visitChildren(t);
}  
void DefWalk::visitConditionalStatement(std::shared_ptr<AST> t) {
    visitChildren(t);
}    
void DefWalk::visitInfiniteLoopStatement(std::shared_ptr<AST> t) {
    visitChildren(t);
}  
void DefWalk::visitPrePredicatedLoopStatement(std::shared_ptr<AST> t) {
    visitChildren(t);
}    
void DefWalk::visitPostPredicatedLoopStatement(std::shared_ptr<AST> t) {
    visitChildren(t);
}   
void DefWalk::visitBreakStatement(std::shared_ptr<AST> t) {
    visitChildren(t);
}    
void DefWalk::visitContinueStatement(std::shared_ptr<AST> t) {
    visitChildren(t);
}
void DefWalk::visitOutputStream(std::shared_ptr<AST> t) {
    visitChildren(t);
}    
void DefWalk::visitInputStream(std::shared_ptr<AST> t) {
    visitChildren(t);
}
void DefWalk::visitProcedureDeclDef(std::shared_ptr<AST> t) {
    visitChildren(t);
}
void DefWalk::visitFunctionDeclDef(std::shared_ptr<AST> t) {
    visitChildren(t);
}
void DefWalk::visitReturnStatement(std::shared_ptr<AST> t) {
    visitChildren(t);
}
void DefWalk::visitCallProcedure(std::shared_ptr<AST> t) {
    visitChildren(t);
}  
void DefWalk::visitCallProcedureFunctionFromExpression(std::shared_ptr<AST> t){
    visitChildren(t);
}
void DefWalk::visitTypedefStatement(std::shared_ptr<AST> t) {
    visitChildren(t);
} 
void DefWalk::visitBlock(std::shared_ptr<AST> t) {
    visitChildren(t);
}  

} // namespace gazprea 