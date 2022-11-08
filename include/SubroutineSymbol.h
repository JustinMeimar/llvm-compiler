#pragma once

#include <string>
#include <memory>
#include <vector>

#include "Symbol.h"
#include "Type.h"

namespace gazprea {
    class SubroutineSymbol : public Symbol, public Scope {
    private:
        std::shared_ptr<Scope> enclosingScope;
    public:
        bool isProcedure;  // True if the subroutine is a procedure, False if the subroutine is a function
        bool isBuiltIn;  // True if this subroutine is built-in, false otherwise
        std::vector<std::shared_ptr<Symbol>> orderedArgs;
        SubroutineSymbol(std::string name, std::shared_ptr<Type> retType, std::shared_ptr<Scope> enclosingScope, bool isProcedure, bool isBuiltIn);
        std::shared_ptr<Symbol> resolve(const std::string &name);
        void define(std::shared_ptr<Symbol> sym);
        std::shared_ptr<Scope> getEnclosingScope();
        std::string getScopeName();
        std::string toString();
        bool isType() {
            return false;
        }
    };
}


