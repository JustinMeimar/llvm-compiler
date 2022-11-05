#include "SymbolTable.h"

namespace gazprea {
    void SymbolTable::initTypeSystem() {
        globals->define(std::make_shared<BuiltInScalarTypeSymbol>("integer"));
        globals->define(std::make_shared<BuiltInScalarTypeSymbol>("real"));
        globals->define(std::make_shared<BuiltInScalarTypeSymbol>("character"));
        globals->define(std::make_shared<BuiltInScalarTypeSymbol>("string"));
        globals->define(std::make_shared<BuiltInScalarTypeSymbol>("boolean"));
        globals->define(std::make_shared<BuiltInScalarTypeSymbol>("interval"));
        globals->define(std::make_shared<BuiltInTypeQualifierSymbol>("var"));
        globals->define(std::make_shared<BuiltInTypeQualifierSymbol>("const"));
    }

    SymbolTable::SymbolTable() : globals(std::make_shared<GlobalScope>()) { 
        initTypeSystem(); 
    }

    std::string SymbolTable::toString() {
        return globals->toString();
    }
}

