#include "BuiltInScalarTypeSymbol.h"

namespace gazprea {
    BuiltInScalarTypeSymbol::BuiltInScalarTypeSymbol(std::string name) : Symbol(name) {}

    std::string BuiltInScalarTypeSymbol::getName() {
        return Symbol::getName();
    }

    int BuiltInScalarTypeSymbol::getTypeId() {
        if (name == "boolean") {
            return Type::BOOLEAN;
        } else if (name == "character") {
            return Type::CHARACTER;
        } else if (name == "integer") {
            return Type::INTEGER;
        } else if (name == "real") {
            return Type::REAL;
        } else if (name == "string") {
            return Type::STRING;
        } else if (name == "interval") {
            return Type::INTERVAL;  // will not be used
        } else if (name == "identity") {
            return Type::IDENTITY;  // will not be used
        }
        return -1;
    }
}

