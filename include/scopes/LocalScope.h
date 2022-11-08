#pragma once

#include <memory>

#include "BaseScope.h"

namespace gazprea {
    class LocalScope : public BaseScope {
    public:
        LocalScope(std::shared_ptr<Scope> parent);
        std::string getScopeName() override;
    };
}
