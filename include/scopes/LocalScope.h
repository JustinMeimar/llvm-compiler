#pragma once

#include <memory>

#include "BaseScope.h"

namespace gazprea {
    class LocalScope : public BaseScope {
    public:
        bool parentIsSubroutineSymbol = false;
        bool parentIsLoop = false;
        bool parentIsConditional = false;
        LocalScope(std::shared_ptr<Scope> parent);
        std::string getScopeName() override;
    };
}
