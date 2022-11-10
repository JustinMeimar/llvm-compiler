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
                case GazpreaParser::INDEXING_TOKEN: visitIndex(t); break;
                case GazpreaParser::FILTER_TOKEN: visitFilter(t); break;
                case GazpreaParser::GENERATOR_TOKEN: visitGenerator(t); break;
                case GazpreaParser::CAST_TOKEN: visitCast(t); break;
                case GazpreaParser::EXPRESSION_TOKEN: visitExpression(t);  break;
                case GazpreaParser::TUPLE_ACCESS_TOKEN: visitTupleAccess(t); break;
                case GazpreaParser::STRING_CONCAT_TOKEN: visitStringConcat(t);break;
                case GazpreaParser::VAR_DECLARATION_TOKEN: visitVariableDeclaration(t); break; 
                case GazpreaParser::CALL_PROCEDURE_FUNCTION_IN_EXPRESSION: visitCallInExpr(t); break;
            
                //Operations 
                case GazpreaParser::UNARY_TOKEN: visitUnaryOp(t); break;
                case GazpreaParser::BINARY_OP_TOKEN: visitBinaryOp(t); break;

                //Compound Literal Types
                case GazpreaParser::VECTOR_LITERAL_TOKEN: visitVectorLiteral(t); break;
                case GazpreaParser::TUPLE_LITERAL_TOKEN: visitTupleLiteral(t); break;
                case GazpreaParser::INTERVAL: visitIntervalLiteral(t); break;

                //Literal Types
                case GazpreaParser::IntegerConstant: visitIntegerConstant(t); break;
                case GazpreaParser::CharacterConstant: visitCharacterConstant(t); break;
                case GazpreaParser::BooleanConstant: visitBooleanConstant(t); break;
                case GazpreaParser::REAL_CONSTANT_TOKEN: visitRealConstant(t); break;
                case GazpreaParser::StringLiteral: visitStringLiteral(t); break;
                case GazpreaParser::IDENTIFIER_TOKEN: visitIdentifier(t); break;
                
                default: visitChildren(t);
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
        //example of function use: character[3] cvec = 'c'; (promote char -> vec of char);
        visitChildren(t);
        auto varTy = t->children[1]->evalType; 
        auto exprTy = t->children[2]->evalType;

        if (varTy->isTupleType() && exprTy->isTupleType()) {  //specifically for tuple

            auto varTuple = t->children[1];

            if (t->children[2]->children[0]->getNodeType() == GazpreaParser::TUPLE_LITERAL_TOKEN) {
                auto exprTupleLiteral = t->children[2]->children[0];
                std::cout << exprTupleLiteral->TuplePromoteTypeList.size() << std::endl; 
            } 
            else if (t->children[2]->children[0]->getNodeType() == GazpreaParser::IDENTIFIER_TOKEN) {
                auto exprTupleId = t->children[2]->children[0]; 
                std::cout << exprTupleId->TuplePromoteTypeList.size() << std::endl; 
            }
            // std::cout << t->children[2]->getNodeType();
            // std::cout << t->children[2]->children[0]->getNodeType();
            // std::cout << exprTuple->TuplePromoteTypeList.size();
            // auto varTuple  = std::dynamic_pointer_cast<TupleType>(varTy); 
            // auto exprTuple = std::dynamic_pointer_cast<TupleType>(exprTy);
            
            // std::shared_ptr<AST> exprNode = t->children[2]->children[0]; 
            // for(int i=0; i < varTuple->size; i++) {
            //     auto fromType = exprTuple->orderedArgs[i]->type->getTypeId();
            //     auto toType   = varTuple->orderedArgs[i]->type->getTypeId();
            //     int promote = tp->promotionFromTo[fromType][toType]; 

            //     // std::cout << t->children[2]->children[0]->getNodeType() << std::endl;
            //     std::shared_ptr<AST> exprTupleMember; 
            //     if (exprNode->nodeType == GazpreaParser::TUPLE_LITERAL_TOKEN) {
            //         exprTupleMember = exprNode->children[0]->children[i]; 
            //     } else if (exprNode->nodeType == GazpreaParser::IDENTIFIER_TOKEN) {
            //         auto tupleSymb = currentScope->resolve(exprNode->getText());
            //         exprTupleMember = tupleSymb->def->children[2]->children[0]->children[0]->children[i]->children[0];
            //     }
            
            //     if (promote != 0) {
            //         std::cout << "promoted index " << i << " : " <<promote << std::endl;
            //         exprTupleMember->promoteType = varTuple->orderedArgs[i]->type;
            //     }
            // }
        } else { //all other variable types
            int varTyEnum = varTy->getTypeId();
            int exprTyEnum = exprTy->getTypeId(); 

            int promote = tp->promotionFromTo[exprTyEnum][varTyEnum];
            if(promote != 0) {
                t->children[2]->promoteType = t->children[1]->evalType;
                //uncomment to printout the promotion 
                std::cout   << "VarDecl promotion:\t"
                            << t->children[1]->parseTree->getText() <<  " of type "
                            << t->children[1]->evalType->getTypeId() <<  " caused promotion of "
                            << t->children[2]->parseTree->getText() << " of type " 
                            << t->children[2]->evalType->getTypeId() << " into type " 
                            << t->children[2]->promoteType->getTypeId() << std::endl;    
            }
        }
    }
    
    void TypeWalk::visitIndex(std::shared_ptr<AST> t) {
        visitChildren(t);
        // auto vectorType = t->children[0]->evalType;
    }

    void TypeWalk::visitFilter(std::shared_ptr<AST> t) {
        visitChildren(t);
        t->evalType = std::dynamic_pointer_cast<Type>(symtab->globals->resolve("integer"));
        t->promoteType = nullptr;
    }

    void TypeWalk::visitGenerator(std::shared_ptr<AST> t) {
        visitChildren(t);
        t->evalType = std::dynamic_pointer_cast<Type>(symtab->globals->resolve("integer"));
        t->promoteType = nullptr;
    }

    void TypeWalk::visitCast(std::shared_ptr<AST> t) {
        visitChildren(t);
    }
    
    void TypeWalk::visitTupleAccess(std::shared_ptr<AST> t) {
        visitChildren(t);
    }

    void TypeWalk::visitExpression(std::shared_ptr<AST> t) {
        visitChildren(t);
        t->evalType = t->children[0]->evalType;
        t->promoteType = nullptr; 
    } 

    void TypeWalk::visitStringConcat(std::shared_ptr<AST> t) {
        visitChildren(t);
        t->evalType = std::dynamic_pointer_cast<Type>(symtab->globals->resolve("string"));
        t->promoteType = nullptr;
    }

    void TypeWalk::visitCallInExpr(std::shared_ptr<AST> t) {
        visitChildren(t);
        auto sbrtSymbol = symtab->globals->resolve(t->children[0]->getText()); //get the subroutine symbol
        t->evalType = sbrtSymbol->type; //will be the return type
        t->promoteType = nullptr;
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
         //uncomment to print what is promoted
        if (node1->promoteType != nullptr) {
            std::cout << "BinaryOp promotion:\tpromote node1 " << node1->parseTree->getText() << " to type: "<< node1->promoteType->getTypeId() << std::endl;
        } else if (node2->promoteType != nullptr) {
            std::cout << "BinaryOp promotion:\tpromote node2 " << node2->parseTree->getText() << " to type: "<< node2->promoteType->getTypeId() << std::endl; 
        }
    }

    void TypeWalk::visitUnaryOp(std::shared_ptr<AST> t) {
        visitChildren(t);
        t->evalType = t->children[1]->evalType;
        t->promoteType = nullptr;
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
        t->promoteType = nullptr;
    }

    void TypeWalk::visitTupleLiteral(std::shared_ptr<AST> t) {
        visitChildren(t);
        size_t tupleSize = t->children[0]->children.size();
        t->evalType = std::make_shared<TupleType>(TupleType(currentScope, t, tupleSize));
        //populate orderedArgs
        auto tupleEvalType = std::dynamic_pointer_cast<TupleType>(t->evalType);
        for(size_t i = 0; i < tupleSize; i++) { 
            auto tupleMember = t->children[0]->children[i]; //AST of member
            if (tupleMember->symbol == nullptr) { // tuple memebers ex: (a , 5, [3, 4, x], 'b') need to be initialized into orderedArgs
                std::string argName = tupleMember->parseTree->getText();
                auto argType = tupleMember->evalType;
                t->TuplePromoteTypeList.push_back(argType); //push type of tuple member
            }
        }
        t->promoteType = nullptr;
    }

    void TypeWalk::visitIntervalLiteral(std::shared_ptr<AST> t) {
        t->evalType = std::dynamic_pointer_cast<Type>(symtab->globals->resolve("interval"));
        t->promoteType = nullptr;
    }
    
    //Terminal Types
    void TypeWalk::visitIntegerConstant(std::shared_ptr<AST> t) {
        t->evalType = std::dynamic_pointer_cast<Type>(symtab->globals->resolve("integer"));
        t->promoteType = nullptr;
    }
    
    void TypeWalk::visitRealConstant(std::shared_ptr<AST> t) {
        t->evalType = std::dynamic_pointer_cast<Type>(symtab->globals->resolve("real"));
        t->promoteType = nullptr; 
    }
    
    void TypeWalk::visitCharacterConstant(std::shared_ptr<AST> t) {
        t->evalType = std::dynamic_pointer_cast<Type>(symtab->globals->resolve("character"));
        t->promoteType = nullptr;
    }
    
    void TypeWalk::visitBooleanConstant(std::shared_ptr<AST> t) {
        t->evalType = std::dynamic_pointer_cast<Type>(symtab->globals->resolve("boolean"));
        t->promoteType = nullptr;
    }

    void TypeWalk::visitStringLiteral(std::shared_ptr<AST> t) {
        t->evalType = std::dynamic_pointer_cast<Type>(symtab->globals->resolve("string"));
        t->promoteType = nullptr;
    }

    void TypeWalk::visitIdentifier(std::shared_ptr<AST> t) {
        t->evalType = t->symbol->type;
        t->promoteType = nullptr;
    }
    
} // namespace gazprea 