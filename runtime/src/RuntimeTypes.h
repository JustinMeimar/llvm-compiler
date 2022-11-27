#pragma once
#include <stdint.h>
#include "Bool.h"
#include "Enums.h"

// forward declaration
typedef struct struct_gazprea_variable Variable;
typedef struct struct_gazprea_pcadp_config PCADPConfig;
typedef struct struct_gazprea_type Type;
typedef enum enum_vectovec_rhssize_restriction VecToVecRHSSizeRestriction;

///------------------------------TYPE---------------------------------------------------------------

typedef struct struct_gazprea_type {
    TypeID m_typeId;
    void *m_compoundTypeInfo;  // for compound type only
} Type;

Type *typeMalloc();                                  /// INTERFACE

void typeInitFromCopy(Type *this, Type *other);
void typeInitFromTwoSingleTerms(Type *this, Type *first, Type *second);     /// INTERFACE
void typeInitFromIntervalType(Type *this, IntervalTypeBaseTypeID id);


void typeDestructor(Type *this);                     /// INTERFACE
void typeDestructThenFree(Type *this);               /// INTERFACE

/// INTERFACE
bool typeIsVariableClassCompatible(Type *this);  // if the type does not have any unknown/unspecified part i.e. a variable can have this type
bool typeIsStream(Type *this);
bool typeIsUnknown(Type *this);
bool typeIsIntegerInterval(Type *this);
bool typeIsUnspecifiedInterval(Type *this);
bool typeIsIdentical(Type *this, Type *other);  // checks if the two types are describing the same types
bool typeIsDomainExprCompatible(Type *this);

///------------------------------COMPOUND TYPE INFO---------------------------------------------------------------
// IntervalType---------------------------------------------------------------------------------------------
typedef struct struct_gazprea_interval_type {
    IntervalTypeBaseTypeID m_baseTypeID;
} IntervalType;

IntervalType *intervalTypeMalloc();
void *intervalTypeInitFromBase(IntervalType *this, IntervalTypeBaseTypeID id);
void *intervalTypeInitFromCopy(IntervalType *this, IntervalType *other);

void *intervalTypeMallocDataFromNull();
void *intervalTypeMallocDataFromIdentity();
void *intervalTypeMallocDataFromHeadTail(int32_t head, int32_t tail);
void *intervalTypeMallocDataFromCopy(void *otherIntervalData);
void intervalTypeFreeData(void *data);

bool intervalTypeIsUnspecified(IntervalType *this);  // if the base type is unspecified, otherwise the base type is integer
int32_t intervalTypeGetElementAtIndex(const int32_t *ivl, int64_t idx);
// should support self assignment
void intervalTypeUnaryPlus(int32_t *result, const int32_t *op);
void intervalTypeUnaryMinus(int32_t *result, const int32_t *op);
void intervalTypeBinaryPlus(int32_t *result, const int32_t *op1, const int32_t *op2);
void intervalTypeBinaryMinus(int32_t *result, const int32_t *op1, const int32_t *op2);
void intervalTypeBinaryMultiply(int32_t *result, const int32_t *op1, const int32_t *op2);
void intervalTypeBinaryEq(bool *result, const int32_t *op1, const int32_t *op2);
void intervalTypeBinaryNe(bool *result, const int32_t *op1, const int32_t *op2);


// TupleType---------------------------------------------------------------------------------------------

typedef struct struct_gazprea_tuple_type {
    Type *m_fieldTypeArr;
    int64_t *m_idxToStrid;  // maps identifier access to position of field in the tuple; of size 2 * m_nField
    int64_t m_nField;
} TupleType;

TupleType *tupleTypeMalloc();
void tupleTypeInitFromTypeAndId(TupleType *this, int64_t nField, Type **typeArray, int64_t *stridArray);
void tupleTypeInitFromCopy(TupleType *this, TupleType *other);
void tupleTypeDestructor(TupleType *this);

int64_t tupleTypeResolveId(TupleType *this, int64_t id);
void *tupleTypeMallocDataFromNull(TupleType *this);
void *tupleTypeMallocDataFromIdentity(TupleType *this);
void *tupleTypeMallocDataFromCopy(TupleType *this, void *otherTupleData);
void *tupleTypeMallocDataFromCopyVariableArray(TupleType *this, Variable **vars);
void *tupleTypeMallocDataFromPCADP(TupleType *this, Variable *src, PCADPConfig *config);
void tupleTypeFreeData(TupleType *this, void *data);

void *variableArrayMalloc(int64_t size);                            /// INTERFACE
void variableArraySet(Variable **arr, int64_t idx, Variable *var);  /// INTERFACE
void variableArrayFree(Variable **arr);                             /// INTERFACE

void *typeArrayMalloc(int64_t size);                                /// INTERFACE
void typeArraySet(Type **arr, int64_t idx, Type *var);              /// INTERFACE
void typeArrayFree(Type **arr);                                     /// INTERFACE

void *stridArrayMalloc(int64_t size);                               /// INTERFACE
void stridArraySet(int64_t *arr, int64_t idx, int64_t val);         /// INTERFACE
void stridArrayFree(int64_t *arr);                                  /// INTERFACE