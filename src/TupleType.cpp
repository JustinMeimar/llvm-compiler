#include "TupleType.h"

namespace gazprea {
    TupleType::TupleType(std::vector<std::shared_ptr<Type>> listType, std::shared_ptr<AST> def) : listType(listType), def(def) {}
}