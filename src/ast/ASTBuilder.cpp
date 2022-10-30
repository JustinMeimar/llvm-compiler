#include "ASTBuilder.h"

namespace gazprea {

    ASTBuilder::ASTBuilder() {}
    ASTBuilder::~ASTBuilder() {}

    std::any ASTBuilder::visitCompilationUnit(GazpreaParser::CompilationUnitContext *ctx) {
        std::shared_ptr<AST> t = std::make_shared<AST>();
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
        std::shared_ptr<AST> t = std::make_shared<AST>(GazpreaParser::VECTOR_SIZE_DECLARATION_LIST_TOKEN);
        for (auto vectorSizeDeclarationAtom : ctx->vectorSizeDeclarationAtom()) {
            t->addChild(visit(vectorSizeDeclarationAtom));
        }
        return t;
    }

    std::any ASTBuilder::visitExplcitType(GazpreaParser::ExplcitTypeContext *ctx){
        std::shared_ptr<AST> t = std::make_shared<AST>(GazpreaParser::TYPE_TOKEN);
        if (ctx->typeQualifier()) {
            t->addChild(visit(ctx->typeQualifier()));
        }
        t->addChild(visit(ctx->unqualifiedType()));
        return visitChildren(ctx);
    }

    std::any ASTBuilder::visitInferredType(GazpreaParser::InferredTypeContext *ctx){
        std::shared_ptr<AST> t = std::make_shared<AST>(GazpreaParser::TYPE_TOKEN);
        t->addChild(visit(ctx->typeQualifier()));
        return t;
    }

    std::any ASTBuilder::visitTypeQualifier(GazpreaParser::TypeQualifierContext *ctx) {
        std::shared_ptr<AST> t = std::make_shared<AST>(GazpreaParser::TYPE_QUALIFIER_TOKEN);
        t->addChild(ctx->getStart());
        return t;
    }

    std::any ASTBuilder::visitVectorMatrixType(GazpreaParser::VectorMatrixTypeContext *ctx){ 
        std::shared_ptr<AST> t = std::make_shared<AST>(GazpreaParser::VECTOR_TYPE_TOKEN);
        t->addChild(visit(ctx->singleTokenType()));
        t->addChild(visit(ctx->vectorSizeDeclarationList())); 
        return t;
    }
 
    std::any ASTBuilder::visitTupleType(GazpreaParser::TupleTypeContext *ctx){
        std::shared_ptr<AST> t = std::make_shared<AST>(GazpreaParser::TUPLE_TYPE_TOKEN);
        t->addChild(visit(ctx->tupleTypeDeclarationList()));
        return t;
    }

    std::any ASTBuilder::visitSingleTokenType(GazpreaParser::SingleTokenTypeContext *ctx){
        std::shared_ptr<AST> t = std::make_shared<AST>(GazpreaParser::SCALAR_TYPE_TOKEN);
        t->addChild(std::make_shared<AST>(ctx->getStart()));
        return t;
    }

    std::any ASTBuilder::visitSingleTokenTypeAtom(GazpreaParser::SingleTokenTypeAtomContext *ctx){
        return visit(ctx->singleTokenType());
    }
    
    std::any ASTBuilder::visitUnqualifiedType(GazpreaParser::UnqualifiedTypeContext *ctx){
        std::shared_ptr<AST> t = std::make_shared<AST>(GazpreaParser::TYPE_TOKEN);
        for (auto* singleType : ctx->singleTermType()){ 
            t->addChild(visit(singleType));
        } 
        return t;
    }

    std::any ASTBuilder::visitVarDeclarationStatement(GazpreaParser::VarDeclarationStatementContext *ctx) {
        std::shared_ptr<AST> t = std::make_shared<AST>(GazpreaParser::TYPEDEF_VAR_DECLARATION_TOKEN);
        t->addChild(visit(ctx->anyType())); 
        t->addChild(std::make_shared<AST>(ctx->Identifier()->getSymbol())); //typedef id
        if (ctx->expression()) {
            t->addChild(visit(ctx->expression()));
        }
        return t;
    }

    std::any ASTBuilder::visitAssignmentStatement(GazpreaParser::AssignmentStatementContext *ctx) {
        std::shared_ptr<AST> t = std::make_shared<AST>(GazpreaParser::ASSIGNMENT_TOKEN);
        t->addChild(visit(ctx->expressionList()));
        t->addChild(visit(ctx->expression()));
        return t;
    }

