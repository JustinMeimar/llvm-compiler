#include "IdentityNullType.h"

namespace gazprea {
    IdentityNullType::IdentityNullType(std::shared_ptr<Type> baseType) : baseType(baseType) {}

    int IdentityNullType::getTypeId() {
        if (baseType->getTypeId() == Type::IDENTITYNULL) {
            return Type::IDENTITYNULL;
        }
        return -1;
    }
}