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
void typeInitFromTwoSingleTerms(Type *this, Type *first, Type *second);
void typeInitFromVectorSizeSpecificationFromLiteral(Type *this, int64_t size, Type *baseType);  // for vector and string
void typeInitFromMatrixSizeSpecificationFromLiteral(Type *this, int64_t nRow, int64_t nCol, Type *baseType);
void typeInitFromIntervalType(Type *this, IntervalTypeBaseTypeID id);
void typeInitFromArrayType(Type *this, bool isString, ElementTypeID eid, int8_t nDim, int64_t *dims);


void typeDestructor(Type *this);                     /// INTERFACE
void typeDestructThenFree(Type *this);               /// INTERFACE

/// INTERFACE
bool typeIsSpecifiable(Type *this);  // basic types and string
bool typeIsConcreteType(Type *this);  // if the type does not have any unknown/unspecified part i.e. a variable can have this type
bool typeIsStream(Type *this);
bool typeIsScalar(Type *this);
bool typeIsScalarBasic(Type *this);
bool typeIsScalarNull(Type *this);
bool typeIsScalarIdentity(Type *this);
bool typeIsScalarInteger(Type *this);
bool typeIsArrayNull(Type *this);
bool typeIsArrayIdentity(Type *this);
bool typeIsUnknown(Type *this);
bool typeIsArrayOrString(Type *this);  // does not include ref type
bool typeIsVectorOrString(Type *this);
bool typeIsMatrix(Type *this);
bool typeIsMixedArray(Type *this);
bool typeIsIntegerInterval(Type *this);
bool typeIsIntegerArray(Type *this);
bool typeIsDomainExprCompatible(Type *this);
bool typeIsIdentical(Type *this, Type *other);  // checks if the two types are describing the same types

///------------------------------COMPOUND TYPE INFO---------------------------------------------------------------
// ArrayType---------------------------------------------------------------------------------------------
typedef struct struct_gazprea_array_type {
    ElementTypeID m_elementTypeID;  // type of the element
    int8_t m_nDim;                  // # of dimensions
    int64_t *m_dims;                // each int64_t specify the length of array in one dimension
    bool m_isOwned;                   // default to true; ownership determines if we are able to free it in destructor
    bool m_isString;
    bool m_isRef;                     // if the array is index reference, default to false
    bool m_isSelfRef;                 // if the array is indexed by itself E.g. a[a], default to false
} ArrayType;

/// allocate
ArrayType *arrayTypeMalloc();
/// constructor
void arrayTypeInitFromVectorSize(ArrayType *this, ElementTypeID elementTypeID, int64_t vecLength, bool isString, bool isOwned);
void arrayTypeInitFromMatrixSize(ArrayType *this, ElementTypeID elementTypeID, int64_t dim1, int64_t dim2, bool isOwned);  // dim1 is #rows
void arrayTypeInitFromCopy(ArrayType *this, ArrayType *other);
void arrayTypeInitFromDims(ArrayType *this, ElementTypeID elementTypeID, int8_t nDim, int64_t *dims,
                           bool isString, bool isOwned, bool isRef, bool isSelfRef);
/// destructor
void arrayTypeDestructor(ArrayType *this);

/// methods
VecToVecRHSSizeRestriction arrayTypeMinimumCompatibleRestriction(ArrayType *this, ArrayType *target);
bool arrayTypeHasUnknownSize(ArrayType *this);
int64_t arrayTypeElementSize(ArrayType *this);
int64_t arrayTypeGetTotalLength(ArrayType *this);
bool arrayTypeCanBinopSameTypeSameSize(ArrayType *result, ArrayType *op1, ArrayType *op2);

// note given a VectorType object, we can't distinguish between
// 1. vector
// 2. vector reference (vec[vec], mat[vec, scalar] or mat[scalar, vec])
// we need to look at Type.m_typeid to determine which one is the actual type of this variable


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
int32_t intervalTypeGetElementAtIndex(const int32_t *ivl, int64_t idx);  // index start at 1
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