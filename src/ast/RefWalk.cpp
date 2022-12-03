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
                case GazpreaParser::TYPEDEF:
                    visitTypedefStatement(t);
                    break;
                case GazpreaParser::IDENTIFIER_TOKEN:
                    visitIdentifier(t);
                    break;
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
                case GazpreaParser::CALL_PROCEDURE_FUNCTION_IN_EXPRESSION:
                case GazpreaParser::CALL_PROCEDURE_STATEMENT_TOKEN:
                    visitCall(t);
                    break;
                default:
                    visitChildren(t);
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

        auto *ctx = dynamic_cast<GazpreaParser::VarDeclarationStatementContext* >(t->parseTree);

        if (t->children[0]->getNodeType() == GazpreaParser::INFERRED_TYPE_TOKEN) {
            variableDeclarationSymbol->typeQualifier = t->children[0]->children[0]->parseTree->getText();
            if (variableDeclarationSymbol->isGlobalVariable 
                && t->children[0]->children[0]->parseTree->getText() != "const") {
                std::string msg = "Global variable cannot have type qualifier \"const\"";
                throw GlobalVariableQualifierError(msg, ctx->getText(), ctx->getStart()->getLine(), ctx->getStart()->getCharPositionInLine());
            }
        } else {
            if (t->children[0]->children[0]->isNil()) {
                if (variableDeclarationSymbol->isGlobalVariable) {
                    std::string msg = "Global variable must have a type qualifier, and that type qualifier must be \"const\"";
                    throw GlobalVariableQualifierError(msg, ctx->getText(), ctx->getStart()->getLine(), ctx->getStart()->getCharPositionInLine());
                }
                variableDeclarationSymbol->typeQualifier = "var";  // default type qualifier is var
                variableDeclarationSymbol->type = t->children[0]->children[1]->type;
            } else {
                variableDeclarationSymbol->typeQualifier = t->children[0]->children[0]->parseTree->getText();
                if (variableDeclarationSymbol->isGlobalVariable && t->children[0]->children[0]->parseTree->getText() != "const") {
                    std::string msg = "Global variable must have a type qualifier, and that type qualifier must be \"const\"";
                    throw GlobalVariableQualifierError(msg, ctx->getText(), ctx->getStart()->getLine(), ctx->getStart()->getCharPositionInLine());
                }
                variableDeclarationSymbol->type = t->children[0]->children[1]->type;
            }
        }
    }

    void RefWalk::visitSubroutineDeclDef(std::shared_ptr<AST> t) {
        visitChildren(t);
        auto subroutineSymbol = std::dynamic_pointer_cast<SubroutineSymbol>(t->symbol);
        if (subroutineSymbol->numTimesDeclare == 1) {
            // visit Forward declaration, or only one definition exists
            subroutineSymbol->type = t->children[2]->type;  // Set return type
        } else {
            subroutineSymbol->type = t->children[2]->type;  // Set return type
            int numArgs = (subroutineSymbol->orderedArgs).size();
            std::vector<std::shared_ptr<Symbol>> newArgs;
            for (int i = numArgs / 2; i < numArgs; i++) {
                newArgs.push_back(subroutineSymbol->orderedArgs[i]);
            }
            subroutineSymbol->orderedArgs = newArgs;
            subroutineSymbol->numTimesDeclare--;  // We will eliminate duplicated parameter once (i.e., this else-block will only run once)
        }
        auto ctx = dynamic_cast<GazpreaParser::SubroutineDeclDefContext*>(t->parseTree);
        if (subroutineSymbol->getName() == "main" && subroutineSymbol->type->getTypeId() != Type::INTEGER) {
            throw MainReturnIntegerError(
                (ctx->children[0]->getText() + " " + ctx->children[2]->getText() + ctx->children[3]->getText() + ctx->children[4]->getText()),
                ctx->getStart()->getLine(), ctx->getStart()->getCharPositionInLine()
            );
        }
        if (subroutineSymbol->getName() == "main" && subroutineSymbol->orderedArgs.size() != 0){
            throw MainArgumentsPresentError("procedure main(...)", 
                ctx->getStart()->getLine(), ctx->getStart()->getCharPositionInLine());
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
        } else {
            // Default type qualifier for subroutine parameter is const 
            variableSymbol->typeQualifier = "const";
        }
        if (numSingleTermType == 3) { 
            variableSymbol->type = std::make_shared<IntervalType>(t->children[0]->type);
            variableSymbol->name = t->children[2]->parseTree->getText();

            auto status = symtab->tupleIdentifierAccess.emplace(
                variableSymbol->name, symtab->numTupleIdentifierAccess
            );
            if (status.second) {
                symtab->numTupleIdentifierAccess++;
            }
        } else if (numSingleTermType == 2) {
            auto symbol = symtab->globals->resolveTypeSymbol(t->children[1]->parseTree->getText());
            if (symbol == nullptr) {
                variableSymbol->name = t->children[1]->parseTree->getText();
                variableSymbol->type = t->children[0]->type;
                
                auto status = symtab->tupleIdentifierAccess.emplace(
                    variableSymbol->name, symtab->numTupleIdentifierAccess
                );
                if (status.second) {
                    symtab->numTupleIdentifierAccess++;
                }
            } else {
                variableSymbol->type = std::make_shared<IntervalType>(t->children[0]->type);
            }
        } else if (numSingleTermType == 1) {
            auto symbol = symtab->globals->resolveTypeSymbol(t->children[0]->parseTree->getText());
            if (t->children[0]->getNodeType() == GazpreaParser::VECTOR_TYPE_TOKEN 
                || t->children[0]->getNodeType() == GazpreaParser::TUPLE_TYPE_TOKEN) {
                if (t->children[0]->type->getTypeId() == -1) {
                    //string[n] picked up as VECTOR, assign type as string
                    t->children[0]->type = symtab->getType(Type::STRING); 
                }
                variableSymbol->type = t->children[0]->type;
            } else if (symbol == nullptr) {
                variableSymbol->name = t->children[0]->parseTree->getText();
                auto status = symtab->tupleIdentifierAccess.emplace(
                    variableSymbol->name, symtab->numTupleIdentifierAccess
                );
                if (status.second) {
                    symtab->numTupleIdentifierAccess++;
                }
            } else {
                variableSymbol->type = t->children[0]->type;
            }
        }
    }

    void RefWalk::visitTypedefStatement(std::shared_ptr<AST> t) {
        visit(t->children[0]);
        auto typedefTypeSymbol = std::dynamic_pointer_cast<TypedefTypeSymbol>(t->symbol);
        // visitChildren(t) already populated t->children[0]->type for us
        typedefTypeSymbol->type = t->children[0]->type;
    }

    void RefWalk::visitIdentifier(std::shared_ptr<AST> t) {
        t->symbol = t->scope->resolve(t->parseTree->getText());
    }

    void RefWalk::visitSingleTokenType(std::shared_ptr<AST> t) {
        auto text = t->parseTree->getText();
        t->type = std::dynamic_pointer_cast<Type>(symtab->globals->resolveTypeSymbol(text));
        if (t->type == nullptr) {
            // TODO: Throw an error
            return;
        }
        if (t->type->isTypedefType()) {
            auto typeDefTypeSymbol = std::dynamic_pointer_cast<TypedefTypeSymbol>(t->type);
            // If TypeDef is already called resolveTargetType once, this method will do nothing
            typeDefTypeSymbol->resolveTargetType();
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

    void RefWalk::visitCall(std::shared_ptr<AST> t) {
        t->children[0]->symbol = symtab->globals->resolveSubroutineSymbol(
            "gazprea.subroutine." + t->children[0]->parseTree->getText()
        );
        visit(t->children[1]);
    }
} // namespace gazprea