#include "DefWalk.h"

namespace gazprea {

DefWalk::DefWalk() {}
DefWalk::~DefWalk() {}

void DefWalk::visit(std::shared_ptr<AST> t) {
    if(!t->isNil()){
        switch(t->getNodeType()){
            //Statements & Intermediate Tokens
            case GazpreaParser::TYPEDEF_VAR_DECLARATION_TOKEN: visitVarDeclarationStatement(t); break;
            case GazpreaParser::INFERRED_TYPE_TOKEN: visitInferedType(t); break;
            case GazpreaParser::EXPLICIT_TYPE_TOKEN: visitExplicitType(t); break;            
            case GazpreaParser::TYPE_QUALIFIER_TOKEN: visitTypeQualifier(t); break;

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

            case GazpreaParser::FORMAL_PARAMETER_LIST_TOKEN: visitFormalParamaterList(t); break;
            case GazpreaParser::UNQUALIFIED_TYPE_TOKEN: visitUnqualifiedType(t); break;
            case GazpreaParser::REAL: visitRealType(t); break;

            case GazpreaParser::SUBROUTINE_BLOCK_BODY_TOKEN: visitSubroutineBlockBody(t); break;
            
            case GazpreaParser::RETURN: visitReturnStatement(t); break;
            case GazpreaParser::CALL_PROCEDURE_STATEMENT_TOKEN: visitCallProcedure(t); break;            
            case GazpreaParser::CALL_PROCEDURE_FUNCTION_IN_EXPRESSION: visitCallProcedureFunctionFromExpression(t); break;
            case GazpreaParser::TYPEDEF: visitTypedefStatement(t); break;
            case GazpreaParser::BLOCK_TOKEN: visitBlock(t); break;

            //Expressions
            case GazpreaParser::EXPR_TOKEN: visitExpr(t); break;
            
            //Atoms
            case GazpreaParser::IDENTIFIER_TOKEN: visitIdentifier(t); break;
            case GazpreaParser::REAL_CONSTANT_TOKEN: visitRealConstant(t); break;

            default: {
                printf("unimplemented token recieved\n"); return;
            };
        };
    }
    else {
        std::cout << "Visit\n"; 
        visitChildren(t); // t must be the null root node 
    }
}

void DefWalk::visitChildren(std::shared_ptr<AST> t) { 
    for(std::shared_ptr<AST> stmt : t->children){
        visit(stmt);
    }
}

void DefWalk::visitVarDeclarationStatement(std::shared_ptr<AST> t) {
    std::cout << "--------------\nVisit Var Decl\n";
    visitChildren(t); 
}
void DefWalk::visitInferedType(std::shared_ptr<AST> t){
    std::cout << "Visit Infered Type\n";
    visitChildren(t);
}
void DefWalk::visitExplicitType(std::shared_ptr<AST> t){
    std::cout << "Visit Explicit Type\n";
    visitChildren(t);
}
void DefWalk::visitTypeQualifier(std::shared_ptr<AST> t){
    std::cout << "Visit Const/Var\n";
    visitChildren(t);
}

void DefWalk::visitAssignmentStatement(std::shared_ptr<AST> t) {
    std::cout << "Visit Assignment\n"; 
    visitChildren(t);
}  
void DefWalk::visitConditionalStatement(std::shared_ptr<AST> t) {
    std::cout << "Visit Conditional\n"; 
    visitChildren(t);
}    
void DefWalk::visitInfiniteLoopStatement(std::shared_ptr<AST> t) {
    std::cout << "Visit Infinite Loop\n"; 
    visitChildren(t);
}  
void DefWalk::visitPrePredicatedLoopStatement(std::shared_ptr<AST> t) {
    std::cout << "Visit Pre Predicated Loop\n"; 
    visitChildren(t);
}    
void DefWalk::visitPostPredicatedLoopStatement(std::shared_ptr<AST> t) {
    std::cout << "Visit Post Predicated Loop\n"; 
    visitChildren(t);
}   
void DefWalk::visitBreakStatement(std::shared_ptr<AST> t) {
    std::cout << "Visit Break\n"; 
    visitChildren(t);
}    
void DefWalk::visitContinueStatement(std::shared_ptr<AST> t) {
    std::cout << "Visit Continue\n"; 
    visitChildren(t);
}
void DefWalk::visitOutputStream(std::shared_ptr<AST> t) {
    std::cout << "Visit Output Stream\n"; 
    visitChildren(t);
}    
void DefWalk::visitInputStream(std::shared_ptr<AST> t) {
    std::cout << "Visit Input Stream\n"; 
    visitChildren(t);
}
void DefWalk::visitProcedureDeclDef(std::shared_ptr<AST> t) {
    std::cout << "Visit Procedure Declaration or Definition\n"; 
    visitChildren(t);
}
void DefWalk::visitFunctionDeclDef(std::shared_ptr<AST> t) {
    std::cout << "Visit Function Decl Def\n"; 
    visitChildren(t);
}

void DefWalk::visitFormalParamaterList(std::shared_ptr<AST> t){
    std::cout << "Visit Formal Parameter List\n";
    visitChildren(t);
}

void DefWalk::visitUnqualifiedType(std::shared_ptr<AST> t){
    std::cout << "Visit unqualified Type\n";
    visitChildren(t);
}

void DefWalk::visitRealType(std::shared_ptr<AST> t) {
    std::cout << "Visit Real Type"; 
    return;
}

void DefWalk::visitSubroutineBlockBody(std::shared_ptr<AST> t){
    std::cout << "Visit subroutine block body\n";
    visitChildren(t);
}
            
void DefWalk::visitReturnStatement(std::shared_ptr<AST> t) {
    std::cout << "Visit Return Statement\n"; 
    visitChildren(t);
}
void DefWalk::visitCallProcedure(std::shared_ptr<AST> t) {
    std::cout << "Visit Call Proceedure\n"; 
    visitChildren(t);
}  
void DefWalk::visitCallProcedureFunctionFromExpression(std::shared_ptr<AST> t){
    std::cout << "Visit Call Proceedure From Expression\n"; 
    visitChildren(t);
}
void DefWalk::visitTypedefStatement(std::shared_ptr<AST> t) {
    std::cout << "Visit TypeDef\n"; 
    visitChildren(t);
} 
void DefWalk::visitBlock(std::shared_ptr<AST> t) {
    std::cout << "Visit Block\n"; 
    visitChildren(t);
} 

void DefWalk::visitExpr(std::shared_ptr<AST> t) {
    std::cout << "Visit Expr\n";
    visitChildren(t);
}
void DefWalk::visitIdentifier(std::shared_ptr<AST> t) {
    std::cout << "Visit Identifier\n";
    return; // Leaf Node
}

void DefWalk::visitRealConstant(std::shared_ptr<AST> t) {
    std::cout << "Visit real constant\n"; 
    return;
}

} // namespace gazprea 