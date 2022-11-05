#include "VariableSymbol.h"

#include <memory>

#include "Symbol.h"
#include "Type.h"

namespace gazprea {
    VariableSymbol::VariableSymbol(std::string name, std::shared_ptr<Type> singleTermType1, std::shared_ptr<Type> singleTermType2) : Symbol(name, singleTermType1, singleTermType2) { }
}

