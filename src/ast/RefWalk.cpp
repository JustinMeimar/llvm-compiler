#include "RefWalk.h"

namespace gazprea {

    RefWalk::RefWalk(std::shared_ptr<SymbolTable> symtab) : symtab(symtab), currentScope(symtab->globals) {
    }
    RefWalk::~RefWalk() {}

    void RefWalk::visit(std::shared_ptr<AST> t) {
        if(!t->isNil()){
            switch(t->getNodeType()){
                //Statements & Intermediate Tokens
                case GazpreaParser::PROCEDURE:
                case GazpreaParser::FUNCTION:
                    visitSubroutineDeclDef(t);
                    break;
                case GazpreaParser::TYPEDEF: visitTypedefStatement(t); break;
                case GazpreaParser::IDENTIFIER_TOKEN: visitIdentifier(t); break;
                case GazpreaParser::VAR_DECLARATION_TOKEN:
                    visitVariableDeclaration(t);
                    break;
                case GazpreaParser::VECTOR_TYPE_TOKEN:
                    visitVectorMatrixType(t);
                    break;
                case GazpreaParser::SINGLE_TOKEN_TYPE_TOKEN:
                    visitSingleTokenType(t);
                    break;
                case GazpreaParser::UNQUALIFIED_TYPE_TOKEN:
                    visitUnqualifiedType(t);
                    break;
                case GazpreaParser::PARAMETER_ATOM_TOKEN:
                    visitParameterAtom(t);
                    break;
                default: visitChildren(t);
            };
        }
        else {
            visitChildren(t); // t must be the null root node 
        }
    }

    void RefWalk::visitChildren(std::shared_ptr<AST> t) { 
        for(auto stat : t->children){
            visit(stat);
        }
    }

    void RefWalk::visitVariableDeclaration(std::shared_ptr<AST> t) {
        visitChildren(t);
        auto variableDeclarationSymbol = std::dynamic_pointer_cast<VariableSymbol>(t->symbol);
        
        if (t->children[0]->getNodeType() == GazpreaParser::INFERRED_TYPE_TOKEN) {
            variableDeclarationSymbol->typeQualifier = t->children[0]->children[0]->parseTree->getText();
        } else {
            if (t->children[0]->children[0]->isNil()) {
                variableDeclarationSymbol->type = t->children[0]->children[1]->type;
            } else {
                variableDeclarationSymbol->typeQualifier = t->children[0]->children[0]->parseTree->getText();
                variableDeclarationSymbol->type = t->children[0]->children[1]->type;
            }
        }
    }

    void RefWalk::visitSubroutineDeclDef(std::shared_ptr<AST> t) {
        visitChildren(t);
        auto subroutineSymbol = std::dynamic_pointer_cast<SubroutineSymbol>(t->symbol);
        if (subroutineSymbol->type == nullptr) {
            subroutineSymbol->type = t->children[2]->type;  // Set return type
            int numArgs = (subroutineSymbol->orderedArgs).size();
            std::vector<std::shared_ptr<Symbol>> newArgs;
            for (int i = numArgs / 2; i < numArgs; i++) {
                newArgs.push_back(subroutineSymbol->orderedArgs[i]);
            }
            subroutineSymbol->orderedArgs = newArgs;
        } 
    }

    void RefWalk::visitParameterAtom(std::shared_ptr<AST> t) {
        visitChildren(t);
        auto variableSymbol = std::dynamic_pointer_cast<VariableSymbol>(t->symbol);
        bool typeQualifierExist = false;
        int numSingleTermType = 0;
        if (t->children[3]->getNodeType() == GazpreaParser::TYPE_QUALIFIER_TOKEN) {
            typeQualifierExist = true;
        }
        for (int i = 0; i < 3; i++) {
            if (!t->children[i]->isNil()) {
                numSingleTermType++;
            }
        }
        if (typeQualifierExist) {
            variableSymbol->typeQualifier = t->children[3]->parseTree->getText();
        }
        if (numSingleTermType == 3) { 
            variableSymbol->type = std::make_shared<IntervalType>(t->children[0]->type);
            variableSymbol->name = t->children[2]->parseTree->getText(); 
        } else if (numSingleTermType == 2) {
            auto symbol = symtab->globals->resolve(t->children[1]->parseTree->getText());
            if (symbol == nullptr || !symbol->isType()) {
                variableSymbol->name = t->children[1]->parseTree->getText();
                variableSymbol->type = t->children[0]->type;
            } else {
                variableSymbol->type = std::make_shared<IntervalType>(t->children[0]->type);
                variableSymbol->name = t->children[1]->parseTree->getText(); 
            }
        } else if (numSingleTermType == 1) {
            auto symbol = symtab->globals->resolve(t->children[0]->parseTree->getText());
            if (t->children[0]->getNodeType() == GazpreaParser::VECTOR_TYPE_TOKEN || t->children[0]->getNodeType() == GazpreaParser::TUPLE_TYPE_TOKEN) {
                variableSymbol->type = t->children[0]->type;
            } else if (symbol == nullptr || !symbol->isType()) {
                variableSymbol->name = t->children[0]->parseTree->getText();
            } else {
                variableSymbol->type = t->children[0]->type;
            }
        }
    }

    void RefWalk::visitTypedefStatement(std::shared_ptr<AST> t) {
        visit(t->children[0]);
        auto typedefTypeSymbol = std::dynamic_pointer_cast<TypedefTypeSymbol>(t->symbol);
        typedefTypeSymbol->type = t->children[0]->type;  // visitChildren(t) already populated t->children[0]->type for us
    }

    void RefWalk::visitIdentifier(std::shared_ptr<AST> t) {
        t->symbol = t->scope->resolve(t->parseTree->getText());
    }

    void RefWalk::visitSingleTokenType(std::shared_ptr<AST> t) {
        auto text = t->parseTree->getText();
        t->type = std::dynamic_pointer_cast<Type>(symtab->globals->resolve(text));
        if (t->type == nullptr) {
            // SingleTokenType can be a Type, or an identifier. If it is an identifier, t->type will be nullptr
            return;
        }
        if (t->type->isTypedefType()) {
            auto typeDefTypeSymbol = std::dynamic_pointer_cast<TypedefTypeSymbol>(t->type);
            typeDefTypeSymbol->resolveTargetType();  // If TypeDef is already called resolveTargetType once, this method will do nothing
        }
    }

    void RefWalk::visitVectorMatrixType(std::shared_ptr<AST> t) {
        visitChildren(t);
        t->type = std::make_shared<MatrixType>(t->children[0]->type, (t->children[1]->children).size(), t);
    }

    void RefWalk::visitUnqualifiedType(std::shared_ptr<AST> t) {
        visitChildren(t);
        if ((t->children).size() == 2) {
            // Interval Type
            t->type = std::make_shared<IntervalType>(t->children[0]->type);
        } else {
            // Not an interval type
            t->type = t->children[0]->type;
        }
    }
} // namespace gazprea 