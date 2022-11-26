#include <stddef.h>
#include <string.h>
#include "Literal.h"
#include "NDArray.h"
#include "RuntimeErrors.h"
#include "VariableStdio.h"
#include "NDArrayVariable.h"

///------------------------------TYPE---------------------------------------------------------------

// ndarray
void typeInitFromBooleanScalar(Type *this) {
    typeInitFromArrayType(this, false, ELEMENT_BOOLEAN, 0, NULL);
}
void typeInitFromIntegerScalar(Type *this) {
    typeInitFromArrayType(this, false, ELEMENT_INTEGER, 0, NULL);
}
void typeInitFromRealScalar(Type *this) {
    typeInitFromArrayType(this, false, ELEMENT_REAL, 0, NULL);
}
void typeInitFromCharacterScalar(Type *this) {
    typeInitFromArrayType(this, false, ELEMENT_CHARACTER, 0, NULL);
}

void typeInitFromVectorSizeSpecification(Type *this, Variable *size, Type *baseType) {
    // make sure the baseType is an unspecified type
    if (!typeIsSpecifiable(baseType)) {
        singleTypeError(baseType, "Attempt to use type as base type: ");
    }
    if (size == NULL) {
        typeInitFromVectorSizeSpecificationFromLiteral(this, SIZE_UNKNOWN, baseType);
    } else {
        int64_t intSize = variableGetIntegerValue(size);
        if (intSize < 0) {
            errorAndExit("Invalid vector size calculated from variable!");
        }
        typeInitFromVectorSizeSpecificationFromLiteral(this, variableGetIntegerValue(size), baseType);
    }
}

void typeInitFromMatrixSizeSpecification(Type *this, Variable *nRow, Variable *nCol, Type *baseType) {
    if (!typeIsSpecifiable(baseType)) {
        singleTypeError(baseType, "Attempt to use type as base type: ");
    }
    int64_t intNRow = SIZE_UNKNOWN;
    int64_t intNCol = SIZE_UNKNOWN;
    if (nRow) {
        intNRow = variableGetIntegerValue(nRow);
        if (intNRow < 0) {
            errorAndExit("Invalid matrix row size calculated from variable!");
        }
    }
    if (nCol) {
        intNCol = variableGetIntegerValue(nCol);
        if (intNCol < 0) {
            errorAndExit("Invalid matrix row size calculated from variable!");
        }
    }
    typeInitFromMatrixSizeSpecificationFromLiteral(this,intNRow, intNCol, baseType);
}

