#pragma once

#include <memory>

#include "Symbol.h"
#include "Type.h"
#include <string>

namespace gazprea {
    class VariableSymbol : public Symbol {
    public:
        std::string typeQualifier;  // Can be "var" or "const"
        VariableSymbol(std::string name, std::shared_ptr<Type> type);
    };
}