#include "SubroutineSymbol.h"

#include <sstream>

namespace gazprea {
    SubroutineSymbol::SubroutineSymbol( std::string name, std::shared_ptr<Type> retType,
                                std::shared_ptr<Scope> enclosingScope) 
                            : Symbol(name, retType), enclosingScope(enclosingScope) {}

    std::shared_ptr<Symbol> SubroutineSymbol::resolve(const std::string &name) {
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

    void SubroutineSymbol::define(std::shared_ptr<Symbol> sym) {
        orderedArgs.push_back(sym);
        sym->scope = shared_from_this();
    }

    std::shared_ptr<Scope> SubroutineSymbol::getEnclosingScope() { return enclosingScope; }

    std::string SubroutineSymbol::getScopeName() { return name; }

    std::string SubroutineSymbol::toString() {
        std::stringstream str;
        str << "method" << Symbol::toString() << ":{";
        for (auto iter = orderedArgs.begin(); iter != orderedArgs.end(); iter++) {
            std::shared_ptr<Symbol> sym = *iter;
            if ( iter != orderedArgs.begin() ) str << ", ";
            str << sym->toString();
        }
        str << "}";
        return str.str();
    }
}