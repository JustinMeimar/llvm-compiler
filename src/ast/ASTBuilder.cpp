#include "ASTBuilder.h"

namespace gazprea {

    ASTBuilder::ASTBuilder() {}
    ASTBuilder::~ASTBuilder() {}

    std::any ASTBuilder::visitCompilationUnit(GazpreaParser::CompilationUnitContext *ctx) {
        auto t = std::make_shared<AST>();
        for (auto stat: ctx->statement()) {
            t->addChild(visit(stat));
        }
        return t;
    }
    
    std::any ASTBuilder::visitVectorSizeDeclarationAtom(GazpreaParser::VectorSizeDeclarationAtomContext *ctx) {
        if (ctx->getStart()->getText() == "*") {
            return std::make_shared<AST>(GazpreaParser::ASTERISK);
        }
        return visit(ctx->expression());
    }

    std::any ASTBuilder::visitVectorSizeDeclarationList(GazpreaParser::VectorSizeDeclarationListContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::VECTOR_SIZE_DECLARATION_LIST_TOKEN);
        for (auto vectorSizeDeclarationAtom : ctx->vectorSizeDeclarationAtom()) {
            t->addChild(visit(vectorSizeDeclarationAtom));
        }
        return t;
    }

    std::any ASTBuilder::visitExplcitType(GazpreaParser::ExplcitTypeContext *ctx){
        auto t = std::make_shared<AST>(GazpreaParser::EXPLICIT_TYPE_TOKEN);
        if (ctx->typeQualifier()) {
            t->addChild(visit(ctx->typeQualifier()));
        }
        t->addChild(visit(ctx->unqualifiedType()));
        
        return t;
    }

    std::any ASTBuilder::visitInferredType(GazpreaParser::InferredTypeContext *ctx){
        auto t = std::make_shared<AST>(GazpreaParser::INFERRED_TYPE_TOKEN);
        t->addChild(visit(ctx->typeQualifier()));
        return t;
    }

    std::any ASTBuilder::visitTypeQualifier(GazpreaParser::TypeQualifierContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::TYPE_QUALIFIER_TOKEN);
        if (ctx->VAR()) {
            t->addChild(std::make_shared<AST>(GazpreaParser::VAR));
        }
        else if (ctx->CONST()) {
            t->addChild(std::make_shared<AST>(GazpreaParser::CONST));
        }        
        return t;
    }

    std::any ASTBuilder::visitVectorMatrixType(GazpreaParser::VectorMatrixTypeContext *ctx){ 
        auto t = std::make_shared<AST>(GazpreaParser::VECTOR_TYPE_TOKEN);
        t->addChild(visit(ctx->singleTokenType()));
        t->addChild(visit(ctx->vectorSizeDeclarationList())); 
        return t;
    }
 
    std::any ASTBuilder::visitTupleType(GazpreaParser::TupleTypeContext *ctx){
        auto t = std::make_shared<AST>(GazpreaParser::TUPLE_TYPE_TOKEN);
        t->addChild(visit(ctx->tupleTypeDeclarationList()));
        return t;
    }

    std::any ASTBuilder::visitSingleTokenType(GazpreaParser::SingleTokenTypeContext *ctx){
        auto t = std::make_shared<AST>(GazpreaParser::SINGLE_TOKEN_TYPE_TOKEN);
        t->addChild(std::make_shared<AST>(ctx->getStart()));
        return t;
    }

    std::any ASTBuilder::visitSingleTokenTypeAtom(GazpreaParser::SingleTokenTypeAtomContext *ctx){
        return visit(ctx->singleTokenType());
    }
    
    std::any ASTBuilder::visitUnqualifiedType(GazpreaParser::UnqualifiedTypeContext *ctx){
        auto t = std::make_shared<AST>(GazpreaParser::UNQUALIFIED_TYPE_TOKEN);
        for (auto singleType : ctx->singleTermType()){ 
            t->addChild(visit(singleType));
        } 
        return t;
    }

    std::any ASTBuilder::visitVarDeclarationStatement(GazpreaParser::VarDeclarationStatementContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::TYPEDEF_VAR_DECLARATION_TOKEN);
        t->addChild(visit(ctx->qualifiedType()));
        t->addChild(visit(ctx->identifier())); //typedef id
        if (ctx->expression()) {
            t->addChild(visit(ctx->expression()));
        }
        return t;
    }

    std::any ASTBuilder::visitAssignmentStatement(GazpreaParser::AssignmentStatementContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::ASSIGNMENT_TOKEN);
        t->addChild(visit(ctx->expressionList()));
        t->addChild(visit(ctx->expression()));
        return t;
    }

    std::any ASTBuilder::visitExpressionList(GazpreaParser::ExpressionListContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::EXPRESSION_LIST_TOKEN);
        for (auto expression: ctx->expression()) {
            t->addChild(visit(expression));
        }
        return t;
    }

    std::any ASTBuilder::visitTupleExpressionList(GazpreaParser::TupleExpressionListContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::EXPRESSION_LIST_TOKEN);  // no need to treat this differently than expression list
        for (auto expression: ctx->expression()) {
            t->addChild(visit(expression));
        }
        return t;
    }

    std::any ASTBuilder::visitFormalParameter(GazpreaParser::FormalParameterContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::FORMAL_PARAMETER_TOKEN);
        t->addChild(visit(ctx->qualifiedType()));
        t->addChild(visit(ctx->identifier()));
        return t;
    }

    std::any ASTBuilder::visitFormalParameterList(GazpreaParser::FormalParameterListContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::FORMAL_PARAMETER_LIST_TOKEN);
        for (auto formalParameter : ctx->formalParameter()) {
            t->addChild(visit(formalParameter));
        }
        return t;
    }
   
    std::any ASTBuilder::visitSubroutineDeclDef(GazpreaParser::SubroutineDeclDefContext *ctx) {
        std::shared_ptr<AST> t = nullptr;
        if (ctx->PROCEDURE()) {
            t = std::make_shared<AST>(GazpreaParser::PROCEDURE);
        } else if (ctx->FUNCTION()) {
            t = std::make_shared<AST>(GazpreaParser::FUNCTION);
        }
        t->addChild(visit(ctx->identifier()));
        if (ctx->formalParameterList()) { // may be no args
            t->addChild(visit(ctx->formalParameterList()));
        }
        if (ctx->unqualifiedType()) {
            t->addChild(visit(ctx->unqualifiedType()));
        }
        if (ctx->subroutineBody()) { // only if def
            t->addChild(visit(ctx->subroutineBody())); 
        }
        return t;
    }

    std::any ASTBuilder::visitFunctionEmptyBody(GazpreaParser::FunctionEmptyBodyContext *ctx){
        return std::make_shared<AST>(GazpreaParser::SUBROUTINE_EMPTY_BODY_TOKEN); 
    }

    std::any ASTBuilder::visitFunctionExprBody(GazpreaParser::FunctionExprBodyContext *ctx){
        auto t = std::make_shared<AST>(GazpreaParser::SUBROUTINE_EXPRESSION_BODY_TOKEN);
        t->addChild(visit(ctx->expression()));
        return t;
    }

    std::any ASTBuilder::visitFunctionBlockBody(GazpreaParser::FunctionBlockBodyContext *ctx){
        auto t = std::make_shared<AST>(GazpreaParser::SUBROUTINE_BLOCK_BODY_TOKEN);
        t->addChild(visit(ctx->block()));
        return t;
    }

    std::any ASTBuilder::visitReturnStatement(GazpreaParser::ReturnStatementContext *ctx){
        auto t = std::make_shared<AST>(GazpreaParser::RETURN);
        t->addChild(visit(ctx->expression()));
        return t;
    }

    std::any ASTBuilder::visitCallProcedure(GazpreaParser::CallProcedureContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::CALL_PROCEDURE_STATEMENT_TOKEN);
        t->addChild(visit(ctx->identifier()));
        t->addChild(visit(ctx->expressionList()));
        return t;
    }

    std::any ASTBuilder::visitConditionalStatement(GazpreaParser::ConditionalStatementContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::CONDITIONAL_STATEMENT_TOKEN);
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
        auto t = std::make_shared<AST>(GazpreaParser::ELSEIF_TOKEN);
        t->addChild(visit(ctx->expression()));
        t->addChild(visit(ctx->exprPrecededStatement()));
        return t;
    }

    std::any ASTBuilder::visitElseStatement(GazpreaParser::ElseStatementContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::ELSE_TOKEN);
        t->addChild(visit(ctx->statement()));
        return t;
    }

    std::any ASTBuilder::visitInfiniteLoopStatement(GazpreaParser::InfiniteLoopStatementContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::INFINITE_LOOP_TOKEN);
        t->addChild(visit(ctx->statement()));
        return t;
    }

    std::any ASTBuilder::visitPrePredicatedLoopStatement(GazpreaParser::PrePredicatedLoopStatementContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::PRE_PREDICATE_LOOP_TOKEN);
        t->addChild(visit(ctx->expression()));
        t->addChild(visit(ctx->exprPrecededStatement()));
        return t;
    }

    std::any ASTBuilder::visitPostPredicatedLoopStatement(GazpreaParser::PostPredicatedLoopStatementContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::POST_PREDICATE_LOOP_TOKEN);
        t->addChild(visit(ctx->statement()));
        t->addChild(visit(ctx->expression()));
        return t;
    }

    std::any ASTBuilder::visitIteratorLoopStatement(GazpreaParser::IteratorLoopStatementContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::ITERATOR_LOOP_TOKEN);
        for (auto domainExpression : ctx->domainExpression()) {
            t->addChild(visit(domainExpression));
        }
        t->addChild(visit(ctx->exprPrecededStatement()));
        return t;
    }

    std::any ASTBuilder::visitBreakStatement(GazpreaParser::BreakStatementContext *ctx) {
        return std::make_shared<AST>(GazpreaParser::BREAK);
    }

    std::any ASTBuilder::visitContinueStatement(GazpreaParser::ContinueStatementContext *ctx) {
        return std::make_shared<AST>(GazpreaParser::CONTINUE);
    }

    std::any ASTBuilder::visitOutputStream(GazpreaParser::OutputStreamContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::OUTPUT_STREAM_TOKEN);
        t->addChild(visit(ctx->expression()));
        t->addChild(visit(ctx->identifier()));
        return t;
    }

    std::any ASTBuilder::visitInputStream(GazpreaParser::InputStreamContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::INPUT_STREAM_TOKEN);
        t->addChild(visit(ctx->expression()));
        t->addChild(visit(ctx->identifier()));
        return t;
    }

    std::any ASTBuilder::visitBlock(GazpreaParser::BlockContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::BLOCK_TOKEN);
        for (auto stat : ctx->statement()) {
            t->addChild(visit(stat));
        }
        return t;
    }

    std::any ASTBuilder::visitExpression(GazpreaParser::ExpressionContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::EXPR_TOKEN);
        t->addChild(visit(ctx->expr()));
        return t;
    }

    std::any ASTBuilder::visitTupleAccess(GazpreaParser::TupleAccessContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::TUPLE_ACCESS_TOKEN);
        t->addChild(visit(ctx->expr()));
        t->addChild(visit(ctx->tupleAccessIndex()));
        return t;
    }

    std::any ASTBuilder::visitCallProcedureFunctionInExpression(GazpreaParser::CallProcedureFunctionInExpressionContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::CALL_PROCEDURE_FUNCTION_IN_EXPRESSION);
        t->addChild(visit(ctx->identifier()));
        if (ctx->expressionList()) {
            t->addChild(visit(ctx->expressionList()));
        }
        return t;
    }

    std::any ASTBuilder::visitIntegerAtom(GazpreaParser::IntegerAtomContext *ctx) {
        return std::make_shared<AST>(ctx->IntegerConstant()->getSymbol());
    }
    
    std::any ASTBuilder::visitUnaryOp(GazpreaParser::UnaryOpContext *ctx) {
        std::shared_ptr<AST> t = nullptr;
        switch (ctx->op->getType()) {
            case GazpreaParser::PLUS:
                t = std::make_shared<AST>(GazpreaParser::PLUS);
                break;
            
            default:
                t = std::make_shared<AST>(GazpreaParser::MINUS);
                break;
        }
        t->addChild(visit(ctx->expr()));
        return t;
    }

    std::any ASTBuilder::visitParenthesis(GazpreaParser::ParenthesisContext *ctx) {
        return visit(ctx->expr());
    }

    std::any ASTBuilder::visitConcatenation(GazpreaParser::ConcatenationContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::STRING_CONCAT_TOKEN);
        t->addChild(visit(ctx->expr(0)));
        t->addChild(visit(ctx->expr(1)));
        return t;
    }

    std::any ASTBuilder::visitFilter(GazpreaParser::FilterContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::FILTER_TOKEN);
        t->addChild(visit(ctx->identifier()));
        t->addChild(visit(ctx->expression()));
        t->addChild(visit(ctx->expressionList()));
        return t;
    }

    std::any ASTBuilder::visitStringLiteralAtom(GazpreaParser::StringLiteralAtomContext *ctx) {
        return std::make_shared<AST>(ctx->StringLiteral()->getSymbol());
    }

    std::any ASTBuilder::visitTupleLiteral(GazpreaParser::TupleLiteralContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::TUPLE_LITERAL_TOKEN);
        t->addChild(visit(ctx->tupleExpressionList()));
        return t;
    }

    std::any ASTBuilder::visitVectorLiteral(GazpreaParser::VectorLiteralContext *ctx){
        auto t = std::make_shared<AST>(GazpreaParser::VECTOR_LITERAL_TOKEN);
        t->addChild(visit(ctx->expressionList()));
        return t;
    }

    std::any ASTBuilder::visitCharacterAtom(GazpreaParser::CharacterAtomContext *ctx) {
        return std::make_shared<AST>(ctx->CharacterConstant()->getSymbol());
    }

    std::any ASTBuilder::visitGenerator(GazpreaParser::GeneratorContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::GENERATOR_TOKEN);
        t->addChild(visit(ctx->generatorDomainVariableList()));
        t->addChild(visit(ctx->expression()));
        return t;
    }

    std::any ASTBuilder::visitInterval(GazpreaParser::IntervalContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::INTERVAL);
        t->addChild(visit(ctx->expr(0)));
        t->addChild(visit(ctx->expr(1)));
        return t;
    }

    std::any ASTBuilder::visitBinaryOp(GazpreaParser::BinaryOpContext *ctx) {
        std::shared_ptr<AST> t = nullptr;
        switch (ctx->op->getType()) {
            case GazpreaParser::PLUS:
                t = std::make_shared<AST>(GazpreaParser::PLUS);
                break;
            case GazpreaParser::MINUS:
                t = std::make_shared<AST>(GazpreaParser::MINUS);
                break;
            case GazpreaParser::ASTERISK:
                t = std::make_shared<AST>(GazpreaParser::ASTERISK);
                break;
            case GazpreaParser::DIV:
                t = std::make_shared<AST>(GazpreaParser::DIV);
                break;
            case GazpreaParser::MODULO:
                t = std::make_shared<AST>(GazpreaParser::MODULO);
                break;
            case GazpreaParser::DOTPRODUCT:
                t = std::make_shared<AST>(GazpreaParser::DOTPRODUCT);
                break;
            case GazpreaParser::LESSTHAN:
                t = std::make_shared<AST>(GazpreaParser::LESSTHAN);
                break;
            case GazpreaParser::GREATERTHAN:
                t = std::make_shared<AST>(GazpreaParser::GREATERTHAN);
                break;
            case GazpreaParser::LESSTHANOREQUAL:
                t = std::make_shared<AST>(GazpreaParser::LESSTHANOREQUAL);
                break;
            case GazpreaParser::GREATERTHANOREQUAL:
                t = std::make_shared<AST>(GazpreaParser::GREATERTHANOREQUAL);
                break;
            case GazpreaParser::ISEQUAL:
                t = std::make_shared<AST>(GazpreaParser::ISEQUAL);
                break;
            case GazpreaParser::ISNOTEQUAL:
                t = std::make_shared<AST>(GazpreaParser::ISNOTEQUAL);
                break;
            case GazpreaParser::AND:
                t = std::make_shared<AST>(GazpreaParser::AND);
                break;
            case GazpreaParser::OR:
                t = std::make_shared<AST>(GazpreaParser::OR);
                break;
            case GazpreaParser::XOR:
                t = std::make_shared<AST>(GazpreaParser::XOR);
                break;
            default:
                t = std::make_shared<AST>(GazpreaParser::BY);
                break;
        }
        t->addChild(visit(ctx->expr(0)));
        t->addChild(visit(ctx->expr(1)));
        return t;
    }

    std::any ASTBuilder::visitIndexing(GazpreaParser::IndexingContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::INDEXING_TOKEN);
        t->addChild(visit(ctx->expr(0)));
        t->addChild(visit(ctx->expr(1)));
        return t;
    }

    std::any ASTBuilder::visitTypedefStatement(GazpreaParser::TypedefStatementContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::TYPEDEF);
        t->addChild(visit(ctx->unqualifiedType()));
        t->addChild(visit(ctx->identifier()));
        return t;
    }

    std::any ASTBuilder::visitCast(GazpreaParser::CastContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::CAST_TOKEN);
        t->addChild(visit(ctx->unqualifiedType()));
        t->addChild(visit(ctx->expression()));
        return t;
    }

    std::any ASTBuilder::visitTupleTypeDeclarationAtom(GazpreaParser::TupleTypeDeclarationAtomContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::TUPLE_TYPE_DECLARATION_ATOM);
        for (auto singleTermType : ctx->singleTermType()) {
            t->addChild(visit(singleTermType));
        }
        return t;
    }

    std::any ASTBuilder::visitTupleTypeDeclarationList(GazpreaParser::TupleTypeDeclarationListContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::TUPLE_TYPE_DECLARATION_LIST);
        for (auto tupleTypeDeclarationAtom : ctx->tupleTypeDeclarationAtom()) {
            t->addChild(visit(tupleTypeDeclarationAtom));
        }
        return t;
    }

    std::any ASTBuilder::visitDomainExpression(GazpreaParser::DomainExpressionContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::DOMAIN_EXPRESSION_TOKEN);
        t->addChild(visit(ctx->identifier()));
        t->addChild(visit(ctx->expression()));
        return t;
    }

    std::any ASTBuilder::visitGeneratorDomainVariableList(GazpreaParser::GeneratorDomainVariableListContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::GENERATOR_DOMAIN_VARIABLE_LIST_TOKEN);
        for (auto domainExpression : ctx->domainExpression()) {
            t->addChild(visit(domainExpression));
        }
        return t;
    }

    std::any ASTBuilder::visitTupleAccessIndex(GazpreaParser::TupleAccessIndexContext *ctx) {
        if (ctx->IntegerConstant()) {
            return std::make_shared<AST>(ctx->IntegerConstant()->getSymbol());
        }
        return visit(ctx->identifier());
    }

    std::any ASTBuilder::visitIdentifier(GazpreaParser::IdentifierContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::IDENTIFIER_TOKEN);
        t->addChild(std::make_shared<AST>(ctx->getStart()));
        return t;
    }

    std::any ASTBuilder::visitRealConstant(GazpreaParser::RealConstantContext *ctx) {
        auto t = std::make_shared<AST>(GazpreaParser::REAL_CONSTANT_TOKEN); 
        try {
            t->floatValue = std::stof(ctx->getText());
        } catch (const std::exception& e) {
            std::cout << e.what(); 
        }

        return t;
    }
}