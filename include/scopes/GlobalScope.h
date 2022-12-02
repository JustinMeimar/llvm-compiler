#pragma once

#include "BaseScope.h"
#include "VariableSymbol.h"
#include "SubroutineSymbol.h"
#include "TypedefTypeSymbol.h"


namespace gazprea {
    class GlobalScope : public BaseScope {
    public:
        GlobalScope();
        std::string getScopeName() override;
        std::map<std::string, std::shared_ptr<Symbol>> typeSymbols;
        std::vector<std::shared_ptr<VariableSymbol>> globalVariableSymbols;
        std::shared_ptr<SubroutineSymbol> resolveSubroutineSymbol(const std::string &name);
        
        void defineTypeSymbol(std::shared_ptr<Symbol> sym);
        std::shared_ptr<Symbol> resolveTypeSymbol(const std::string &name);
    };
}