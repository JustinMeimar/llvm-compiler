#include "TypeWalk.h"

namespace gazprea {

    TypeWalk::TypeWalk(std::shared_ptr<SymbolTable> symtab) : symtab(symtab), currentScope(symtab->globals) {
    }
    TypeWalk::~TypeWalk() {}

    void TypeWalk::visit(std::shared_ptr<AST> t) {
        if(!t->isNil()){
            switch(t->getNodeType()){
                //Top level
                case GazpreaParser::EXPRESSION_TOKEN: visitExpression(t);  
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

    void TypeWalk::visitBinaryOp(std::shared_ptr<AST> t) {
        visitChildren(t); 

    }
    void TypeWalk::visitExpression(std::shared_ptr<AST> t) {
        visitChildren(t);


    }

    void TypeWalk::visitIntegerConstant(std::shared_ptr<AST> t) {
        t->evalType = std::dynamic_pointer_cast<Type>(symtab->globals->resolve("integer"));
    }
    void TypeWalk::visitRealConstant(std::shared_ptr<AST> t) {
        t->evalType = std::dynamic_pointer_cast<Type>(symtab->globals->resolve("real"));
    }
    
} // namespace gazprea 