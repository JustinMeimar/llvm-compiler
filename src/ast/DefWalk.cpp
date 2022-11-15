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
        symtab->globals->define(std::make_shared<VariableSymbol>("std_input", nullptr));
        symtab->globals->define(std::make_shared<VariableSymbol>("std_output", nullptr));
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
                case GazpreaParser::TYPEDEF:
                    visitTypedefStatement(t);
                    break;
                case GazpreaParser::BLOCK_TOKEN: 
                    visitBlock(t);
                    break;
                case GazpreaParser::IDENTIFIER_TOKEN:
                    visitIdentifier(t);
                    break;
                case GazpreaParser::VAR_DECLARATION_TOKEN:
                    visitVariableDeclaration(t);
                    break;
                case GazpreaParser::PARAMETER_ATOM_TOKEN:
                    visitParameterAtom(t);
                    break;
                case GazpreaParser::TUPLE_TYPE_TOKEN:
                    visitTupleType(t);
                    break;
                case GazpreaParser::BREAK:
                    visitBreak(t);
                    break;
                case GazpreaParser::CONTINUE:
                    visitContinue(t);
                    break;
                case GazpreaParser::RETURN:
                    visitReturn(t);
                    break;
                case GazpreaParser::TUPLE_ACCESS_TOKEN:
                    visitTupleAccess(t);
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
        auto identifierAST = t->children[1];
        auto vs = std::make_shared<VariableSymbol>(identifierAST->parseTree->getText(), nullptr);
        vs->def = t;  // track AST location of def's ID (i.e., where in AST does this symbol defined)
        t->symbol = vs;  // track in AST
        currentScope->define(vs);
        visitChildren(t);
        if (currentScope->getScopeName() == "global") {
            vs->isGlobalVariable = true;
            symtab->globals->globalVariableSymbols.push_back(vs);
        } else {
            vs->isGlobalVariable = false;
        }
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
        std::shared_ptr<SubroutineSymbol> subroutineSymbol;
        auto declarationSubroutineSymbol = symtab->globals->resolve(identiferAST->parseTree->getText());
        if (declarationSubroutineSymbol == nullptr) {
            subroutineSymbol = std::make_shared<SubroutineSymbol>(identiferAST->parseTree->getText(), nullptr, symtab->globals, isProcedure, isBuiltIn);
            subroutineSymbol->declaration = t;
            symtab->globals->define(subroutineSymbol); // def subroutine in globals
            subroutineSymbol->numTimesDeclare++;
        } else {
            subroutineSymbol = std::dynamic_pointer_cast<SubroutineSymbol>(declarationSubroutineSymbol);
            subroutineSymbol->definition = t;
            subroutineSymbol->numTimesDeclare++;
        }

        t->symbol = subroutineSymbol;  // track in AST
        currentSubroutineScope = subroutineSymbol;  // Track Subroutine Scope for visitReturn()
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
        symtab->globals->define(typeDefTypeSymbol);
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

    void DefWalk::visitTupleType(std::shared_ptr<AST> t) {
        size_t tupleSize = t->children[0]->children.size();
        auto tupleType = std::make_shared<TupleType>(currentScope, t, tupleSize);
        t->type = tupleType;
        currentScope = tupleType;        // set current scope to tuple scope
        visitChildren(t);
        currentScope = currentScope->getEnclosingScope(); // pop tuple scope
    }

    void DefWalk::visitParameterAtom(std::shared_ptr<AST> t) {
        auto variableSymbol = std::make_shared<VariableSymbol>("", nullptr);
        currentScope->define(variableSymbol);
        t->symbol = variableSymbol;
        variableSymbol->def = t;
    }

    void DefWalk::visitBreak(std::shared_ptr<AST> t) {
        t->scope = currentScope;
    }

    void DefWalk::visitContinue(std::shared_ptr<AST> t) {
        t->scope = currentScope;
    }

    void DefWalk::visitReturn(std::shared_ptr<AST> t) {
        t->scope = currentSubroutineScope;
        visitChildren(t);
    }

    void DefWalk::visitTupleAccess(std::shared_ptr<AST> t) {
        visitChildren(t);
        if (t->children[1]->getNodeType() == GazpreaParser::IDENTIFIER_TOKEN) {
            auto identifierName = t->children[1]->parseTree->getText();
            auto status = symtab->tupleIdentifierAccess.emplace(identifierName, symtab->numTupleIdentifierAccess);
            if (status.second) {
                symtab->numTupleIdentifierAccess++;
            }
        }
    }
} // namespace gazprea 