    std::any ASTBuilder::visitExpressionList(GazpreaParser::ExpressionListContext *ctx) {
        std::shared_ptr<AST> t = std::make_shared<AST>(GazpreaParser::EXPRESSION_LIST_TOKEN);
        for (auto expression: ctx->expression()) {
            t->addChild(visit(expression));
        }
        return t;
    }

    std::any ASTBuilder::visitFormalParameter(GazpreaParser::FormalParameterContext *ctx) {
        std::shared_ptr<AST> t = std::make_shared<AST>(GazpreaParser::FORMAL_PARAMETER_TOKEN);
        t->addChild(visit(ctx->anyType()));
        t->addChild(std::make_shared<AST>(ctx->Identifier()->getSymbol()));
        return t;
    }

    std::any ASTBuilder::visitFormalParameterList(GazpreaParser::FormalParameterListContext *ctx) {
        std::shared_ptr<AST> t = std::make_shared<AST>(GazpreaParser::FORMAL_PARAMETER_LIST_TOKEN);
        for (auto formalParameter : ctx->formalParameter()) {
            t->addChild(visit(formalParameter));
        }
        return t;
    }
   
    std::any ASTBuilder::visitSubroutineDeclDef(GazpreaParser::SubroutineDeclDefContext *ctx) {
        std::shared_ptr<AST> t;
        if (ctx->PROCEDURE()) {
            t = std::make_shared<AST>(GazpreaParser::PROCEDURE);
        } else if (ctx->FUNCTION()) {
            t = std::make_shared<AST>(GazpreaParser::FUNCTION);
        }
        t->addChild(std::make_shared<AST>(ctx->Identifier()->getSymbol()));
        if (ctx->formalParameterList()) {
            t->addChild(visit(ctx->formalParameterList()));
        } 
        t->addChild(visit(ctx->unqualifiedType()));
        
        return t;
    }

    std::any ASTBuilder::visitFunctionEmptyBody(GazpreaParser::FunctionEmptyBodyContext *ctx){

    }

    std::any ASTBuilder::visitFunctionExprBody(GazpreaParser::FunctionExprBodyContext *ctx){

    }

    std::any ASTBuilder::visitFunctionBlockBody(GazpreaParser::FunctionBlockBodyContext *ctx){

    }

    std::any ASTBuilder::visitReturnStatement(GazpreaParser::ReturnStatementContext *ctx){
        std::shared_ptr<AST> t = std::make_shared<AST>(GazpreaParser::RETURN);
        t->addChild(visit(ctx->expression()));
        return t;
    }

    std::any ASTBuilder::visitCallProcedure(GazpreaParser::CallProcedureContext *ctx) {
        std::shared_ptr<AST> t = std::make_shared<AST>(GazpreaParser::CALL_PROCEDURE_STATEMENT);
        t->addChild(std::make_shared<AST>(ctx->Identifier()->getSymbol()));
        t->addChild(visit(ctx->expressionList()));
        return t;
    }

    std::any ASTBuilder::visitConditionalStatement(GazpreaParser::ConditionalStatementContext *ctx) {
        std::shared_ptr<AST> t = std::make_shared<AST>(GazpreaParser::CONDITIONAL_TOKEN);
        t->addChild(visit(ctx->expression()));

        if (ctx->statement()) {
            t->addChild(visit(ctx->statement()));
        }  
        for (auto* elseIfStatement : ctx->elseIfStatement()) {
            t->addChild(visit(elseIfStatement));
        }
        if (ctx->elseStatement()) {
            t->addChild(visit(ctx->elseStatement()));
        }
        return t;
    }

    std::any ASTBuilder::visitElseIfStatement(GazpreaParser::ElseIfStatementContext *ctx) {
        std::shared_ptr<AST> t = std::make_shared<AST>(GazpreaParser::ELSEIF_TOKEN);
        t->addChild(visit(ctx->expression()));
        if (ctx->statement()) {
            t->addChild(visit(ctx->statement()));
        }
        return t;
    }

    std::any ASTBuilder::visitElseStatement(GazpreaParser::ElseStatementContext *ctx) {
        std::shared_ptr<AST> t = std::make_shared<AST>(GazpreaParser::ELSE_TOKEN);
        if (ctx->statement()) {
            t->addChild(visit(ctx->statement()));
        }
        return t;
    }

