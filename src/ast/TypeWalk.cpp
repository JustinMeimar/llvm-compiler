#include "TypeWalk.h"

namespace gazprea {

    TypeWalk::TypeWalk(std::shared_ptr<SymbolTable> symtab) : symtab(symtab), currentScope(symtab->globals) {
        this->tp = std::make_shared<TypePromote>(TypePromote(this->symtab));
    }
    TypeWalk::~TypeWalk() {}

    void TypeWalk::visit(std::shared_ptr<AST> t) {
        if(!t->isNil()){
            switch(t->getNodeType()){
                //Top level exprs 
                case GazpreaParser::INDEXING_TOKEN:
                    visitIndex(t);
                    break;
                case GazpreaParser::FILTER_TOKEN:
                    visitFilter(t);
                    break;
                case GazpreaParser::GENERATOR_TOKEN:
                    visitGenerator(t);
                    break;
                case GazpreaParser::CAST_TOKEN:
                    visitCast(t);
                    break;
                case GazpreaParser::EXPRESSION_TOKEN:
                    visitExpression(t);
                    break;
                case GazpreaParser::TUPLE_ACCESS_TOKEN:
                    visitTupleAccess(t);
                    break;
                case GazpreaParser::STRING_CONCAT_TOKEN: 
                    visitStringConcat(t);
                    break;
                case GazpreaParser::VAR_DECLARATION_TOKEN:
                    visitVariableDeclaration(t);
                    break; 
                case GazpreaParser::ASSIGNMENT_TOKEN:
                    visitAssignment(t);
                    break;
                case GazpreaParser::CALL_PROCEDURE_FUNCTION_IN_EXPRESSION:
                    visitCallInExpr(t);
                    break;
            
                //Operations 
                case GazpreaParser::UNARY_TOKEN:
                    visitUnaryOp(t);
                    break;
                case GazpreaParser::BINARY_OP_TOKEN:
                    visitBinaryOp(t);
                    break;

                //Compound Literal Types
                case GazpreaParser::VECTOR_LITERAL_TOKEN:
                    visitVectorLiteral(t);
                    break;
                case GazpreaParser::TUPLE_LITERAL_TOKEN:
                    visitTupleLiteral(t);
                    break;
                case GazpreaParser::INTERVAL:
                    visitIntervalLiteral(t);
                    break;

                //Literal Types
                case GazpreaParser::IntegerConstant:
                    visitIntegerConstant(t);
                    break;
                case GazpreaParser::CharacterConstant:
                    visitCharacterConstant(t);
                    break;
                case GazpreaParser::BooleanConstant:
                    visitBooleanConstant(t);
                    break;
                case GazpreaParser::REAL_CONSTANT_TOKEN:
                    visitRealConstant(t);
                    break;
                case GazpreaParser::StringLiteral:
                    visitStringLiteral(t);
                    break;
                case GazpreaParser::IDENTIFIER_TOKEN:
                    visitIdentifier(t);
                    break;
                case GazpreaParser::IDENTITY:
                case GazpreaParser::NULL_LITERAL:
                    visitIdentityOrNull(t);
                    break;
                
                // Other Statements
                case GazpreaParser::INPUT_STREAM_TOKEN:
                case GazpreaParser::OUTPUT_STREAM_TOKEN:
                    visitStreamStatement(t);
                    break;
                case GazpreaParser::TYPEDEF:
                    visitTypedefStatement(t);
                    break;
                default:
                    visitChildren(t);
            };
        }
        else {
            visitChildren(t); // t must be the null root node 
        }
    }

    void TypeWalk::visitChildren(std::shared_ptr<AST> t) {
        for( auto child : t->children) visit(child);
    }
    