void typeInitFromUnspecifiedString(Type *this) {
    int64_t dims[1] = {SIZE_UNSPECIFIED };
    typeInitFromArrayType(this, true, ELEMENT_CHARACTER, 1, dims);
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

void typeInitFromUnknownType(Type *this) {
    this->m_typeId = TYPEID_UNKNOWN;
    this->m_compoundTypeInfo = NULL;
}

///------------------------------TUPLE---------------------------------------------------------------

void variableSetStrIdArray(Variable *this, int64_t *stridArray) {
    if (this->m_type->m_typeId != TYPEID_TUPLE) {
        singleTypeError(this->m_type, "Attempt to set strIdArray of variable of type:");
    }
    TupleType *CTI = this->m_type->m_compoundTypeInfo;
    memcpy(CTI->m_idxToStrid, stridArray, CTI->m_nField * sizeof(int64_t));
}

Type *variableGetType(Variable *this) {
    return this->m_type;
}

Type *variableSwapType(Variable *this, Type *newType) {
    if (this->m_type->m_typeId != TYPEID_TUPLE || newType->m_typeId != TYPEID_TUPLE) {
        return NULL;  // do not swap
    }
    // TODO: type checking between type pair
    Type *temp = this->m_type;
    this->m_type = newType;
    return temp;
}

///------------------------------VARIABLE---------------------------------------------------------------

void variableInitFromBooleanScalar(Variable *this, bool value) {
    variableInitFromNDArray(this, false, ELEMENT_BOOLEAN, 0, NULL, &value, false);
}

void variableInitFromIntegerScalar(Variable *this, int32_t value){
    variableInitFromNDArray(this, false, ELEMENT_INTEGER, 0, NULL, &value, false);
}

void variableInitFromRealScalar(Variable *this, float value){
    variableInitFromNDArray(this, false, ELEMENT_REAL, 0, NULL, &value, false);
}

void variableInitFromCharacterScalar(Variable *this, int8_t value){
    variableInitFromNDArray(this, false, ELEMENT_CHARACTER, 0, NULL, &value, false);
}

void variableInitFromNullScalar(Variable *this) {
    variableInitFromNDArray(this, false, ELEMENT_NULL, 0, NULL, NULL, false);
}

void variableInitFromIdentityScalar(Variable *this){
    variableInitFromNDArray(this, false, ELEMENT_IDENTITY, 0, NULL, NULL, false);
}

void variableInitFromMatrixLiteralHelper(Variable *this, int64_t nVars, Variable **vars, int64_t longestLen) {
    MixedTypeElement mixedTemplate = {ELEMENT_NULL, NULL };
    int64_t dims[2] = {nVars, longestLen};
    this->m_type = typeMalloc();
    typeInitFromArrayType(this->m_type, false, ELEMENT_MIXED, 2, dims);
    MixedTypeElement *arr = arrayMallocFromElementValue(ELEMENT_MIXED, dims[0] * dims[1], &mixedTemplate);

    for (int64_t i = 0; i < nVars; i++) {
        Variable *curVar = vars[i];
        Type *curType = curVar->m_type;
        for (int64_t j = 0; j < longestLen; j++) {
            MixedTypeElement *curElement = arr + i * longestLen + j;
            if (curType->m_typeId != TYPEID_NDARRAY)
                singleTypeError(curType, "Invalid type in an array (matrix) literal:");

            int64_t size = variableGetLength(vars[i]);
            if (j < size) {
                ArrayType *CTI = curType->m_compoundTypeInfo;
                ElementTypeID eid = CTI->m_elementTypeID;
                void *elementPtr = arrayGetElementPtrAtIndex(eid, curVar->m_data, j);
                if (eid == ELEMENT_MIXED) {
                    MixedTypeElement *element = elementPtr;
                    mixedTypeElementInitFromValue(curElement, element->m_elementTypeID,
                                                  element->m_element);
                } else {
                    mixedTypeElementInitFromValue(curElement, eid, elementPtr);
                }
            } else {
                mixedTypeElementInitFromValue(curElement, ELEMENT_NULL, NULL);
            }
        }
    }

    this->m_data = arr;
    variableAttrInitHelper(this, -1, this->m_data, false);
}

void variableInitFromVectorLiteral(Variable *this, int64_t nVars, Variable **vars) {
    // one pass to check if the result is a vector or a matrix, another to convert it into mixed array

    bool isMatrix = false;
    for (int64_t i = 0; i < nVars; i++) {
        int8_t nDim = variableGetNDim(vars[i]);
        if (nDim == 1 || nDim == DIM_UNSPECIFIED) {
            isMatrix = true;
            break;
        } else if (nDim > 1 || nDim == DIM_INVALID)
            singleTypeError(vars[i]->m_type, "Vector literal contains type:");
    }

    int64_t longestLen = 0;
    if (isMatrix) {
        // one more pass to find out the longest vector among all rows
        for (int64_t i = 0; i < nVars; i++) {
            int64_t size = variableGetLength(vars[i]);
            longestLen = longestLen > size ? longestLen : size;
        }
    }

    Variable **modifiedVars = malloc(nVars * sizeof(Variable *));
    for (int64_t i = 0; i < nVars; i++) {
        if (isMatrix && typeIsScalar(vars[i]->m_type)) {
            int64_t dims[1] = {longestLen};
            variableInitFromScalarToConcreteArray(modifiedVars[i], vars[i], 1, dims, false);
        } else if (variableGetIndexRefTypeID(vars[i]) != NDARRAY_INDEX_REF_NOT_A_REF) {
            variableInitFromNDArrayIndexRefToValue(modifiedVars[i], vars[i]);
        } else {  // do not modify
            modifiedVars[i] = vars[i];
        }
    }

    if (nVars == 0) {  // empty array
        this->m_type = typeMalloc();
        MixedTypeElement mixedTemplate = {ELEMENT_NULL, NULL };
        typeInitFromArrayType(this->m_type, false, ELEMENT_MIXED, DIM_UNSPECIFIED, NULL);
        this->m_data = arrayMallocFromElementValue(ELEMENT_MIXED, 0, &mixedTemplate);;
    } else if (isMatrix) {
        // matrix literal
        variableInitFromMatrixLiteralHelper(this, nVars, modifiedVars, longestLen);
    } else {
        // vector literal
        this->m_type = typeMalloc();
        MixedTypeElement mixedTemplate = {ELEMENT_NULL, NULL };
        int64_t dims[1] = {nVars};
        typeInitFromArrayType(this->m_type, false, ELEMENT_MIXED, 1, dims);
        MixedTypeElement *arr = arrayMallocFromElementValue(ELEMENT_MIXED, nVars, &mixedTemplate);
        for (int64_t i = 0; i < nVars; i++) {
            MixedTypeElement *curElement = arr + i;
            Variable *rhs = modifiedVars[i];
            Type *rhsType = rhs->m_type;
            switch(rhsType->m_typeId) {
                case TYPEID_NDARRAY: {
                    if (!typeIsScalar(rhsType)) {
                        singleTypeError(rhsType, "Nonscalar type in an array (vector) literal:");
                    }
                    ArrayType *CTI = rhsType->m_compoundTypeInfo;
                    mixedTypeElementInitFromValue(curElement, CTI->m_elementTypeID, rhs->m_data);
                } break;
                default:
                    singleTypeError(rhsType, "Invalid type in an array (vector) literal:");
            }
        }

        this->m_data = arr;
        variableAttrInitHelper(this, -1, this->m_data, false);
    }

    for (int64_t i = 0; i < nVars; i++) {
        if (modifiedVars[i] != vars[i])
            variableDestructThenFree(modifiedVars[i]);
    }
    free(modifiedVars);
#ifdef DEBUG_PRINT
    variableInitDebugPrint(this, "from vector literal");
#endif
}

void variableInitFromString(Variable *this, int64_t strLength, int8_t *str) {
    int64_t dims[1] = {strLength};
    variableInitFromNDArray(this, true, ELEMENT_CHARACTER, 1, dims, str, false);
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

    variableAttrInitHelper(this, -1, this->m_data, false);
#ifdef DEBUG_PRINT
    variableInitDebugPrint(this, "from tuple literal");
#endif
}

void variableInitFromStdInput(Variable *this) {
    variableEmptyInitFromTypeID(this, TYPEID_STREAM_IN);
#ifdef DEBUG_PRINT
    variableInitDebugPrint(this, "a std_input");
#endif
}

void variableInitFromStdOutput(Variable *this) {
    variableEmptyInitFromTypeID(this, TYPEID_STREAM_OUT);
#ifdef DEBUG_PRINT
    variableInitDebugPrint(this, "a std_output");
#endif
}

void variableInitFromEmptyArray(Variable *this) {
    variableInitFromVectorLiteral(this, 0, NULL);
#ifdef DEBUG_PRINT
    variableInitDebugPrint(this, "an empty array");
#endif
}