    std::any ASTBuilder::visitInfiniteLoopStatement(GazpreaParser::InfiniteLoopStatementContext *ctx) {
        std::shared_ptr<AST> t = std::make_shared<AST>(GazpreaParser::INFINITE_LOOP_TOKEN);
        if (ctx->statement()) {
            t->addChild(visit(ctx->statement()));
        }
        return t;
    }

    std::any ASTBuilder::visitPrePredicatedLoopStatement(GazpreaParser::PrePredicatedLoopStatementContext *ctx) {
        std::shared_ptr<AST> t = std::make_shared<AST>(GazpreaParser::PRE_PREDICATE_LOOP_TOKEN);
        t->addChild(visit(ctx->expression()));
        if (ctx->statement()) {
            t->addChild(visit(ctx->statement()));
        }
        return t;
    }

    std::any ASTBuilder::visitPostPredicatedLoopStatement(GazpreaParser::PostPredicatedLoopStatementContext *ctx) {
        std::shared_ptr<AST> t = std::make_shared<AST>(GazpreaParser::POST_PREDICATE_LOOP_TOKEN);
        t->addChild(visit(ctx->statement()));
        if (ctx->statement()) {
            t->addChild(visit(ctx->expression()));
        }
        return t;
    }

    std::any ASTBuilder::visitIteratorLoopStatement(GazpreaParser::IteratorLoopStatementContext *ctx) {
        std::shared_ptr<AST> t = std::make_shared<AST>(GazpreaParser::ITERATOR_LOOP_TOKEN);
        
        return t;
    }

    std::any ASTBuilder::visitBreakStatement(GazpreaParser::BreakStatementContext *ctx) {
        return std::make_shared<AST>(GazpreaParser::BREAK);
    }

    std::any ASTBuilder::visitContinueStatement(GazpreaParser::ContinueStatementContext *ctx) {
        return std::make_shared<AST>(GazpreaParser::CONTINUE);
    }

    std::any ASTBuilder::visitOutputStream(GazpreaParser::OutputStreamContext *ctx) {
        std::shared_ptr<AST> t = std::make_shared<AST>(GazpreaParser::OUTPUT_STREAM_TOKEN);
        t->addChild(visit(ctx->expression()));
        return t;
    }

    std::any ASTBuilder::visitInputStream(GazpreaParser::InputStreamContext *ctx) {
        std::shared_ptr<AST> t = std::make_shared<AST>(GazpreaParser::INPUT_STREAM_TOKEN);
        t->addChild(visit(ctx->expression()));
        return t;
    }

    std::any ASTBuilder::visitBlock(GazpreaParser::BlockContext *ctx) {
        std::shared_ptr<AST> t = std::make_shared<AST>(GazpreaParser::BLOCK_TOKEN);
        for (auto *stat : ctx->statement()) {
            t->addChild(visit(stat));
        }
        return t;
    }

    std::any ASTBuilder::visitExpression(GazpreaParser::ExpressionContext *ctx) {
        std::shared_ptr<AST> t = std::make_shared<AST>(GazpreaParser::EXPR_TOKEN);
        t->addChild(visit(ctx->expr()));
        return t;
    }

    std::any ASTBuilder::visitTupleAccess(GazpreaParser::TupleAccessContext *ctx) {
        std::shared_ptr<AST> t = std::make_shared<AST>(GazpreaParser::TUPLE_ACCESS_TOKEN);
        t->addChild(visit(ctx->expr(0)));
        t->addChild(visit(ctx->expr(1)));
        return t;
    }

    std::any ASTBuilder::visitRealAtom(GazpreaParser::RealAtomContext *ctx) {
        return std::make_shared<AST>(ctx->RealConstant()->getSymbol());
    }

