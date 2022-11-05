#include "GlobalScope.h"

namespace gazprea {
    GlobalScope::GlobalScope() : BaseScope(nullptr) {}

    std::string GlobalScope::getScopeName() {
        return "global";
    }
}