    void TypeWalk::visitVariableDeclaration(std::shared_ptr<AST> t) { 
        visitChildren(t);
        if (t->children[2]->isNil()) { return; } // Decl w/ no def 
        if (t->children[0]->getNodeType() == GazpreaParser::INFERRED_TYPE_TOKEN) {
            auto variableDeclarationSymbol = std::dynamic_pointer_cast<VariableSymbol>(t->symbol);
            variableDeclarationSymbol->type = t->children[2]->evalType;
            return;
        }
        
        auto varTy = t->children[1]->evalType;  // Filled in by visitChildren(t), and specifically, visitIdentifier(t);
        auto exprTy = t->children[2]->evalType;
        
        if (exprTy->getTypeId() == Type::IDENTITYNULL) {
            t->children[2]->promoteToType = varTy;

        } else if (varTy->isTupleType() && exprTy->isTupleType()) {  //specifically for tuple
            auto tupleTypeInTypeSpecifier = std::dynamic_pointer_cast<TupleType>(varTy);
            auto tupleTypeInExpression = std::dynamic_pointer_cast<TupleType>(exprTy);
            if (tupleTypeInTypeSpecifier->orderedArgs.size() != tupleTypeInExpression->orderedArgs.size()) {
                std::cout << "Compile-time error!" << std::endl;
            }
            for (size_t i = 0; i < tupleTypeInExpression->orderedArgs.size(); i++) {
                int exprTupleElTy = tupleTypeInExpression->orderedArgs[i]->type->getTypeId();
                int varTupleElTy =  tupleTypeInTypeSpecifier->orderedArgs[i]->type->getTypeId();
                int promote = tp->promotionFromTo[exprTupleElTy][varTupleElTy]; 
                if (promote != 0) {
                    t->children[2]->tuplePromoteTypeList[i] = symtab->getType(promote);
                }
            }
        } else { //all other variable types
            int varTyEnum = varTy->getTypeId();
            int exprTyEnum = exprTy->getTypeId(); 

            int promote = tp->promotionFromTo[exprTyEnum][varTyEnum];
            if(promote != 0) {
                t->children[2]->promoteToType = t->children[1]->evalType;
            }
        }
    }

    void TypeWalk::visitAssignment(std::shared_ptr<AST> t) {
        visitChildren(t);
        auto RHSTy = t->children[1]->evalType;
        auto numLHSExpressions = t->children[0]->children.size();
        if (numLHSExpressions == 1) {
            auto LHSExpressionAST = t->children[0]->children[0];

            if (LHSExpressionAST->evalType->getTypeId() == Type::TUPLE && RHSTy->getTypeId() == Type::TUPLE) {
                auto tupleTypeInRHSExpression = std::dynamic_pointer_cast<TupleType>(RHSTy);
                auto tupleTypeInLHSExpression = std::dynamic_pointer_cast<TupleType>(LHSExpressionAST->evalType);

                if (tupleTypeInRHSExpression->orderedArgs.size() != tupleTypeInLHSExpression->orderedArgs.size()) {
                    std::cout << "Compile-time error!" << std::endl;
                    return;
                }

                for (size_t i = 0; i < tupleTypeInRHSExpression->orderedArgs.size(); i++) {
                    int promote = tp->promotionFromTo[tupleTypeInRHSExpression->orderedArgs[i]->type->getTypeId()][tupleTypeInLHSExpression->orderedArgs[i]->type->getTypeId()];
                    if (promote != 0) {
                        t->children[1]->tuplePromoteTypeList[i] = tupleTypeInLHSExpression->orderedArgs[i]->type;
                    }
                }
            } else if (RHSTy->getTypeId() == Type::IDENTITYNULL) { //assign promoteToType of IDENTITY to LHS Type
                t->children[1]->promoteToType = LHSExpressionAST->evalType;
            } else {
                int LHSTyEnum = LHSExpressionAST->evalType->getTypeId();
                int RHSTyEnum = RHSTy->getTypeId();
                int promote = tp->promotionFromTo[RHSTyEnum][LHSTyEnum];
                if (promote != 0) {
                    t->children[1]->promoteToType = LHSExpressionAST->evalType;
                }
            }
        } else {
            // Tuple Parallel Assignment
            if (RHSTy->getTypeId() != Type::TUPLE) {
                std::cout << "Compile-time error! - RHS must be a tuple in Parallel Assignment" << std::endl;
                return;
            }
            // RHS must be a tuple
            auto tupleTypeInRHSExpression = std::dynamic_pointer_cast<TupleType>(RHSTy);
            if (numLHSExpressions != tupleTypeInRHSExpression->orderedArgs.size()) {
                std::cout << "Compile-time error! - Incompatible type assignment" << std::endl;
                return;
            }
            for (size_t i = 0; i < numLHSExpressions; i++) {
                auto LHSExpressionAtomAST = t->children[0]->children[i];
                int promote = tp->promotionFromTo[tupleTypeInRHSExpression->orderedArgs[i]->type->getTypeId()][LHSExpressionAtomAST->evalType->getTypeId()];
                if (promote != 0) {
                    t->children[1]->tuplePromoteTypeList[i] = LHSExpressionAtomAST->evalType;
                }
            }
        }
    }
    
