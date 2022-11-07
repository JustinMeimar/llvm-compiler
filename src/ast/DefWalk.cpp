#include "DefWalk.h"

namespace gazprea {

    DefWalk::DefWalk(std::shared_ptr<SymbolTable> symtab) : symtab(symtab), currentScope(symtab->globals) {
        // Define built-in subroutine symbols
        bool isBuiltIn = true;
        symtab->globals->define(std::make_shared<SubroutineSymbol>("length", nullptr, symtab->globals, false, isBuiltIn));
        symtab->globals->define(std::make_shared<SubroutineSymbol>("rows", nullptr, symtab->globals, false, isBuiltIn));
        symtab->globals->define(std::make_shared<SubroutineSymbol>("columns", nullptr, symtab->globals, false, isBuiltIn));
        symtab->globals->define(std::make_shared<SubroutineSymbol>("reverse", nullptr, symtab->globals, false, isBuiltIn));
        symtab->globals->define(std::make_shared<SubroutineSymbol>("stream_state", nullptr, symtab->globals, true, isBuiltIn));
    }
    DefWalk::~DefWalk() {}

    void DefWalk::visit(std::shared_ptr<AST> t) {
        if(!t->isNil()){
            switch(t->getNodeType()){
                //Statements & Intermediate Tokens
                case GazpreaParser::PROCEDURE:
                case GazpreaParser::FUNCTION:
                    visitSubroutineDeclDef(t);
                    break;
                case GazpreaParser::TYPEDEF: visitTypedefStatement(t); break;
                case GazpreaParser::BLOCK_TOKEN: visitBlock(t); break;
                case GazpreaParser::IDENTIFIER_TOKEN: visitIdentifier(t); break;
                case GazpreaParser::VAR_DECLARATION_TOKEN:
                    visitVariableDeclaration(t);
                    break;
                default: visitChildren(t);
            };
        }
        else {
            visitChildren(t); // t must be the null root node 
        }
    }

    void DefWalk::visitChildren(std::shared_ptr<AST> t) { 
        for(auto stat : t->children){
            visit(stat);
        }
    }

    void DefWalk::visitVariableDeclaration(std::shared_ptr<AST> t) {
        auto identiferAST = t->children[1];
        auto vs = std::make_shared<VariableSymbol>(identiferAST->parseTree->getText(), nullptr);
        vs->def = t;  // track AST location of def's ID (i.e., where in AST does this symbol defined)
        t->symbol = vs;  // track in AST
        currentScope->define(vs);
        visitChildren(t);
    }

    void DefWalk::visitSubroutineDeclDef(std::shared_ptr<AST> t) {
        bool isProcedure;
        bool isBuiltIn = false;
        if (t->getNodeType() == GazpreaParser::PROCEDURE) {
            isProcedure = true;
        } else {
            isProcedure = false;
        }
        auto identiferAST = t->children[0];
        auto subroutineSymbol = std::make_shared<SubroutineSymbol>(identiferAST->parseTree->getText(), nullptr, currentScope, isProcedure, isBuiltIn);
        subroutineSymbol->def = t;  // track AST location of def's ID (i.e., where in AST does this symbol defined)
        t->symbol = subroutineSymbol;  // track in AST
        currentScope->define(subroutineSymbol); // def subroutine in globals
        currentScope = subroutineSymbol;        // set current scope to subroutine scope
        visitChildren(t);
        currentScope = currentScope->getEnclosingScope(); // pop subroutine scope

        t->children[0]->scope = currentScope;  // Manually set the scope of the identifier token of this AST node (override visitIdentifier(t))
    }

    void DefWalk::visitTypedefStatement(std::shared_ptr<AST> t) {
        auto identiferAST = t->children[1];
        auto typeDefTypeSymbol = std::make_shared<TypedefTypeSymbol>(identiferAST->parseTree->getText());
        typeDefTypeSymbol->def = t;  // track AST location of def's ID (i.e., where in AST does this symbol defined)
        t->symbol = typeDefTypeSymbol;  // track in AST
        currentScope->define(typeDefTypeSymbol);
        visitChildren(t);
    } 
    void DefWalk::visitBlock(std::shared_ptr<AST> t) {
        currentScope = std::make_shared<LocalScope>(currentScope); // push scope
        t->scope = currentScope;
        visitChildren(t);
        currentScope = currentScope->getEnclosingScope(); // pop scope
    }

    void DefWalk::visitIdentifier(std::shared_ptr<AST> t) {
        t->scope = currentScope;
    }
} // namespace gazprea 