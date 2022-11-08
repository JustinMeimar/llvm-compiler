#pragma once

#include <stdint.h>
#include "RuntimeTypes.h"

///------------------------------VARIABLE---------------------------------------------------------------
typedef struct struct_gazprea_type {
    TypeID m_typeId;
    void *m_compoundTypeInfo;  // for compound type only
} Type;

typedef struct struct_gazprea_variable {
    Type *m_type;
    void *m_parent;  // used to avoid memory location aliasing in case of vector, matrix and tuple
    int64_t m_fieldPos;  // used to avoid tuple field aliasing
    void *m_data;  // stores the value of the variable
} Variable;

Variable *variableMalloc();

typedef enum enum_vectovec_rhssize_restriction {
    vectovec_rhs_must_be_same_size,
    vectovec_rhs_size_must_not_be_greater,
    vectovec_rhs_size_can_be_any,
} VecToVecRHSSizeRestriction;

typedef struct struct_gazprea_pcadp_config {
    bool m_allowUnknownTargetType;  // i.e. var a = <expr>;
    bool m_allowUnknownTargetArraySize;  // integer[*] a = [1, 2];
    bool m_resultCanBeRefType;  // call p(vec[1..3])
    bool m_allowIndexedAssignmentAfterConversion;  // i.e. a[1..3] = [1, 2, 3];
    VecToVecRHSSizeRestriction m_rhsSizeRestriction;
    bool m_allowArrToArrDifferentElementTypeConversion;
    bool m_allowRHSVectorLiteralDirectPromotion;
    bool m_isCast;  // including null/identity matrices
} PCADPConfig;

void variableInitFromPCADP(Variable *this, Type *targetType, Variable *rhs, PCADPConfig *config);
void variableInitFromMemcpy(Variable *this, Variable *other);
void variableInitFromNull(Variable *this, Type *type);
void variableInitFromIdentity(Variable *this, Type *type);
void variableInitFromUnaryOp(Variable *this, Variable *operand, UnaryOpCode opcode);
void variableInitFromBinaryOp(Variable *this, Variable *op1, Variable *op2, BinOpCode opcode);
void variableInitFromParameter(Variable *this, Type *lhsType, Variable *rhs);
void variableInitFromCast(Variable *this, Type *lhsType, Variable *rhs);
void variableInitFromAssign(Variable *this, Type *lhsType, Variable *rhs);
void variableInitFromDeclaration(Variable *this, Type *lhsType, Variable *rhs);
void variableInitFromPromotion(Variable *this, Type *lhsType, Variable *rhs);
void variableInitFromMixedArrayPromoteToSameType(Variable *this, Variable *mixed);
void variableInitFromIntervalHeadTail(Variable *this, Variable *head, Variable *tail);
void variableInitFromIntervalStep(Variable *this, Variable *ivl, Variable *step);  // the new variable is a vector
void variableDestructor(Variable *this);
void variableDestructThenFree(Variable *this);

void variableBinOpBothSidesPromotion(Variable *this, Variable *op1, Variable *op2, Type *op1Target, Type *op2Target);
bool variableAliasWith(Variable *this, Variable *other);  // return ture if the two variable alias
void variableVectorIndexAssignment(Variable *vector, Variable *index, Variable *rhs);
void variableMatrixIndexAssignment(Variable *vector, Variable *rowIndex, Variable *colIndex, Variable *rhs);

// debug print to stdout
void variableDebugPrint(Type *this);