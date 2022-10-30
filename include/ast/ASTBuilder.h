#pragma once
#include "GazpreaBaseVisitor.h"
#include "AST.h"

namespace gazprea {

class  ASTBuilder : public GazpreaBaseVisitor {
public:

  ASTBuilder();
  ~ASTBuilder();

  virtual std::any visitCompilationUnit(GazpreaParser::CompilationUnitContext *ctx) override;

  virtual std::any visitVectorSizeDeclarationAtom(GazpreaParser::VectorSizeDeclarationAtomContext *ctx) override;

  virtual std::any visitVectorSizeDeclarationList(GazpreaParser::VectorSizeDeclarationListContext *ctx) override;

  virtual std::any visitTupleTypeDeclarationAtom(GazpreaParser::TupleTypeDeclarationAtomContext *ctx) override;

  virtual std::any visitTupleTypeDeclarationList(GazpreaParser::TupleTypeDeclarationListContext *ctx) override;

  virtual std::any visitScalarType(GazpreaParser::ScalarTypeContext *ctx) override;

  virtual std::any visitVectorType(GazpreaParser::VectorTypeContext *ctx) override;

  virtual std::any visitNonTupleType(GazpreaParser::NonTupleTypeContext *ctx) override;

  virtual std::any visitTupleType(GazpreaParser::TupleTypeContext *ctx) override;

  virtual std::any visitType(GazpreaParser::TypeContext *ctx) override;

  virtual std::any visitTypeQualifier(GazpreaParser::TypeQualifierContext *ctx) override;

  virtual std::any visitTypedefStatement(GazpreaParser::TypedefStatementContext *ctx) override;

  virtual std::any visitScalarVarDeclaration(GazpreaParser::ScalarVarDeclarationContext *ctx) override;

  virtual std::any visitVectorVarDeclaration(GazpreaParser::VectorVarDeclarationContext *ctx) override;

  virtual std::any visitTupleVarDeclaration(GazpreaParser::TupleVarDeclarationContext *ctx) override;

  virtual std::any visitTypeDefVarDeclaration(GazpreaParser::TypeDefVarDeclarationContext *ctx) override;

  virtual std::any visitAssignmentStatement(GazpreaParser::AssignmentStatementContext *ctx) override;

  virtual std::any visitExpressionList(GazpreaParser::ExpressionListContext *ctx) override;

  virtual std::any visitFormalParameter(GazpreaParser::FormalParameterContext *ctx) override;

  virtual std::any visitFormalParameterList(GazpreaParser::FormalParameterListContext *ctx) override;

  virtual std::any visitFunctionDeclaration(GazpreaParser::FunctionDeclarationContext *ctx) override;

  virtual std::any visitFunctionDefinition(GazpreaParser::FunctionDefinitionContext *ctx) override;

  virtual std::any visitProcedureDeclaration(GazpreaParser::ProcedureDeclarationContext *ctx) override;

  virtual std::any visitProcedureDefinition(GazpreaParser::ProcedureDefinitionContext *ctx) override;

  virtual std::any visitReturnStatement(GazpreaParser::ReturnStatementContext *ctx) override;

  virtual std::any visitCallProcedure(GazpreaParser::CallProcedureContext *ctx) override;

  virtual std::any visitConditionalStatement(GazpreaParser::ConditionalStatementContext *ctx) override;

  virtual std::any visitElseIfStatement(GazpreaParser::ElseIfStatementContext *ctx) override;

  virtual std::any visitElseStatement(GazpreaParser::ElseStatementContext *ctx) override;

  virtual std::any visitInfiniteLoopStatement(GazpreaParser::InfiniteLoopStatementContext *ctx) override;

  virtual std::any visitPrePredicatedLoopStatement(GazpreaParser::PrePredicatedLoopStatementContext *ctx) override;

  virtual std::any visitPostPredicatedLoopStatement(GazpreaParser::PostPredicatedLoopStatementContext *ctx) override;

  virtual std::any visitIteratorLoopStatement(GazpreaParser::IteratorLoopStatementContext *ctx) override;

  virtual std::any visitBreakStatement(GazpreaParser::BreakStatementContext *ctx) override;

  virtual std::any visitContinueStatement(GazpreaParser::ContinueStatementContext *ctx) override;

  virtual std::any visitOutputStream(GazpreaParser::OutputStreamContext *ctx) override;

  virtual std::any visitInputStream(GazpreaParser::InputStreamContext *ctx) override;

  virtual std::any visitBlock(GazpreaParser::BlockContext *ctx) override;

  virtual std::any visitExpression(GazpreaParser::ExpressionContext *ctx) override;

  virtual std::any visitCast(GazpreaParser::CastContext *ctx) override;

  virtual std::any visitTupleAccess(GazpreaParser::TupleAccessContext *ctx) override;

  virtual std::any visitRealAtom(GazpreaParser::RealAtomContext *ctx) override;

  virtual std::any visitUnaryOp(GazpreaParser::UnaryOpContext *ctx) override;

  virtual std::any visitCallProcedureFunctionInExpression(GazpreaParser::CallProcedureFunctionInExpressionContext *ctx) override;

  virtual std::any visitIntegerAtom(GazpreaParser::IntegerAtomContext *ctx) override;

  virtual std::any visitVectorLiteral(GazpreaParser::VectorLiteralContext *ctx) override;

  virtual std::any visitParenthesis(GazpreaParser::ParenthesisContext *ctx) override;

  virtual std::any visitConcatenation(GazpreaParser::ConcatenationContext *ctx) override;

  virtual std::any visitFilter(GazpreaParser::FilterContext *ctx) override;

  virtual std::any visitStringLiteralAtom(GazpreaParser::StringLiteralAtomContext *ctx) override;

  virtual std::any visitTupleLiteral(GazpreaParser::TupleLiteralContext *ctx) override;

  virtual std::any visitIdentifierAtom(GazpreaParser::IdentifierAtomContext *ctx) override;

  virtual std::any visitCharacterAtom(GazpreaParser::CharacterAtomContext *ctx) override;

  virtual std::any visitGenerator(GazpreaParser::GeneratorContext *ctx) override;

  virtual std::any visitIdentityAtom(GazpreaParser::IdentityAtomContext *ctx) override;

  virtual std::any visitInterval(GazpreaParser::IntervalContext *ctx) override;

  virtual std::any visitBinaryOp(GazpreaParser::BinaryOpContext *ctx) override;

  virtual std::any visitIndexing(GazpreaParser::IndexingContext *ctx) override;

  virtual std::any visitGeneratorDomainVariable(GazpreaParser::GeneratorDomainVariableContext *ctx) override;

  virtual std::any visitGeneratorDomainVariableList(GazpreaParser::GeneratorDomainVariableListContext *ctx) override;

  virtual std::any visitFilterPredicate(GazpreaParser::FilterPredicateContext *ctx) override;
  

};

}  // namespace gazprea
