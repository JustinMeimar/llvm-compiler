#include "Symbol.h"

namespace gazprea {
    Symbol::Symbol(std::string name) : Symbol(name, nullptr, nullptr) {}
    Symbol::Symbol(std::string name, std::shared_ptr<Type> singleTermType1, std::shared_ptr<Type> singleTermType2) : name(name), singleTermType1(singleTermType1), singleTermType2(singleTermType2) {}

    std::string Symbol::getName() { return name; }

    std::string Symbol::toString() {
        if (singleTermType1 != nullptr) {
            if (singleTermType2 != nullptr) {
                return '<' + getName() + ":" + singleTermType1->getName() + " " + singleTermType2->getName() + '>';
            } else {
                return '<' + getName() + ":" + singleTermType1->getName() + '>';
            }
        }
        return getName();
    }

    Symbol::~Symbol() {}
}

