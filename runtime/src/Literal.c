#include <stddef.h>
#include <string.h>
#include "Literal.h"
#include "NDArray.h"
#include "RuntimeErrors.h"

///------------------------------TYPE---------------------------------------------------------------

// ndarray
void typeInitFromBooleanScalar(Type *this) {
    typeInitFromArrayType(this, TYPEID_NDARRAY, ELEMENT_BOOLEAN, 0, NULL);
}
void typeInitFromIntegerScalar(Type *this) {
    typeInitFromArrayType(this, TYPEID_NDARRAY, ELEMENT_INTEGER, 0, NULL);
}
void typeInitFromRealScalar(Type *this) {
    typeInitFromArrayType(this, TYPEID_NDARRAY, ELEMENT_REAL, 0, NULL);
}
void typeInitFromCharacterScalar(Type *this) {
    typeInitFromArrayType(this, TYPEID_NDARRAY, ELEMENT_CHARACTER, 0, NULL);
}

void typeInitFromVectorSizeSpecification(Type *this, Variable *size, Type *baseType) {
    // make sure the baseType is an unspecified type
    if (!typeIsSpecifiable(baseType)) {
        targetTypeError(baseType, "Attempt to use type as base type: ");
    }
    if (size == NULL) {
        typeInitFromVectorSizeSpecificationFromLiteral(this, -1, baseType);
    } else {
        typeInitFromVectorSizeSpecificationFromLiteral(this, variableGetIntegerValue(size), baseType);
    }
}

void typeInitFromMatrixSizeSpecification(Type *this, Variable *nRow, Variable *nCol, Type *baseType) {
    if (!typeIsSpecifiable(baseType)) {
        targetTypeError(baseType, "Attempt to use type as base type: ");
    }
    typeInitFromMatrixSizeSpecificationFromLiteral(this,
            nRow ? variableGetIntegerValue(nRow) : -1,
            nCol ? variableGetIntegerValue(nCol) : -1,
            baseType);
}

void typeInitFromUnspecifiedString(Type *this) {
    int64_t dims[1] = {SIZE_UNSPECIFIED };
    typeInitFromArrayType(this, TYPEID_NDARRAY, ELEMENT_CHARACTER, 1, dims);
    this->m_typeId = TYPEID_STRING;
}

void variableInitFromArrayLiteral(Variable *this, int64_t nVars, Variable **vars) {
    // TODO
}


void typeInitFromUnspecifiedInterval(Type *this) {
    typeInitFromIntervalType(this, UNSPECIFIED_BASE_INTERVAL);
}

void typeInitFromIntegerInterval(Type *this) {
    typeInitFromIntervalType(this, INTEGER_BASE_INTERVAL);
}

void typeInitFromTupleType(Type *this, int64_t nField, Type **typeArray, int64_t *stridArray) {
    this->m_compoundTypeInfo = tupleTypeMalloc();
    tupleTypeInitFromTypeAndId(this->m_compoundTypeInfo, nField, typeArray, stridArray);
}

void typeInitFromEmptyArray(Type *this) {
    this->m_typeId = TYPEID_EMPTY_ARRAY;
    this->m_compoundTypeInfo = NULL;
}

void typeInitFromUnknownType(Type *this) {
    this->m_typeId = TYPEID_UNKNOWN;
    this->m_compoundTypeInfo = NULL;
}


///------------------------------VARIABLE---------------------------------------------------------------

void variableInitFromBooleanScalar(Variable *this, bool value) {
    variableInitFromNDArray(this, TYPEID_NDARRAY, ELEMENT_BOOLEAN, 0, NULL, &value, false);
}

void variableInitFromIntegerScalar(Variable *this, int32_t value){
    variableInitFromNDArray(this, TYPEID_NDARRAY, ELEMENT_INTEGER, 0, NULL, &value, false);
}

void variableInitFromRealScalar(Variable *this, float value){
    variableInitFromNDArray(this, TYPEID_NDARRAY, ELEMENT_REAL, 0, NULL, &value, false);
}

void variableInitFromCharacterScalar(Variable *this, int8_t value){
    variableInitFromNDArray(this, TYPEID_NDARRAY, ELEMENT_CHARACTER, 0, NULL, &value, false);
}

void variableInitFromNullScalar(Variable *this) {
    variableInitFromNDArray(this, TYPEID_NDARRAY, ELEMENT_NULL, 0, NULL, NULL, false);
}

void variableInitFromIdentityScalar(Variable *this){
    variableInitFromNDArray(this, TYPEID_NDARRAY, ELEMENT_IDENTITY, 0, NULL, NULL, false);
}

void variableInitFromString(Variable *this, int64_t strLength, int8_t *str) {
    int64_t dims[1] = {strLength};
    variableInitFromNDArray(this, TYPEID_STRING, ELEMENT_CHARACTER, 1, dims, str, false);
}

void variableInitFromTupleLiteral(Variable *this, int64_t nField, Variable **vars) {
    this->m_type = typeMalloc();
    Type *(types[nField]);
    for (int64_t i = 0; i < nField; i++) {
        types[i] = vars[i]->m_type;
    }
    typeInitFromTupleType(this->m_type, nField, types, NULL);

    // initialize m_data
    this->m_data = tupleTypeMallocDataFromCopyVariableArray(this->m_type->m_compoundTypeInfo, vars);

    this->m_fieldPos = -1;
    this->m_parent = this->m_data;
}

void variableInitFromStdInput(Variable *this) {
    variableEmptyInitFromTypeID(this, TYPEID_STREAM_IN);
}

void variableInitFromStdOutput(Variable *this) {
    variableEmptyInitFromTypeID(this, TYPEID_STREAM_OUT);
}

void variableInitFromEmptyArray(Variable *this) {
    variableEmptyInitFromTypeID(this, TYPEID_EMPTY_ARRAY);
}