    std::any ASTBuilder::visitCallProcedureFunctionInExpression(GazpreaParser::CallProcedureFunctionInExpressionContext *ctx) {
        std::shared_ptr<AST> t = std::make_shared<AST>(GazpreaParser::CALL_PROCEDURE_FUNCTION_IN_EXPRESSION);
        t->addChild(std::make_shared<AST>(ctx->Identifier()->getSymbol()));
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
        switch (ctx->op->getType())
        {
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
        std::shared_ptr<AST> t = std::make_shared<AST>(GazpreaParser::STRING_CONCAT_TOKEN);
        t->addChild(visit(ctx->expr(0)));
        t->addChild(visit(ctx->expr(1)));
        return t;
    }

    std::any ASTBuilder::visitFilter(GazpreaParser::FilterContext *ctx) {
        std::shared_ptr<AST> t = std::make_shared<AST>(GazpreaParser::FILTER_TOKEN);
        t->addChild(std::make_shared<AST>(ctx->Identifier()->getSymbol()));
        t->addChild(visit(ctx->expression()));
        t->addChild(visit(ctx->expressionList()));
        return t;
    }

    std::any ASTBuilder::visitStringLiteralAtom(GazpreaParser::StringLiteralAtomContext *ctx) {
        return std::make_shared<AST>(ctx->StringLiteral()->getSymbol());
    }

    std::any ASTBuilder::visitTupleLiteral(GazpreaParser::TupleLiteralContext *ctx) {
        std::shared_ptr<AST> t = std::make_shared<AST>(GazpreaParser::TUPLE_LITERAL_TOKEN);
        t->addChild(visit(ctx->expressionList()));
        return t;
    }

    std::any ASTBuilder::visitVectorLiteral(GazpreaParser::VectorLiteralContext *ctx){
        std::shared_ptr<AST> t = std::make_shared<AST>(GazpreaParser::VECTOR_LITERAL_TOKEN);
        t->addChild(visit(ctx->expressionList()));
        return t;
    }

    std::any ASTBuilder::visitIdentifierAtom(GazpreaParser::IdentifierAtomContext *ctx) {
        return std::make_shared<AST>(ctx->Identifier()->getSymbol());
    }

    std::any ASTBuilder::visitCharacterAtom(GazpreaParser::CharacterAtomContext *ctx) {
        return std::make_shared<AST>(ctx->CharacterConstant()->getSymbol());
    }

    std::any ASTBuilder::visitGenerator(GazpreaParser::GeneratorContext *ctx) {
        std::shared_ptr<AST> t = std::make_shared<AST>(GazpreaParser::GENERATOR_TOKEN);
        t->addChild(visit(ctx->generatorDomainVariableList()));
        t->addChild(visit(ctx->expression()));
        return t;
    }

    std::any ASTBuilder::visitInterval(GazpreaParser::IntervalContext *ctx) {
        std::shared_ptr<AST> t = std::make_shared<AST>(GazpreaParser::INTERVAL);
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
        std::shared_ptr<AST> t = std::make_shared<AST>(GazpreaParser::INDEXING_TOKEN);
        t->addChild(visit(ctx->expr(0)));
        t->addChild(visit(ctx->expr(1)));
        return t;
    }

    std::any ASTBuilder::visitTypedefStatement(GazpreaParser::TypedefStatementContext *ctx) {
        std::shared_ptr<AST> t = std::make_shared<AST>(GazpreaParser::TYPEDEF);
        t->addChild(visit(ctx->unqualifiedType()));
        t->addChild(std::make_shared<AST>(ctx->Identifier()->getSymbol()));
        return t;
    }

    std::any ASTBuilder::visitCast(GazpreaParser::CastContext *ctx) {
        std::shared_ptr<AST> t = std::make_shared<AST>(GazpreaParser::CAST_TOKEN);
        t->addChild(visit(ctx->unqualifiedType()));
        t->addChild(visit(ctx->expression()));
        return t;
    }

    std::any ASTBuilder::visitTupleTypeDeclarationAtom(GazpreaParser::TupleTypeDeclarationAtomContext *ctx) {
        std::shared_ptr<AST> t = std::make_shared<AST>(GazpreaParser::TUPLE_TYPE_DECLARATION_ATOM);
        t->addChild(visit(ctx->unqualifiedType()));
        if (ctx->Identifier()) {
            t->addChild(std::make_shared<AST>(ctx->Identifier()->getSymbol()));
        }
        return t;
    }

    std::any ASTBuilder::visitTupleTypeDeclarationList(GazpreaParser::TupleTypeDeclarationListContext *ctx) {
        std::shared_ptr<AST> t = std::make_shared<AST>(GazpreaParser::TUPLE_TYPE_DECLARATION_LIST);
        for (auto tupleTypeDeclarationAtom : ctx->tupleTypeDeclarationAtom()) {
            t->addChild(visit(tupleTypeDeclarationAtom));
        }
        return t;
    }

    std::any ASTBuilder::visitDomainExpression(GazpreaParser::DomainExpressionContext *ctx) {
        //
    }

    std::any ASTBuilder::visitGeneratorDomainVariableList(GazpreaParser::GeneratorDomainVariableListContext *ctx) {
        //
    }
}