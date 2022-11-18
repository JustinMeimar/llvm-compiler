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


void typeInitFromUnspecifiedInterval(Type *this) {
    typeInitFromIntervalType(this, UNSPECIFIED_BASE_INTERVAL);
}

void typeInitFromIntegerInterval(Type *this) {
    typeInitFromIntervalType(this, INTEGER_BASE_INTERVAL);
}

void typeInitFromTupleType(Type *this, int64_t nField, Type **typeArray, int64_t *stridArray) {
    this->m_typeId = TYPEID_TUPLE;
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

///------------------------------TUPLE---------------------------------------------------------------

void variableSetStrIdArray(Variable *this, int64_t *stridArray) {
    if (this->m_type->m_typeId != TYPEID_TUPLE) {
        targetTypeError(this->m_type, "Attempt to set strIdArray of variable of type:");
    }
    TupleType *CTI = this->m_type->m_compoundTypeInfo;
    memcpy(CTI->m_idxToStrid, stridArray, CTI->m_nField * sizeof(int64_t));
}

Type *variableGetType(Variable *this) {
    return this->m_type;
}

Type *variableSwapType(Variable *this, Type *newType) {
    if (this->m_type->m_typeId != TYPEID_TUPLE || newType->m_typeId != TYPEID_TUPLE) {
        return this->m_type;  // do not swap
    }
    // TODO: type checking between type pair
    Type *temp = this->m_type;
    this->m_type = newType;
    return temp;
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

void variableInitFromVectorLiteral(Variable *this, int64_t nVars, Variable **vars) {
    // one pass to check if the result is a vector or a matrix, another to convert it into mixed array
    bool isMatrix = false;
    for (int64_t i = 0; i < nVars; i++) {
        int8_t nDim = variableGetNDim(vars[i]);
        if (nDim > 1)
            targetTypeError(vars[i]->m_type, "Vector literal contains type:");
        else if (nDim == 1) {
            isMatrix = true;
            break;
        }
    }
    MixedTypeElement mixedTemplate = {ELEMENT_NULL, NULL };
    this->m_type = typeMalloc();
    if (isMatrix) {
        // matrix literal
        // one more pass to find out the longest vector among all rows
        int64_t width = 0;
        for (int64_t i = 0; i < nVars; i++) {
            int64_t size = variableGetLength(vars[i]);
            width = width > size ? width : size;
        }
        int64_t dims[2] = {nVars, width};
        typeInitFromArrayType(this->m_type, TYPEID_NDARRAY, ELEMENT_MIXED, 2, dims);
        MixedTypeElement *arr = arrayMallocFromElementValue(ELEMENT_MIXED, dims[0] * dims[1], &mixedTemplate);

        for (int64_t i = 0; i < nVars; i++) {
            Variable *rhs = vars[i];
            Type *rhsType = rhs->m_type;
            for (int64_t j = 0; j < width; j++) {
                MixedTypeElement *curElement = arr + i * width + j;
                switch(rhsType->m_typeId) {
                    case TYPEID_NDARRAY:
                    case TYPEID_STRING: {
                        if (variableGetNDim(rhs) != 1) {
                            targetTypeError(rhsType, "Invalid dimension in an array (matrix) literal:");
                        }
                        int64_t size = variableGetLength(vars[i]);
                        if (j < size) {
                            ArrayType *CTI = rhsType->m_compoundTypeInfo;
                            ElementTypeID eid = CTI->m_elementTypeID;
                            void *elementPtr = arrayGetElementPtrAtIndex(eid, rhs->m_data, j);
                            if (eid == ELEMENT_MIXED) {
                                MixedTypeElement *element = elementPtr;
                                mixedTypeElementInitFromValue(curElement, element->m_elementTypeID, element->m_element);
                            } else {
                                mixedTypeElementInitFromValue(curElement, eid, elementPtr);
                            }
                        } else {
                            mixedTypeElementInitFromValue(curElement, ELEMENT_NULL, NULL);
                        }
                    } break;
                    case TYPEID_EMPTY_ARRAY: {
                        mixedTypeElementInitFromValue(curElement, ELEMENT_NULL, NULL);
                    } break;
                    default:
                        targetTypeError(rhsType, "Invalid type in an array (matrix) literal:");
                }
            }
        }

        this->m_data = arr;
    } else {
        // vector literal
        int64_t dims[1] = {nVars};
        typeInitFromArrayType(this->m_type, TYPEID_NDARRAY, ELEMENT_MIXED, 1, dims);
        MixedTypeElement *arr = arrayMallocFromElementValue(ELEMENT_MIXED, nVars, &mixedTemplate);
        for (int64_t i = 0; i < nVars; i++) {
            MixedTypeElement *curElement = arr + i;
            Variable *rhs = vars[i];
            Type *rhsType = rhs->m_type;
            switch(rhsType->m_typeId) {
                case TYPEID_NDARRAY: {
                    if (!typeIsScalar(rhsType)) {
                        targetTypeError(rhsType, "Nonscalar type in an array (vector) literal:");
                    }
                    ArrayType *CTI = rhsType->m_compoundTypeInfo;
                    mixedTypeElementInitFromValue(curElement, CTI->m_elementTypeID, rhs->m_data);
                } break;
                default:
                    targetTypeError(rhsType, "Invalid type in an array (vector) literal:");
            }
        }

        this->m_data = arr;
    }
    this->m_parent = this->m_data;
    this->m_fieldPos = -1;
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