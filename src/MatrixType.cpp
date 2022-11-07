#include "MatrixType.h"

namespace gazprea {
    MatrixType::MatrixType(std::shared_ptr<Type> baseType, int dimension, std::shared_ptr<AST> def) : baseType(baseType), dimension(dimension), def(def) {}
}