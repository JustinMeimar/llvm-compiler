#include "TypedefTypeSymbol.h"

namespace gazprea {
    TypedefTypeSymbol::TypedefTypeSymbol(std::string name) : Symbol(name) {}

    std::string TypedefTypeSymbol::getName() {
        return Symbol::getName();
    }

    void TypedefTypeSymbol::resolveTargetType() {
        auto iterator = type;
        while (iterator->isTypedefType()) {
            auto typeDefType = std::dynamic_pointer_cast<TypedefTypeSymbol>(iterator);
            iterator = typeDefType->type;        
        }
        type = iterator;
    }
}