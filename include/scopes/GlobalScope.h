#pragma once

#include "BaseScope.h"
#include "VariableSymbol.h"


namespace gazprea {
    class GlobalScope : public BaseScope {
    public:
        GlobalScope();
        std::string getScopeName() override;
        std::vector<std::shared_ptr<VariableSymbol>> globalVariableSymbols;
    };
}