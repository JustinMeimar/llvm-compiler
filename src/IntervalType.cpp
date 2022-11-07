#include "IntervalType.h"

namespace gazprea {
    IntervalType::IntervalType(std::shared_ptr<Type> baseType, std::shared_ptr<Type> intervalType) : baseType(baseType), intervalType(intervalType) {}
}