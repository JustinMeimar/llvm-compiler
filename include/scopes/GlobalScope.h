#pragma once

#include "BaseScope.h"


namespace gazprea {
    class GlobalScope : public BaseScope {
    public:
        GlobalScope();
        std::string getScopeName() override;
    };
}