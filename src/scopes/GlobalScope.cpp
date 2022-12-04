#include "GlobalScope.h"

namespace gazprea {
    GlobalScope::GlobalScope() : BaseScope(nullptr) {}

    std::string GlobalScope::getScopeName() {
        return "gazprea.scope.global";
    }

    std::shared_ptr<SubroutineSymbol> GlobalScope::resolveSubroutineSymbol(const std::string &name) {
        for (auto const& [key, val] : symbols) {
            if (key == name) {
                auto vs = std::dynamic_pointer_cast<SubroutineSymbol>(val);
                if (vs != nullptr) {
                    return vs;
                }
            }
        }
        return nullptr;
    }

    std::shared_ptr<Symbol> GlobalScope::resolveTypeSymbol(const std::string &name) {
        auto find_s = typeSymbols.find(name);
        if ( find_s != typeSymbols.end() ) return find_s->second;
        return nullptr; // not found
    }
	
    void GlobalScope::defineTypeSymbol(std::shared_ptr<Symbol> sym) {
        if (typeSymbols.count(sym->name) != 0) { 
            sym->isDoubleDefined = true;
        }
        typeSymbols.emplace(sym->name, sym);
        sym->scope = shared_from_this(); // track the scope in each symbol
    }
}

