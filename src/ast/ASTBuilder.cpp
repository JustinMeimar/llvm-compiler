#include "ASTBuilder.h"

namespace gazprea {

ASTBuilder::ASTBuilder(){}
ASTBuilder::~ASTBuilder(){}

std::any ASTBuilder::visitCompilationUnit(GazpreaParser::CompilationUnitContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitStatement(GazpreaParser::StatementContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitVectorSizeDeclarationAtom(GazpreaParser::VectorSizeDeclarationAtomContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitVectorSizeDeclarationList(GazpreaParser::VectorSizeDeclarationListContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitVectorMatrixCompatibleScalarType(GazpreaParser::VectorMatrixCompatibleScalarTypeContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitScalarType(GazpreaParser::ScalarTypeContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitVectorType(GazpreaParser::VectorTypeContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitNonTupleType(GazpreaParser::NonTupleTypeContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitTupleType(GazpreaParser::TupleTypeContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitType(GazpreaParser::TypeContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitTypeQualifier(GazpreaParser::TypeQualifierContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitScalarVarDeclaration(GazpreaParser::ScalarVarDeclarationContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitVectorVarDeclaration(GazpreaParser::VectorVarDeclarationContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitTupleVarDeclaration(GazpreaParser::TupleVarDeclarationContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitAssignmentStatement(GazpreaParser::AssignmentStatementContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitExpressionList(GazpreaParser::ExpressionListContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitFormalParameter(GazpreaParser::FormalParameterContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitFormalParameterList(GazpreaParser::FormalParameterListContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitFunctionDeclarationDefinition(GazpreaParser::FunctionDeclarationDefinitionContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitProcedureDeclarationDefinition(GazpreaParser::ProcedureDeclarationDefinitionContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitCallProcedure(GazpreaParser::CallProcedureContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitConditionalStatement(GazpreaParser::ConditionalStatementContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitElseIfStatement(GazpreaParser::ElseIfStatementContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitElseStatement(GazpreaParser::ElseStatementContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitInfiniteLoopStatement(GazpreaParser::InfiniteLoopStatementContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitPrePredicatedLoopStatement(GazpreaParser::PrePredicatedLoopStatementContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitPostPredicatedLoopStatement(GazpreaParser::PostPredicatedLoopStatementContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitIteratorLoopStatement(GazpreaParser::IteratorLoopStatementContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitBreakStatement(GazpreaParser::BreakStatementContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitContinueStatement(GazpreaParser::ContinueStatementContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitOutputStream(GazpreaParser::OutputStreamContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitInputStream(GazpreaParser::InputStreamContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitBlock(GazpreaParser::BlockContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitExpression(GazpreaParser::ExpressionContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitTupleAccess(GazpreaParser::TupleAccessContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitRealAtom(GazpreaParser::RealAtomContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitCallProcedureFunctionInExpression(GazpreaParser::CallProcedureFunctionInExpressionContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitIntegerAtom(GazpreaParser::IntegerAtomContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitUnaryOp(GazpreaParser::UnaryOpContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitParenthesis(GazpreaParser::ParenthesisContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitConcatenation(GazpreaParser::ConcatenationContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitFilter(GazpreaParser::FilterContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitStringLiteralAtom(GazpreaParser::StringLiteralAtomContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitTupleLiteral(GazpreaParser::TupleLiteralContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitIdentifierAtom(GazpreaParser::IdentifierAtomContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitCharacterAtom(GazpreaParser::CharacterAtomContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitGenerator(GazpreaParser::GeneratorContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitIdentityAtom(GazpreaParser::IdentityAtomContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitInterval(GazpreaParser::IntervalContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitBinaryOp(GazpreaParser::BinaryOpContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitIndexing(GazpreaParser::IndexingContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitGeneartorDomainVariable(GazpreaParser::GeneartorDomainVariableContext *ctx){
    return 0;
} 

std::any ASTBuilder::visitFilterPredicate(GazpreaParser::FilterPredicateContext *ctx){
    return 0;
} 
}