    void TypeWalk::visitIndex(std::shared_ptr<AST> t) {
        visitChildren(t);
        switch (t->children[0]->evalType->getTypeId()) {
            case Type::BOOLEAN_1:
                t->evalType = symtab->getType(Type::BOOLEAN);
                break;
            case Type::CHARACTER_1:
                t->evalType = symtab->getType(Type::CHARACTER);
                break;
            case Type::INTEGER_1:
                t->evalType = symtab->getType(Type::INTEGER);
                break;
            case Type::REAL_1:
                t->evalType = symtab->getType(Type::REAL);
                break;
            case Type::BOOLEAN_2:
                t->evalType = symtab->getType(Type::BOOLEAN_1);
                break;
            case Type::CHARACTER_2:
                t->evalType = symtab->getType(Type::CHARACTER_1);
                break;
            case Type::INTEGER_2:
                t->evalType = symtab->getType(Type::INTEGER_1);
                break;
            case Type::REAL_2:
                t->evalType = symtab->getType(Type::REAL_1);
                break;
            default:
                break;
        }
        t->promoteToType = nullptr;

    }

    void TypeWalk::visitFilter(std::shared_ptr<AST> t) {
        visitChildren(t);
        t->evalType = symtab->getType(Type::INTEGER_1);
        t->promoteToType = nullptr;
    }

    void TypeWalk::visitGenerator(std::shared_ptr<AST> t) {
        visitChildren(t);
        t->evalType = symtab->getType(Type::INTEGER_1);
        t->promoteToType = nullptr;
    }

    void TypeWalk::visitCast(std::shared_ptr<AST> t) {
        visitChildren(t);
        t->evalType = t->children[0]->type;
        t->promoteToType = nullptr;
    }
    
    void TypeWalk::visitTupleAccess(std::shared_ptr<AST> t) {
        visit(t->children[0]);
        auto tupleType = std::dynamic_pointer_cast<TupleType>(t->children[0]->evalType);
        if (t->children[1]->getNodeType() == GazpreaParser::IDENTIFIER_TOKEN) {
            auto argSymbol = tupleType->resolve(t->children[1]->parseTree->getText());
            t->evalType = argSymbol->type;
        } else {
            auto index = std::stoi(t->children[1]->parseTree->getText());
            t->evalType = tupleType->orderedArgs[index - 1]->type;
        }
        t->promoteToType = nullptr;
    }

    void TypeWalk::visitExpression(std::shared_ptr<AST> t) {
        visitChildren(t);
        t->evalType = t->children[0]->evalType;
        t->promoteToType = nullptr; 
        if (t->evalType != nullptr && t->evalType->getTypeId() == Type::TUPLE) {
            auto tupleType = std::dynamic_pointer_cast<TupleType>(t->evalType);
            t->tuplePromoteTypeList = std::vector<std::shared_ptr<Type>>(tupleType->orderedArgs.size());
        }
    } 

    void TypeWalk::visitStringConcat(std::shared_ptr<AST> t) {
        visitChildren(t);
        t->evalType = std::dynamic_pointer_cast<Type>(symtab->globals->resolve("string"));
        t->promoteToType = nullptr;
    }

    void TypeWalk::visitCallInExpr(std::shared_ptr<AST> t) {
        visitChildren(t);
        auto sbrtSymbol = symtab->globals->resolve(t->children[0]->getText()); //get the subroutine symbol
        t->evalType = sbrtSymbol->type;  //will be the return type
        t->promoteToType = nullptr;
    }
    
    void TypeWalk::visitBinaryOp(std::shared_ptr<AST> t) {
        visitChildren(t); 
        auto node1 = t->children[0];
        auto node2 = t->children[1]; 
        
        //getResultType automatically populates promotType of children
        switch(t->children[2]->getNodeType()){ 
            case GazpreaParser::MODULO:
            case GazpreaParser::XOR:
            case GazpreaParser::AND:
                t->evalType = symtab->getType(Type::INTEGER);
            case GazpreaParser::PLUS:
            case GazpreaParser::MINUS:
            case GazpreaParser::DIV:
            case GazpreaParser::ASTERISK: 
                t->evalType = tp->getResultType(tp->arithmeticResultType, node1, node2);
            break;
            case GazpreaParser::LESSTHAN:
            case GazpreaParser::GREATERTHAN:
            case GazpreaParser::LESSTHANOREQUAL:
            case GazpreaParser::GREATERTHANOREQUAL:
                t->evalType = tp->getResultType(tp->relationalResultType, node1, node2);
            break;
            case GazpreaParser::ISEQUAL:
            case GazpreaParser::ISNOTEQUAL:
                t->evalType = tp->getResultType(tp->equalityResultType, node1, node2);
            break; 
        }     
    }

