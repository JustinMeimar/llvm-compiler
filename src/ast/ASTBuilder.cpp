#include "ASTBuilder.h"

namespace gazprea {

    ASTBuilder::ASTBuilder() {}
    ASTBuilder::~ASTBuilder() {}

    std::any ASTBuilder::visitCompilationUnit(GazpreaParser::CompilationUnitContext *ctx) {
        auto t = std::make_shared<AST>(ctx);
        for (auto stat: ctx->statement()) {
            t->addChild(visit(stat));
        }
        return t;
    }
    
    std::any ASTBuilder::visitVectorSizeDeclarationAtom(GazpreaParser::VectorSizeDeclarationAtomContext *ctx) {
        if (ctx->getStart()->getText() == "*") {
            return std::make_shared<AST>(GazpreaParser::ASTERISK, ctx);
        }
        return visit(ctx->expression());
    }

    std::any ASTBuilder::visitVectorSizeDeclarationList(GazpreaParser::VectorSizeDeclarationListContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::VECTOR_SIZE_DECLARATION_LIST_TOKEN, ctx);
        for (auto vectorSizeDeclarationAtom : ctx->vectorSizeDeclarationAtom()) {
            t->addChild(visit(vectorSizeDeclarationAtom));
        }
        return t;
    }

    std::any ASTBuilder::visitExplicitType(GazpreaParser::ExplicitTypeContext *ctx){
        auto t = std::make_shared<AST>(GazpreaParser::EXPLICIT_TYPE_TOKEN, ctx);
        if (ctx->typeQualifier()) {
            t->addChild(visit(ctx->typeQualifier()));
        } else {
            t->addChild(AST::NewNilNode());
        }
        t->addChild(visit(ctx->unqualifiedType()));
        
        return t;
    }

    std::any ASTBuilder::visitInferredType(GazpreaParser::InferredTypeContext *ctx){
        auto t = std::make_shared<AST>(GazpreaParser::INFERRED_TYPE_TOKEN, ctx);
        t->addChild(visit(ctx->typeQualifier()));
        return t;
    }

    std::any ASTBuilder::visitTypeQualifier(GazpreaParser::TypeQualifierContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::TYPE_QUALIFIER_TOKEN, ctx);
        if (ctx->VAR()) {
            t->addChild(std::make_shared<AST>(GazpreaParser::VAR));
        }
        else if (ctx->CONST()) {
            t->addChild(std::make_shared<AST>(GazpreaParser::CONST));
        }        
        return t;
    }

    std::any ASTBuilder::visitVectorMatrixType(GazpreaParser::VectorMatrixTypeContext *ctx){ 
        auto t = std::make_shared<AST>(GazpreaParser::VECTOR_TYPE_TOKEN, ctx);
        t->addChild(visit(ctx->singleTokenType()));
        t->addChild(visit(ctx->vectorSizeDeclarationList())); 
        return t;
    }
 
    std::any ASTBuilder::visitTupleType(GazpreaParser::TupleTypeContext *ctx){
        auto t = std::make_shared<AST>(GazpreaParser::TUPLE_TYPE_TOKEN, ctx);
        t->addChild(visit(ctx->parameterList()));
        return t;
    }

    std::any ASTBuilder::visitSingleTokenType(GazpreaParser::SingleTokenTypeContext *ctx){
        auto t = std::make_shared<AST>(GazpreaParser::SINGLE_TOKEN_TYPE_TOKEN, ctx);
        t->addChild(std::make_shared<AST>(ctx->children[0]));
        return t;
    }
    
    std::any ASTBuilder::visitUnqualifiedType(GazpreaParser::UnqualifiedTypeContext *ctx){
        auto t = std::make_shared<AST>(GazpreaParser::UNQUALIFIED_TYPE_TOKEN, ctx);
        for (auto singleType : ctx->singleTermType()){ 
            t->addChild(visit(singleType));
        } 
        return t;
    }

    std::any ASTBuilder::visitVarDeclarationStatement(GazpreaParser::VarDeclarationStatementContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::VAR_DECLARATION_TOKEN, ctx);
        t->addChild(visit(ctx->qualifiedType()));
        t->addChild(visit(ctx->identifier())); //typedef id
        if (ctx->expression()) {
            t->addChild(visit(ctx->expression()));
        } else {
            t->addChild(AST::NewNilNode());
        }
        return t;
    }

    std::any ASTBuilder::visitAssignmentStatement(GazpreaParser::AssignmentStatementContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::ASSIGNMENT_TOKEN, ctx);
        t->addChild(visit(ctx->expressionList()));
        t->addChild(visit(ctx->expression()));
        return t;
    }

    std::any ASTBuilder::visitExpressionList(GazpreaParser::ExpressionListContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::EXPRESSION_LIST_TOKEN, ctx);
        for (auto expression: ctx->expression()) {
            t->addChild(visit(expression));
        }
        return t;
    }

    std::any ASTBuilder::visitTupleExpressionList(GazpreaParser::TupleExpressionListContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::EXPRESSION_LIST_TOKEN, ctx);  // no need to treat this differently than expression list
        for (auto expression: ctx->expression()) {
            t->addChild(visit(expression));
        }
        return t;
    }
   
    std::any ASTBuilder::visitSubroutineDeclDef(GazpreaParser::SubroutineDeclDefContext *ctx) {
        std::shared_ptr<AST> t = nullptr;
        if (ctx->PROCEDURE()) {
            t = std::make_shared<AST>(GazpreaParser::PROCEDURE, ctx);
        } else if (ctx->FUNCTION()) {
            t = std::make_shared<AST>(GazpreaParser::FUNCTION, ctx);
        }
        t->addChild(visit(ctx->identifier()));
        if (ctx->parameterList()) { // may be no args
            t->addChild(visit(ctx->parameterList()));
        } else {
            t->addChild(AST::NewNilNode());
        }
        if (ctx->unqualifiedType()) {
            t->addChild(visit(ctx->unqualifiedType()));
        } else {
            t->addChild(AST::NewNilNode());
        }
        t->addChild(visit(ctx->subroutineBody()));
        return t;
    }

    std::any ASTBuilder::visitSubroutineEmptyBody(GazpreaParser::SubroutineEmptyBodyContext *ctx){
        return std::make_shared<AST>(GazpreaParser::SUBROUTINE_EMPTY_BODY_TOKEN, ctx);
    }

    std::any ASTBuilder::visitSubroutineExprBody(GazpreaParser::SubroutineExprBodyContext *ctx){
        auto t = std::make_shared<AST>(GazpreaParser::SUBROUTINE_EXPRESSION_BODY_TOKEN, ctx);
        t->addChild(visit(ctx->expression()));
        return t;
    }

    std::any ASTBuilder::visitSubroutineBlockBody(GazpreaParser::SubroutineBlockBodyContext *ctx){
        auto t = std::make_shared<AST>(GazpreaParser::SUBROUTINE_BLOCK_BODY_TOKEN, ctx);
        t->addChild(visit(ctx->block()));
        return t;
    }

    std::any ASTBuilder::visitReturnStatement(GazpreaParser::ReturnStatementContext *ctx){
        auto t = std::make_shared<AST>(GazpreaParser::RETURN, ctx);
        t->addChild(visit(ctx->expression()));
        return t;
    }

    std::any ASTBuilder::visitCallProcedure(GazpreaParser::CallProcedureContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::CALL_PROCEDURE_STATEMENT_TOKEN, ctx);
        t->addChild(visit(ctx->identifier()));
        t->addChild(visit(ctx->expressionList()));
        return t;
    }

    std::any ASTBuilder::visitConditionalStatement(GazpreaParser::ConditionalStatementContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::CONDITIONAL_STATEMENT_TOKEN, ctx);
        t->addChild(visit(ctx->expression()));
        t->addChild(visit(ctx->exprPrecededStatement()));
        for (auto elseIfStatement : ctx->elseIfStatement()) {
            t->addChild(visit(elseIfStatement));
        }
        if (ctx->elseStatement()) {
            t->addChild(visit(ctx->elseStatement()));
        }
        return t;
    }

    std::any ASTBuilder::visitElseIfStatement(GazpreaParser::ElseIfStatementContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::ELSEIF_TOKEN, ctx);
        t->addChild(visit(ctx->expression()));
        t->addChild(visit(ctx->exprPrecededStatement()));
        return t;
    }

    std::any ASTBuilder::visitElseStatement(GazpreaParser::ElseStatementContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::ELSE_TOKEN, ctx);
        t->addChild(visit(ctx->statement()));
        return t;
    }

    std::any ASTBuilder::visitInfiniteLoopStatement(GazpreaParser::InfiniteLoopStatementContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::INFINITE_LOOP_TOKEN, ctx);
        t->addChild(visit(ctx->statement()));
        return t;
    }

    std::any ASTBuilder::visitPrePredicatedLoopStatement(GazpreaParser::PrePredicatedLoopStatementContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::PRE_PREDICATE_LOOP_TOKEN, ctx);
        t->addChild(visit(ctx->expression()));
        t->addChild(visit(ctx->exprPrecededStatement()));
        return t;
    }

    std::any ASTBuilder::visitPostPredicatedLoopStatement(GazpreaParser::PostPredicatedLoopStatementContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::POST_PREDICATE_LOOP_TOKEN, ctx);
        t->addChild(visit(ctx->statement()));
        t->addChild(visit(ctx->expression()));
        return t;
    }

    std::any ASTBuilder::visitIteratorLoopStatement(GazpreaParser::IteratorLoopStatementContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::ITERATOR_LOOP_TOKEN, ctx);
        for (auto domainExpression : ctx->domainExpression()) {
            t->addChild(visit(domainExpression));
        }
        t->addChild(visit(ctx->exprPrecededStatement()));
        return t;
    }

    std::any ASTBuilder::visitBreakStatement(GazpreaParser::BreakStatementContext *ctx) {
        return std::make_shared<AST>(GazpreaParser::BREAK, ctx);
    }

    std::any ASTBuilder::visitContinueStatement(GazpreaParser::ContinueStatementContext *ctx) {
        return std::make_shared<AST>(GazpreaParser::CONTINUE, ctx);
    }

    std::any ASTBuilder::visitOutputStream(GazpreaParser::OutputStreamContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::OUTPUT_STREAM_TOKEN, ctx);
        t->addChild(visit(ctx->expression()));
        t->addChild(visit(ctx->identifier()));
        return t;
    }

    std::any ASTBuilder::visitInputStream(GazpreaParser::InputStreamContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::INPUT_STREAM_TOKEN, ctx);
        t->addChild(visit(ctx->expression()));
        t->addChild(visit(ctx->identifier()));
        return t;
    }

    std::any ASTBuilder::visitBlock(GazpreaParser::BlockContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::BLOCK_TOKEN, ctx);
        for (auto stat : ctx->statement()) {
            t->addChild(visit(stat));
        }
        return t;
    }

    std::any ASTBuilder::visitExpression(GazpreaParser::ExpressionContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::EXPRESSION_TOKEN, ctx);
        t->addChild(visit(ctx->expr()));
        return t;
    }

    std::any ASTBuilder::visitTupleAccess(GazpreaParser::TupleAccessContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::TUPLE_ACCESS_TOKEN, ctx);
        t->addChild(visit(ctx->expr()));
        if (ctx->IntegerConstant()) {
            t->addChild(std::make_shared<AST>(ctx->IntegerConstant()));
        } else if (ctx->identifier()) {
            t->addChild(visit(ctx->identifier()));
        }
        return t;
    }

    std::any ASTBuilder::visitCallProcedureFunctionInExpression(GazpreaParser::CallProcedureFunctionInExpressionContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::CALL_PROCEDURE_FUNCTION_IN_EXPRESSION, ctx);
        t->addChild(visit(ctx->identifier()));
        if (ctx->expressionList()) {
            t->addChild(visit(ctx->expressionList()));
        }
        return t;
    }

    std::any ASTBuilder::visitIntegerAtom(GazpreaParser::IntegerAtomContext *ctx) {
        return std::make_shared<AST>(GazpreaParser::IntegerConstant, ctx->IntegerConstant());
    }
    
    std::any ASTBuilder::visitUnaryOp(GazpreaParser::UnaryOpContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::UNARY_TOKEN, ctx);
        switch (ctx->op->getType()) {
            case GazpreaParser::PLUS:
                t->addChild(std::make_shared<AST>(GazpreaParser::PLUS));
                break;
            case GazpreaParser::MINUS:
                t->addChild(std::make_shared<AST>(GazpreaParser::MINUS));
                break;
            default:
                t->addChild(std::make_shared<AST>(GazpreaParser::NOT));
                break;
        }
        t->addChild(visit(ctx->expr()));
        return t;
    }

    std::any ASTBuilder::visitParenthesis(GazpreaParser::ParenthesisContext *ctx) {
        return visit(ctx->expr());
    }

    std::any ASTBuilder::visitConcatenation(GazpreaParser::ConcatenationContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::STRING_CONCAT_TOKEN, ctx);
        t->addChild(visit(ctx->expr(0)));
        t->addChild(visit(ctx->expr(1)));
        return t;
    }

    std::any ASTBuilder::visitFilter(GazpreaParser::FilterContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::FILTER_TOKEN, ctx);
        t->addChild(visit(ctx->identifier()));
        t->addChild(visit(ctx->expression()));
        t->addChild(visit(ctx->expressionList()));
        return t;
    }

    std::any ASTBuilder::visitStringLiteralAtom(GazpreaParser::StringLiteralAtomContext *ctx) {
        return std::make_shared<AST>(ctx->StringLiteral());
    }

    std::any ASTBuilder::visitTupleLiteral(GazpreaParser::TupleLiteralContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::TUPLE_LITERAL_TOKEN, ctx);
        t->addChild(visit(ctx->tupleExpressionList()));
        return t;
    }

    std::any ASTBuilder::visitVectorLiteral(GazpreaParser::VectorLiteralContext *ctx){
        auto t = std::make_shared<AST>(GazpreaParser::VECTOR_LITERAL_TOKEN, ctx);
        t->addChild(visit(ctx->expressionList()));
        return t;
    }

    std::any ASTBuilder::visitCharacterAtom(GazpreaParser::CharacterAtomContext *ctx) {
        return std::make_shared<AST>(ctx->CharacterConstant());
    }

    std::any ASTBuilder::visitGenerator(GazpreaParser::GeneratorContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::GENERATOR_TOKEN, ctx);
        t->addChild(visit(ctx->generatorDomainVariableList()));
        t->addChild(visit(ctx->expression()));
        return t;
    }

    std::any ASTBuilder::visitInterval(GazpreaParser::IntervalContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::INTERVAL, ctx);
        t->addChild(visit(ctx->expr(0)));
        t->addChild(visit(ctx->expr(1)));
        return t;
    }

    std::any ASTBuilder::visitBinaryOp(GazpreaParser::BinaryOpContext *ctx) {
        std::shared_ptr<AST> t = std::make_shared<AST>(GazpreaParser::BINARY_OP_TOKEN, ctx);
        t->addChild(visit(ctx->expr(0)));
        t->addChild(visit(ctx->expr(1)));
        t->addChild(std::make_shared<AST>(ctx->op->getType()));
        return t;
    }

    std::any ASTBuilder::visitIndexing(GazpreaParser::IndexingContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::INDEXING_TOKEN, ctx);
        t->addChild(visit(ctx->expr(0)));
        t->addChild(visit(ctx->expr(1)));
        return t;
    }

    std::any ASTBuilder::visitTypedefStatement(GazpreaParser::TypedefStatementContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::TYPEDEF, ctx);
        t->addChild(visit(ctx->unqualifiedType()));
        t->addChild(visit(ctx->identifier()));
        return t;
    }

    std::any ASTBuilder::visitCast(GazpreaParser::CastContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::CAST_TOKEN, ctx);
        t->addChild(visit(ctx->unqualifiedType()));
        t->addChild(visit(ctx->expression()));
        return t;
    }

    std::any ASTBuilder::visitParameterAtom(GazpreaParser::ParameterAtomContext *ctx) {
        ///children: Term1 Term2 Term3 TypeQualifier
        auto t = std::make_shared<AST>(GazpreaParser::PARAMETER_ATOM_TOKEN, ctx);
        const auto &terms = ctx->singleTermType();
        for (int i = 0; i < 3; i++) {
            if (i + 1 <= (int) terms.size())
                t->addChild(visit(terms[i]));
            else
                t->addChild(AST::NewNilNode());
        }
        if (ctx->typeQualifier()) {
            t->addChild(visit(ctx->typeQualifier()));
        } else {
            t->addChild(AST::NewNilNode());
        }
        return t;
    }

    std::any ASTBuilder::visitParameterList(GazpreaParser::ParameterListContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::PARAMETER_LIST_TOKEN, ctx);
        for (auto tupleTypeDeclarationAtom : ctx->parameterAtom()) {
            t->addChild(visit(tupleTypeDeclarationAtom));
        }
        return t;
    }

    std::any ASTBuilder::visitDomainExpression(GazpreaParser::DomainExpressionContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::DOMAIN_EXPRESSION_TOKEN, ctx);
        t->addChild(visit(ctx->identifier()));
        t->addChild(visit(ctx->expression()));
        return t;
    }

    std::any ASTBuilder::visitGeneratorDomainVariableList(GazpreaParser::GeneratorDomainVariableListContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::GENERATOR_DOMAIN_VARIABLE_LIST_TOKEN, ctx);
        for (auto domainExpression : ctx->domainExpression()) {
            t->addChild(visit(domainExpression));
        }
        return t;
    }

    std::any ASTBuilder::visitIdentifier(GazpreaParser::IdentifierContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::IDENTIFIER_TOKEN, ctx->children[0]);
        return t;
    }

    std::any ASTBuilder::visitRealConstant(GazpreaParser::RealConstantContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::REAL_CONSTANT_TOKEN, ctx);
        return t;
    }
}