#include "TypeWalk.h"

namespace gazprea {

    TypeWalk::TypeWalk(std::shared_ptr<SymbolTable> symtab) : symtab(symtab), currentScope(symtab->globals) {}
    TypeWalk::~TypeWalk() {}

    void TypeWalk::visit(std::shared_ptr<AST> t) {
        if(!t->isNil()){
            switch(t->getNodeType()){
                case GazpreaParser::VAR_DECLARATION_TOKEN: visitVariableDeclaration(t); break; 
                case GazpreaParser::EXPRESSION_TOKEN: visitExpression(t);  break;
                case GazpreaParser::BINARY_OP_TOKEN:
                    visitBinaryOp(t);
                    break;
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
        if (t->children[2]->evalType == nullptr) {
            return;
        }
        //this indexing only works for explicit type i think..
        auto varTy = t->children[1]->evalType->getTypeId(); 
        auto exprTy = t->children[2]->evalType->getTypeId();

        int promote = tp->promotionFromTo[exprTy][varTy];
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

        // t->evalType == std::dynamic_pointer_cast<Type>(symtab->globals->resolve());
    }

    void TypeWalk::visitTupler(std::shared_ptr<AST> t) {

    }

    void TypeWalk::visitInterval(std::shared_ptr<AST> t) {

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