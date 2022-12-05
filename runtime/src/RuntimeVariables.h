#pragma once

#include <stdint.h>
#include "RuntimeTypes.h"

///------------------------------VARIABLE---------------------------------------------------------------

typedef struct struct_gazprea_variable {
    Type *m_type;
    void *m_data;  // stores the value of the variable

    int64_t m_fieldPos;  // used to avoid tuple field aliasing
    void *m_parent;  // used to avoid memory location aliasing in case of vector, matrix and tuple
    bool m_isBlockScoped;
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
    bool m_resultCanBeRefType;  // call p(vec[1..3]);  unused because the LLVM side implementation does not call PCADP when result can be a ref
    bool m_allowIndexedAssignmentAfterConversion;  // i.e. a[1..3] = [1, 2, 3];
    VecToVecRHSSizeRestriction m_rhsSizeRestriction;
    bool m_allowArrToArrDifferentElementTypeConversion;
    bool m_allowRHSVectorLiteralDirectPromotion;
    bool m_isCast;  // including null/identity matrices
    bool m_allowRHSNullIdentity;
    bool m_resultIsBlockScoped;  // only true for declaration and parameter
} PCADPConfig;

extern PCADPConfig pcadpParameterConfig;
extern PCADPConfig pcadpCastConfig;
extern PCADPConfig pcadpAssignmentConfig;
extern PCADPConfig pcadpDeclarationConfig;
extern PCADPConfig pcadpPromotionConfig;
extern PCADPConfig pcadpDomainExpressionConfig;

void variableInitFromPCADP(Variable *this, Type *targetType, Variable *rhs, PCADPConfig *config);
void variableInitFromMemcpy(Variable *this, Variable *other);
void variableInitFromIdentifier(Variable *this, Variable *other);                                 /// INTERFACE
void variableInitFromNull(Variable *this, Type *type);
void variableInitFromIdentity(Variable *this, Type *type);
void variableInitFromUnaryOp(Variable *this, Variable *operand, UnaryOpCode opcode);              /// INTERFACE
void variableInitFromBinaryOp(Variable *this, Variable *op1, Variable *op2, BinOpCode opcode);    /// INTERFACE

void variableInitFromParameter(Variable *this, Type *lhsType, Variable *rhs);                     /// INTERFACE
void variableInitFromCast(Variable *this, Type *lhsType, Variable *rhs);                          /// INTERFACE
void variableInitFromDeclaration(Variable *this, Type *lhsType, Variable *rhs);                   /// INTERFACE
void variableInitFromAssign(Variable *this, Type *lhsType, Variable *rhs);
void variableInitFromPromotion(Variable *this, Type *lhsType, Variable *rhs);                     /// INTERFACE
void variableInitFromDomainExpression(Variable *this, Variable *rhs);                             /// INTERFACE
void variableInitFromPCADP1dVariableToVector(Variable *this, Variable *rhs, PCADPConfig *config);
void variableInitFromPCADPToIntegerVector(Variable *this, Variable *rhs, PCADPConfig *config);
void variableInitFromPCADPToIntegerScalar(Variable *this, Variable *rhs, PCADPConfig *config);
void variableInitFromScalarToConcreteArray(Variable *this, Variable *scalar, int8_t nDim, int64_t *dims, bool isString);

void variableInitFromMixedArrayPromoteToSameType(Variable *this, Variable *mixed);
void variableInitFromIntervalHeadTail(Variable *this, Variable *head, Variable *tail);
void variableInitFromIntervalStep(Variable *this, Variable *ivl, Variable *step);  // the new variable is a vector
void variableInitFromNDArray(Variable *this, bool isString, ElementTypeID eid, int8_t nDim, int64_t *dims,
                             void *value, bool valueIsScalar);
void variableDestructor(Variable *this);                                                          /// INTERFACE
void variableDestructThenFree(Variable *this);                                                    /// INTERFACE
void variableDestructThenFreeImpl(Variable *this);

/// INTERFACE - for domain variable i.e. "i in 1..10"
bool variableIsIntegerInterval(Variable *this);
bool variableIsIntegerArray(Variable *this);
bool variableIsDomainExprCompatible(Variable *this);
int64_t variableGetLength(Variable *this);
void variableInitFromArrayElementAtIndex(Variable *this, Variable *arr, int64_t idx);
void variableInitFromIntegerArrayElementAtIndex(Variable *this, Variable *arr, int64_t idx);
int32_t variableGetIntegerElementAtIndex(Variable *this, int64_t idx);  // works for both vectors and integer intervals

void variableSetIsBlockScoped(Variable *this, bool isBlockScoped);
Variable *variableConvertLiteralAndRefToConcreteArray(Variable *arr);  // return NULL if need not convert (not literal or ref); will not convert empty array
int8_t variableGetNDim(Variable *this);  // returns SIZE_UNKNOWN for types other than array, string or interval
void variableEmptyInitFromTypeID(Variable *this, TypeID id);  // a helper function for other inits
void variableAttrInitHelper(Variable *this, int64_t fieldPos, void *m_parent, bool m_isBlockScoped);
// promote to integer scalar and return the value as int32_t
int32_t variableGetIntegerValue(Variable *this);                                                  /// INTERFACE
bool variableGetBooleanValue(Variable *this);                                                     /// INTERFACE
Variable *variableGetTupleField(Variable *tuple, int64_t pos);                                    /// INTERFACE
Variable *variableGetTupleFieldFromID(Variable *tuple, int64_t id);                               /// INTERFACE
int64_t variableGetNumFieldInTuple(Variable *this);                                               /// INTERFACE
bool variableAliasWith(Variable *this, Variable *other);                                          /// INTERFACE return ture if the two variable alias
void variableAssignment(Variable *this, Variable *rhs);                                           /// INTERFACE