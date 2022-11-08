#include "IntervalType.h"

namespace gazprea {
    IntervalType::IntervalType(std::shared_ptr<Type> baseType) : baseType(baseType) {}

    int IntervalType::getTypeId() {
        if (baseType->getTypeId() == Type::INTEGER) {
            return Type::INTEGER_INTERVAL;
        }
        return -1;
    }
}