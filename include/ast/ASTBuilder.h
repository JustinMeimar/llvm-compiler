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

  virtual std::any visitParameterAtom(GazpreaParser::ParameterAtomContext *ctx) override;

  virtual std::any visitParameterList(GazpreaParser::ParameterListContext *ctx) override;

  virtual std::any visitSingleTokenType(GazpreaParser::SingleTokenTypeContext *ctx) override;

  virtual std::any visitVectorMatrixType(GazpreaParser::VectorMatrixTypeContext *ctx) override;

  virtual std::any visitTupleType(GazpreaParser::TupleTypeContext *ctx) override;

  virtual std::any visitTypeQualifier(GazpreaParser::TypeQualifierContext *ctx) override;

  virtual std::any visitUnqualifiedType(GazpreaParser::UnqualifiedTypeContext *ctx) override;

  virtual std::any visitExplcitType(GazpreaParser::ExplcitTypeContext *ctx) override;

  virtual std::any visitInferredType(GazpreaParser::InferredTypeContext *ctx) override;

  virtual std::any visitTypedefStatement(GazpreaParser::TypedefStatementContext *ctx) override;

  virtual std::any visitVarDeclarationStatement(GazpreaParser::VarDeclarationStatementContext *ctx) override;

  virtual std::any visitAssignmentStatement(GazpreaParser::AssignmentStatementContext *ctx) override;

  virtual std::any visitExpressionList(GazpreaParser::ExpressionListContext *ctx) override;

  virtual std::any visitTupleExpressionList(GazpreaParser::TupleExpressionListContext *ctx) override;

  virtual std::any visitSubroutineEmptyBody(GazpreaParser::SubroutineEmptyBodyContext *ctx) override;

  virtual std::any visitSubroutineExprBody(GazpreaParser::SubroutineExprBodyContext *ctx) override;

  virtual std::any visitSubroutineBlockBody(GazpreaParser::SubroutineBlockBodyContext *ctx) override;

  virtual std::any visitSubroutineDeclDef(GazpreaParser::SubroutineDeclDefContext *ctx) override;

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

  virtual std::any visitUnaryOp(GazpreaParser::UnaryOpContext *ctx) override;

  virtual std::any visitCallProcedureFunctionInExpression(GazpreaParser::CallProcedureFunctionInExpressionContext *ctx) override;

  virtual std::any visitIntegerAtom(GazpreaParser::IntegerAtomContext *ctx) override;

  virtual std::any visitVectorLiteral(GazpreaParser::VectorLiteralContext *ctx) override;

  virtual std::any visitParenthesis(GazpreaParser::ParenthesisContext *ctx) override;

  virtual std::any visitConcatenation(GazpreaParser::ConcatenationContext *ctx) override;

  virtual std::any visitFilter(GazpreaParser::FilterContext *ctx) override;

  virtual std::any visitStringLiteralAtom(GazpreaParser::StringLiteralAtomContext *ctx) override;

  virtual std::any visitTupleLiteral(GazpreaParser::TupleLiteralContext *ctx) override;

  virtual std::any visitCharacterAtom(GazpreaParser::CharacterAtomContext *ctx) override;

  virtual std::any visitGenerator(GazpreaParser::GeneratorContext *ctx) override;

  virtual std::any visitInterval(GazpreaParser::IntervalContext *ctx) override;

  virtual std::any visitBinaryOp(GazpreaParser::BinaryOpContext *ctx) override;

  virtual std::any visitIndexing(GazpreaParser::IndexingContext *ctx) override;

  virtual std::any visitDomainExpression(GazpreaParser::DomainExpressionContext *ctx) override;

  virtual std::any visitGeneratorDomainVariableList(GazpreaParser::GeneratorDomainVariableListContext *ctx) override;

  virtual std::any visitIdentifier(GazpreaParser::IdentifierContext *ctx) override;

  virtual std::any visitRealConstant(GazpreaParser::RealConstantContext *ctx) override;

};

}  // namespace gazprea
