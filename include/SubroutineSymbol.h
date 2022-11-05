#pragma once

#include <string>
#include <memory>
#include <vector>

#include "Symbol.h"
#include "Type.h"

namespace gazprea {
    class SubroutineSymbol : public Symbol, public Scope {
    private:
        std::vector<std::shared_ptr<Symbol>> orderedArgs; // Note: The original Java implementation used a data structure 
                                                        // called a LinkedHashMap which as no direct equivalent in C++
                                                        // Essentially it is a Hash Map implementation that retains the original
                                                        // insertion ordering of its elements.
                                                        // For simplicity, this C++ implementation uses a vector and performs
                                                        // a linear scan to resolve symbols. However, for larger projects this
                                                        // simple alternative may not be ideal for performance.
        std::shared_ptr<Scope> enclosingScope;
    public:
        SubroutineSymbol(std::string name, std::shared_ptr<Type> retType, std::shared_ptr<Scope> enclosingScope);
        std::shared_ptr<Symbol> resolve(const std::string &name);
        void define(std::shared_ptr<Symbol> sym);
        std::shared_ptr<Scope> getEnclosingScope();
        std::string getScopeName();
        std::string toString();
    };
}


