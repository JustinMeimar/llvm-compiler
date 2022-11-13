#include "TypePromote.h"

namespace gazprea {

TypePromote::TypePromote(std::shared_ptr<SymbolTable> symtab) : symtab(symtab) {}
TypePromote::~TypePromote() {}

// -1 means void type (invalid)
int TypePromote::arithmeticResultType[16][16] = { // + - / * ^  ..  
    //                          0   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15
    /*   0  TUPLE          */ {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    /*   1  INTERVAL       */ {-1,  1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    /*   2  BOOLEAN        */ {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    /*   3  CHARACTER      */ {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    /*   4  INTEGER        */ {-1, -1, -1, -1,  4,  5, -1, -1, -1, -1, 10, -1, -1, -1, 14, 15},
    /*   5  REAL           */ {-1, -1, -1, -1,  5,  5, -1, -1, -1, -1, -1, -1, -1, -1, 15, 15},
    /*   6  STRING         */ {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    /*   7  INTEGER_INTERVAL*/{-1, -1, -1, -1, -1, -1, -1,  7, -1, -1, -1, -1, -1, -1, -1, -1},
    /*   8  BOOLEAN_1      */ {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    /*   9  CHARACTER_1    */ {-1, -1, -1, -1, -1, -1, -1, -1, -1, 10, -1, -1, -1, -1, -1, -1},
    /*   10 INTEGER_1      */ {-1, -1, -1, -1, 10, -1, -1, -1, -1, -1, 10, 11, -1, -1, -1, -1},
    /*   11 REAL_1         */ {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 11, 11, -1, -1, -1, -1},
    /*   12 BOOLEAN_2      */ {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    /*   13 CHARACTER_2    */ {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 14, 14, -1},
    /*   14 INTEGER_2      */ {-1, -1, -1, -1, 14, 15, -1, -1, -1, -1, -1, -1, -1, 14, 14, 15},
    /*   15 REAL_2         */ {-1, -1, -1, -1, 15, 15, -1, -1, -1, -1, -1, -1, -1, -1, 15, 15}
};

int TypePromote::relationalResultType[16][16] = { // <=, >=, <, >
    //                      0    1   2   3   4   5   6   7   8   9   10 11  12  13  14  15
    /*   TUPLE          */ {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    /*   INTERVAL       */ {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    /*   BOOLEAN        */ {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    /*   CHARACTER      */ {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    /*   INTEGER        */ {-1, -1, -1, -1,  2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    /*   REAL           */ {-1, -1, -1, -1, -1,  2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    /*   STRING         */ {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    /*   INTEGER_INTERVAL*/{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    /*   BOOLEAN_1      */ {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    /*   CHARACTER_1    */ {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    /*   INTEGER_1      */ {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    /*   REAL_1         */ {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    /*   BOOLEAN_2      */ {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    /*   CHARACTER_2    */ {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    /*   INTEGER_2      */ {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    /*   REAL_2         */ {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}
};

int TypePromote::equalityResultType[16][16] = { // ===, !=
    //                       0   1    2   3   4   5   6   7   8   9  10  11  1 2  13  14  15
    /*   TUPLE          */ { 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    /*   INTERVAL       */ {-1,  2, -1, -1, -1, -1, -1, -1, -1, -1,  2, -1, -1, -1, -1, -1},
    /*   BOOLEAN        */ {-1, -1,  2, -1, -1, -1, -1, -1,  2, -1, -1, -1,  2, -1, -1, -1},
    /*   CHARACTER      */ {-1, -1, -1,  2, -1, -1, -1, -1, -1,  2, -1, -1, -1,  2, -1, -1},
    /*   INTEGER        */ {-1, -1, -1, -1,  2, -1, -1, -1, -1, -1,  2,  2, -1, -1,  2,  2},
    /*   REAL           */ {-1, -1, -1, -1, -1,  2, -1, -1, -1, -1, -1,  2, -1, -1, -1,  2},
    /*   STRING         */ {-1, -1, -1, -1, -1, -1,  2, -1, -1,  2, -1, -1, -1, -1, -1, -1},
    /*   INTEGER_INTERVAL*/{-1, -1, -1, -1, -1, -1, -1,  2, -1, -1, -1, -1, -1, -1, -1, -1},
    /*   BOOLEAN_1      */ {-1, -1,  2, -1, -1, -1, -1, -1,  2, -1, -1, -1, -1, -1, -1, -1},
    /*   CHARACTER_1    */ {-1, -1, -1,  2, -1, -1,  2, -1, -1,  2, -1, -1, -1, -1, -1, -1},
    /*   INTEGER_1      */ {-1, -1, -1, -1,  2, -1, -1, -1, -1, -1,  2, -1, -1, -1, -1, -1},
    /*   REAL_1         */ {-1, -1, -1, -1,  2,  2, -1, -1, -1, -1, -1,  2, -1, -1, -1, -1},
    /*   BOOLEAN_ 2      */{-1, -1,  2, -1, -1, -1, -1, -1, -1, -1, -1, -1,  2, -1, -1, -1},
    /*   CHARACTER_ 2    */{-1, -1, -1,  2, -1, -1, -1, -1, -1, -1, -1, -1, -1,  2, -1, -1},
    /*   INTEGER_ 2      */{-1, -1, -1, -1,  2, -1, -1, -1, -1, -1, -1, -1, -1, -1,  2, -1},
    /*   REAL_ 2         */{-1, -1, -1, -1,  2,  2, -1, -1, -1, -1, -1, -1, -1, -1, -1,  2}
};

int TypePromote::promotionFromTo[16][16] = { // 0 = nullptr
    //                          0   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15
    /*   0  TUPLE          */ { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
    /*   1  INTERVAL       */ { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0},
    /*   2  BOOLEAN        */ { 0,  0,  0,  0,  0,  0,  0,  0,  8,  0,  0,  0, 12,  0,  0,  0},
    /*   3  CHARACTER      */ { 0,  0,  0,  0,  0,  0,  0,  0,  0,  9,  0,  0,  0, 13,  0,  0},
    /*   4  INTEGER        */ { 0,  0,  0,  0,  0,  5,  0,  0,  0,  0, 10, 11,  0,  0, 14, 15},
    /*   5  REAL           */ { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 11,  0,  0,  0, 15},
    /*   6  STRING         */ { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
    /*   7  INTEGER_INTERVAL*/{ 0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10,  0,  0,  0,  0,  0},
    /*   8  BOOLEAN_1      */ { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
    /*   9  CHARACTER_1    */ { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
    /*   10 INTEGER_1      */ { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 11,  0,  0,  0,  0},
    /*   11 REAL_1         */ { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
    /*   12 BOOLEAN_2      */ { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
    /*   13 CHARACTER_2    */ { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
    /*   14 INTEGER_2      */ { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
    /*   15 REAL_2         */ { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}
};

std::shared_ptr<Type> TypePromote::getResultType(int typeTable[16][16], std::shared_ptr<AST> lhs, std::shared_ptr<AST> rhs){
    int lhsType = lhs->evalType->getTypeId();
    int rhsType = rhs->evalType->getTypeId();

    if (lhsType == Type::IDENTITYNULL && rhsType == Type::IDENTITYNULL) {
        std::cout << "Compile-Time-Error! unable to infer type of two identities";
        return nullptr;
    } 
    else if (lhsType == Type::IDENTITYNULL && rhsType != Type::IDENTITYNULL ) {
        lhs->promoteToType = rhs->evalType;
        return rhs->evalType;
    } 
    else if (lhsType != Type::IDENTITYNULL && rhsType == Type::IDENTITYNULL ) {
        rhs->promoteToType = lhs->evalType;
        return lhs->evalType;

    } else { 
        int resTypeId = typeTable[lhsType][rhsType];

        if (resTypeId == -1 ) { 
            std::cout << "Compile-Time-Error: TypeTable invalid result\n";
            return nullptr; 
        }  
        auto newType = this->symtab->getType(resTypeId);

        //lhs promote type
        int lhsPromoteType = this->promotionFromTo[lhsType][resTypeId];
        if (lhsPromoteType != 0) {
            lhs->promoteToType = newType;      
        } 
        //rhs promote type
        int rhsPromoteType = this->promotionFromTo[rhsType][resTypeId];
        if (rhsPromoteType != 0) { 
            rhs->promoteToType = newType; // 
        }

        //return eval type
        return newType;
        }
     
}

} //namespace gazprea
