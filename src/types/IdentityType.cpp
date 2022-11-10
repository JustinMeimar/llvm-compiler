#include "IdentityType.h"

namespace gazprea {
    IdentityType::IdentityType(std::shared_ptr<Type> baseType) : baseType(baseType) {}

    int IdentityType::getTypeId() {
        if (baseType->getTypeId() == Type::IDENTITY) {
            return Type::IDENTITY;
        }
        return -1;
    }
}