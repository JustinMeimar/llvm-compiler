#include "TypeWalk.h"

namespace gazprea {

    TypeWalk::TypeWalk(std::shared_ptr<SymbolTable> symtab) : symtab(symtab), currentScope(symtab->globals) {
    }
    TypeWalk::~TypeWalk() {}

    void TypeWalk::visit(std::shared_ptr<AST> t) {
        if(!t->isNil()){
            switch(t->getNodeType()){
                //Top level
                case GazpreaParser::PROCEDURE:
                case GazpreaParser::FUNCTION:
                    visitSubroutineDeclDef(t);
                    break; 
                //Expr 
                case GazpreaParser::EXPRESSION_TOKEN: visitExpression(t);  break;
                case GazpreaParser::BLOCK_TOKEN: visitBlock(t); break;
                case GazpreaParser::VAR_DECLARATION_TOKEN: visitVariableDeclaration(t); break;

                //Binary Op
                case GazpreaParser::ASTERISK:
                case GazpreaParser::DIV:
                case GazpreaParser::MODULO:
                case GazpreaParser::DOTPRODUCT:
                case GazpreaParser::PLUS:
                case GazpreaParser::MINUS:
                case GazpreaParser::BY:
                case GazpreaParser::GREATERTHAN:
                case GazpreaParser::LESSTHAN:
                case GazpreaParser::LESSTHANOREQUAL:
                case GazpreaParser::GREATERTHANOREQUAL:
                case GazpreaParser::ISEQUAL:
                case GazpreaParser::ISNOTEQUAL:
                case GazpreaParser::AND:
                case GazpreaParser::OR:
                case GazpreaParser::XOR:
                    visitBinaryOp(t);
                    break;
                //Unary Op

                //Leaf Nodes
                case GazpreaParser::IntegerConstant: visitIntegerConstant(t); break;
                case GazpreaParser::REAL_CONSTANT_TOKEN: visitRealConstant(t); break;
                case GazpreaParser::IDENTIFIER_TOKEN: visitIdentifier(t); break;

                default: visitChildren(t);
            };
        }
        else {
            visitChildren(t); // t must be the null root node 
        }
    }

    void TypeWalk::visitChildren(std::shared_ptr<AST> t) {
        for( auto child : t->children) {
            visit(child);
        } 
    }

    void TypeWalk::visitSubroutineDeclDef(std::shared_ptr<AST> t) {
        visitChildren(t);
    }

    void TypeWalk::visitBlock(std::shared_ptr<AST> t) {
        visitChildren(t);
    } 

    void TypeWalk::visitVariableDeclaration(std::shared_ptr<AST> t) {
        visitChildren(t);
    }

    void TypeWalk::visitExpression(std::shared_ptr<AST> t) {
        visitChildren(t);
        t->evalType = t->children[0]->evalType;
        t->promoteType = nullptr; 
    }
    
    void TypeWalk::visitBinaryOp(std::shared_ptr<AST> t) {
        visitChildren(t); 
        std::cout << t->children.size(); //prints 0

        // What we want to do if binary op AST has children... 
        // auto node1 = t->children[0];
        // auto node2 = t->children[1];        
        // t->evalType = this->tp->getResultType(tp->arithmeticResultType, node1, node2);
    }

    void TypeWalk::visitIntegerConstant(std::shared_ptr<AST> t) {
        t->evalType = std::dynamic_pointer_cast<Type>(symtab->globals->resolve("integer"));
        t->promoteType = nullptr;
    }
    
    void TypeWalk::visitRealConstant(std::shared_ptr<AST> t) {
        t->evalType = std::dynamic_pointer_cast<Type>(symtab->globals->resolve("real"));
        t->promoteType = nullptr; 
    }

    void TypeWalk::visitIdentifier(std::shared_ptr<AST> t) {
        t->evalType = t->symbol->type;
        t->promoteType = nullptr;
    }
    
} // namespace gazprea 