#pragma once

#include "Symbol.h"
#include "Type.h"
#include "Scope.h"
#include "AST.h"

namespace gazprea {
    class TupleType : public Type, public Scope {
    private:
        std::shared_ptr<Scope> enclosingScope;
    public:
        std::shared_ptr<AST> def;
        size_t size;
        std::vector<std::shared_ptr<Symbol>> orderedArgs;

        TupleType(std::shared_ptr<Scope> enclosingScope, std::shared_ptr<AST> def);
        TupleType(std::shared_ptr<Scope> enclosingScope, std::shared_ptr<AST> def, size_t size);

        bool isTypedefType() {
            return false;
        }
        bool isMatrixType() {
            return true;
        }
        bool isIntervalType() {
            return false;
        }
        bool isTupleType() {
            return true;
        }
        int getTypeId();

        std::string getName() {
            return "tuple";
        }

        std::string getScopeName() {
            return "tuple";
        }

        std::string toString() {
            return "tuple";
        }

        std::shared_ptr<Scope> getEnclosingScope();
        std::shared_ptr<Symbol> resolve(const std::string &name);
        void define(std::shared_ptr<Symbol> sym);
    };
}