    void TypeWalk::visitUnaryOp(std::shared_ptr<AST> t) {
        visitChildren(t);
        t->evalType = t->children[1]->evalType;
        t->promoteToType = nullptr;
    }

    //Compound Types
    void TypeWalk::visitVectorLiteral(std::shared_ptr<AST> t) {
        visitChildren(t);
        if (t->children[0]->children.size() == 0) { return; } //null vector 
        if (t->children[0]->children[0]->children[0]->getNodeType() == GazpreaParser::VECTOR_LITERAL_TOKEN) {
            //literal matrix
            auto baseType =  t->children[0]->children[0]->children[0]->children[0]->children[0]->evalType;
            t->evalType = std::make_shared<MatrixType>(MatrixType(baseType, 2, t));
        } else {
            //literal vector
            auto baseType = t->children[0]->children[0]->children[0]->evalType;
            t->evalType = std::make_shared<MatrixType>(MatrixType(baseType, 1, t));
        }
        t->promoteToType = nullptr;
    }

    void TypeWalk::visitTupleLiteral(std::shared_ptr<AST> t) {
        visitChildren(t);
        size_t tupleSize = t->children[0]->children.size();
        auto tupleType = std::make_shared<TupleType>(currentScope, t, tupleSize);
        t->evalType = tupleType;
        //populate orderedArgs
        for(size_t i = 0; i < tupleSize; i++) { 
            auto tupleMember = t->children[0]->children[i]; //AST of member
            tupleType->orderedArgs.push_back(std::make_shared<VariableSymbol>("", tupleMember->evalType));
        }
        t->tuplePromoteTypeList = std::vector<std::shared_ptr<Type>>(tupleSize);  // Initialize all elements to nullptr
        t->promoteToType = nullptr;  // will not be used
    }

    void TypeWalk::visitIntervalLiteral(std::shared_ptr<AST> t) {
        t->evalType = symtab->getType(Type::INTEGER_INTERVAL);
        t->promoteToType = nullptr;
    }
    
    //Terminal Types
    void TypeWalk::visitIntegerConstant(std::shared_ptr<AST> t) {
        t->evalType = symtab->getType(Type::INTEGER);
        t->promoteToType = nullptr;
    }
    
    void TypeWalk::visitRealConstant(std::shared_ptr<AST> t) {
        t->evalType = symtab->getType(Type::REAL);
        t->promoteToType = nullptr; 
    }
    
    void TypeWalk::visitCharacterConstant(std::shared_ptr<AST> t) {
        t->evalType = symtab->getType(Type::CHARACTER);
        t->promoteToType = nullptr;
    }
    
    void TypeWalk::visitBooleanConstant(std::shared_ptr<AST> t) {
        t->evalType = symtab->getType(Type::BOOLEAN);
        t->promoteToType = nullptr;
    }

    void TypeWalk::visitStringLiteral(std::shared_ptr<AST> t) {
        t->evalType = symtab->getType(Type::STRING);
        t->promoteToType = nullptr;
    }

    void TypeWalk::visitIdentifier(std::shared_ptr<AST> t) {
        t->evalType = t->symbol->type;
        t->promoteToType = nullptr;
        if (t->evalType != nullptr && t->evalType->getTypeId() == Type::TUPLE) {
            auto tupleType = std::dynamic_pointer_cast<TupleType>(t->evalType);
            t->tuplePromoteTypeList = std::vector<std::shared_ptr<Type>>(tupleType->orderedArgs.size());
        }
    }
    
    void TypeWalk::visitIdentityOrNull(std::shared_ptr<AST> t) {
        t->evalType = symtab->getType(Type::IDENTITYNULL); //esentially until used in a context 
        t->promoteToType = nullptr;
    }

    void TypeWalk::visitStreamStatement(std::shared_ptr<AST> t) {
        visit(t->children[0]);  // Only visiting the expression, not visiting the identifier
    }

    void TypeWalk::visitTypedefStatement(std::shared_ptr<AST> t) {
        visit(t->children[0]);
    }
    
} // namespace gazprea 