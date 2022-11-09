#include "TypeWalk.h"

namespace gazprea {

    TypeWalk::TypeWalk(std::shared_ptr<SymbolTable> symtab) : symtab(symtab), currentScope(symtab->globals) {}
    TypeWalk::~TypeWalk() {}

    void TypeWalk::visit(std::shared_ptr<AST> t) {
        if(!t->isNil()){
            switch(t->getNodeType()){
                case GazpreaParser::PROCEDURE:
                case GazpreaParser::FUNCTION:
                    visitSubroutineDeclDef(t);
                    break;                 

                case GazpreaParser::VAR_DECLARATION_TOKEN: visitVariableDeclaration(t); break; 
                case GazpreaParser::EXPRESSION_TOKEN: visitExpression(t);  break;
                case GazpreaParser::BINARY_OP_TOKEN:
                    visitBinaryOp(t);
                    break;

                //Compound Types
                case GazpreaParser::VECTOR_LITERAL_TOKEN: visitVectorLiteral(t); break;
                case GazpreaParser::TUPLE_LITERAL_TOKEN: visitTuple(t); break;
                case GazpreaParser::INTERVAL: visitInterval(t); break;

                //Terminal Types
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
    
    void TypeWalk::visitSubroutineDeclDef(std::shared_ptr<AST> t) {
        //currentScope = t->symbol //Dosent work
        visitChildren(t);
        // currentScope = currentScope->getEnclosingScope();
    }

    void TypeWalk::visitVariableDeclaration(std::shared_ptr<AST> t) { 
        //example of function use: character[3] cvec = 'c'; (promote char -> vec of char);
        visitChildren(t);
        auto varTy = t->children[1]->evalType; 
        auto exprTy = t->children[2]->evalType;

        if (varTy->isTupleType() && exprTy->isTupleType()) {  //specifically for tuple
            auto varTuple  = std::dynamic_pointer_cast<TupleType>(varTy); 
            auto exprTuple = std::dynamic_pointer_cast<TupleType>(exprTy);

            std::shared_ptr<AST> exprNode =  t->children[2]->children[0];
            for(int i=0; i < varTuple->size; i++) {
                auto fromType = exprTuple->orderedArgs[i]->type->getTypeId();
                auto toType   = varTuple->orderedArgs[i]->type->getTypeId();
                int promote = tp->promotionFromTo[fromType][toType]; 

                // std::cout << t->children[2]->children[0]->getNodeType() << std::endl;
                std::shared_ptr<AST> exprTupleMember; 
                if (exprNode->nodeType == GazpreaParser::TUPLE_LITERAL_TOKEN) {
                    exprTupleMember = exprNode->children[0]->children[i]; 
                } else if (exprNode->nodeType == GazpreaParser::IDENTIFIER_TOKEN) {
                    auto resolvedTuple = currentScope->resolve(exprNode->getText()); // NO RESOLVE HERE cause idk how to push scope.
                    /* 
                    Once we can resolve, we can assign exprTupleMember to resolvedTuple->def->children[0]->children[i];
                    and handle as if it were a literal
                    */ 
                }
                if (promote != 0) {
                    // exprTupleMember->promoteType = varTuple->orderedArgs[i]->type;
                }
            }
        } else { //all other variable types
            int varTyEnum = varTy->getTypeId();
            int exprTyEnum = exprTy->getTypeId();
            int promote = tp->promotionFromTo[exprTyEnum][varTyEnum];
            if(promote != 0) {
                t->children[2]->promoteType = t->children[1]->evalType;
                //uncomment to printout the promotion 
                // std::cout   << "VarDecl promotion:\t" 
                //             << t->children[1]->parseTree->getText() <<  " of type "
                //             << t->children[1]->evalType->getTypeId() <<  " caused promotion of "
                //             << t->children[2]->parseTree->getText() << " of type " 
                //             << t->children[2]->evalType->getTypeId() << " into type " 
                //             << t->children[2]->promoteType->getTypeId() << std::endl;   
                // }
            }
        }
    }

    void TypeWalk::visitExpression(std::shared_ptr<AST> t) {
        visitChildren(t);
        t->evalType = t->children[0]->evalType;
        t->promoteType = nullptr; 
    }
    
    void TypeWalk::visitBinaryOp(std::shared_ptr<AST> t) {
        visitChildren(t); 
        auto node1 = t->children[0];
        auto node2 = t->children[1]; 

        //getResultType automatically populates promotType of children
        switch(t->children[2]->getNodeType()){ 
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

    //Compound Types
    void TypeWalk::visitVectorLiteral(std::shared_ptr<AST> t) {
        visitChildren(t);
        if (t->children[0]->children.size() == 0) { return; } //null vector
        if (t->children[0]->children[0]->children[0]->getNodeType() == GazpreaParser::VECTOR_LITERAL_TOKEN) {
            //matrix
            auto baseType =  t->children[0]->children[0]->children[0]->children[0]->children[0]->evalType;
            t->evalType = std::make_shared<MatrixType>(MatrixType(baseType, 2, t));
            std::cout << "Matrix BaseType: " << baseType->getTypeId() << std::endl;
            std::cout << "Matrix Type: "  << t->evalType->getTypeId() << std::endl;       
        } else {
            //literal vector
            auto baseType = t->children[0]->children[0]->children[0]->evalType;
            t->evalType = std::make_shared<MatrixType>(MatrixType(baseType, 1, t));
            // std::cout << "Vector BaseType: " << baseType->getTypeId() << std::endl;
            // std::cout << "Vector Type: "  << t->evalType->getTypeId() << std::endl;
        }
        t->promoteType = nullptr;
    }

    void TypeWalk::visitTuple(std::shared_ptr<AST> t) {
        visitChildren(t);
        size_t tupleSize = t->children[0]->children.size();
        t->promoteType = nullptr;
        t->evalType = std::make_shared<TupleType>(TupleType(currentScope, t, tupleSize));

        auto tupleEvalType = std::dynamic_pointer_cast<TupleType>(t->evalType);
        for(size_t i = 0; i < tupleSize; i++) { 
            auto tupleMember = t->children[0]->children[i]; //AST of member
            // tupleMember->evalType = 
            std::cout << "tuple memeber type: " << tupleMember->evalType->getTypeId() << std::endl;

            if (tupleMember->symbol == nullptr) { // tuple memebers ex: (a , 5, [3,4, x], 'b') need to be initialized into orderedArgs
                std::string argName = tupleMember->parseTree->getText();
                auto argType = tupleMember->evalType;
                tupleMember->symbol = std::make_shared<VariableSymbol>(VariableSymbol(argName, argType));
                tupleEvalType->orderedArgs.push_back(tupleMember->symbol); 
            }
        }
    }

    void TypeWalk::visitInterval(std::shared_ptr<AST> t) {
        std::cout << "Recieved Interval\n";
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