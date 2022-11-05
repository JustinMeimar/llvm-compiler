#pragma once

#include <string>
#include <memory>

#include "Type.h"
#include "Scope.h"
#include "AST.h"

namespace gazprea {
    class Scope; // forward declaration of Scope to resolve circular dependency

    class Symbol { // A generic programming language symbol
    public:
        std::string name;               // All symbols at least have a name
        std::shared_ptr<Type> singleTermType1;  // e.g., integer, ...
        std::shared_ptr<Type> singleTermType2;  // e.g., interval, nullptr, ...
        std::shared_ptr<Scope> scope;   // All symbols know what scope contains them.
        std::shared_ptr<AST> def;

        Symbol(std::string name);
        Symbol(std::string name, std::shared_ptr<Type> singleTermType1, std::shared_ptr<Type> singleTermType2);
        virtual std::string getName();

        virtual std::string toString();
        virtual ~Symbol();
    };
}

