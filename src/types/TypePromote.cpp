#include "TypePromote.h"

namespace gazprea {

void TypePromote::getResultType(int typeTable[16][16], std::shared_ptr<AST> lhs, std::shared_ptr<AST> rhs){

    int lhs_type = lhs->evalType->getTypeId();
    int rhs_type = rhs->evalType->getTypeId();

    int rt = typeTable[lhs_type][rhs_type];

    // lhs->promoteType = std::make_shared<Type>(Type(this->promotionFromTo[lhs_type][rt]));
    // rhs->promoteType = std::make_shared<Type>(Type(this->promotionFromTo[rhs_type][rt]));
}

} //namespace gazprea
