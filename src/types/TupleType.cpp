#include "TupleType.h"

namespace gazprea {
    TupleType::TupleType(std::shared_ptr<Scope> enclosingScope, std::shared_ptr<AST> def) : enclosingScope(enclosingScope), def(def) {}

    std::shared_ptr<Symbol> TupleType::resolve(const std::string &name) {
        for ( auto sym : orderedArgs ) {
            if ( sym->getName() == name ) {
                return sym;
            }
        }
        // if not here, check any enclosing scope
        if ( getEnclosingScope() != nullptr ) {
            return getEnclosingScope()->resolve(name);
        }
        return nullptr; // not found
    }

    void TupleType::define(std::shared_ptr<Symbol> sym) {
        orderedArgs.push_back(sym);
        sym->scope = shared_from_this();
    }

    std::shared_ptr<Scope> TupleType::getEnclosingScope() {
        return enclosingScope;
    }

    int TupleType::getTypeId() {
        return Type::TUPLE;
    }
}