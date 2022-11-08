#include "TypePromotion.h"

namespace gazprea {

int Promote::getResultType(int typeTable[16][16], std::shared_ptr<AST> lhs, std::shared_ptr<AST> rhs){

    int lhs_type = lhs->evalType->getTypeId();
    int rhs_type = rhs->evalType->getTypeId();

    int result_type = this->promotionFromTo[lhs_type][rhs_type];

    return result_type;
}

/* Language Patterns Exceprt
    public Type getResultType(Type[][] typeTable, CymbolAST a, CymbolAST b) {
        int ta = a.evalType.getTypeIndex(); // type index of left operand
        int tb = b.evalType.getTypeIndex(); // type index of right operand
        Type result = typeTable[ta][tb];
        // operation result type
        // promote operand types to result type
        a.promoteToType = promoteFromTo[ta][result];
        b.promoteToType = promoteFromTo[tb][result];
        return result;
    }
*/

} //namespace gazprea
