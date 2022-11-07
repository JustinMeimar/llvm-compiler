#include <stddef.h>
#include "RuntimeTypes.h"
#include <stdlib.h>
#include <string.h>
#include "RuntimeErrors.h"
#include "RuntimeVariables.h"

///------------------------------TYPE AND VARIABLE---------------------------------------------------------------

Type *typeMalloc() {
    return malloc(sizeof(Type));
}

void typeInitFromCopy(Type *this, Type *other) {
    TypeID otherID = other->m_typeId;
    this->m_typeId = otherID;
    if (!typeIsBasicType(other) || otherID == typeid_interval) {
        // nothing to copy
        this->m_compoundTypeInfo = NULL;
    } else if (otherID == typeid_ndarray || otherID == typeid_string) {
        this->m_compoundTypeInfo = arrayTypeMalloc();
        arrayTypeInitFromCopy(this->m_compoundTypeInfo, other->m_compoundTypeInfo);
    } else if (otherID == typeid_tuple) {
        this->m_compoundTypeInfo = tupleTypeMalloc();
        tupleTypeInitFromCopy(this->m_compoundTypeInfo, other->m_compoundTypeInfo);
    } else {
        targetTypeError(other, "Failed to copy from type: ");
    }
}

void typeInitFromVectorSizeSpecification(Type *this, int64_t size, Type *baseType) {
    ArrayType *baseTypeCTI = (ArrayType *) baseType->m_compoundTypeInfo;
    this->m_typeId = baseType->m_typeId;  // whether baseType is scalar ndarray or string, the result of specification has the same type as baseType
    if (typeIsBasicType(baseType)) {  // do nothing
    } else if (baseType->m_typeId == typeid_string) {
        if (baseTypeCTI->m_dims[0] != SIZE_UNSPECIFIED)
            targetTypeError(baseType, "Attempt to specify size for a string that has already specified type! ");
    } else {
        targetTypeError(baseType, "Invalid base type: ");
    }
    ArrayType *CTI = arrayTypeMalloc();
    arrayTypeInitFromVectorSize(CTI, baseTypeCTI->m_elementTypeID, size);
    this->m_compoundTypeInfo = (void *) CTI;
}

void typeInitFromMatrixSizeSpecification(Type *this, int64_t nRow, int64_t nCol, Type *baseType) {
    ArrayType *baseTypeCTI = (ArrayType *) baseType->m_compoundTypeInfo;
    this->m_typeId = baseType->m_typeId;
    if (typeIsBasicType(baseType)) {  // do nothing
    } else {
        targetTypeError(baseType, "Invalid base type: ");
    }
    ArrayType *CTI = arrayTypeMalloc();
    arrayTypeInitFromMatrixSize(CTI, baseTypeCTI->m_elementTypeID, nRow, nCol);
    this->m_compoundTypeInfo = (void *) CTI;
}

void typeInitFromIntervalType(Type *this) {
    this->m_compoundTypeInfo = NULL;
    this->m_typeId = typeid_interval;
}

bool typeIsScalarNull(Type *this) {
    if (this->m_typeId == typeid_ndarray) {
        ArrayType *CTI = this->m_compoundTypeInfo;
        if (CTI->m_elementTypeID == element_null && CTI->m_nDim == 0)
            return true;
    }
    return false;
}

bool typeIsScalarIdentity(Type *this) {
    if (this->m_typeId == typeid_ndarray) {
        ArrayType *CTI = this->m_compoundTypeInfo;
        if (CTI->m_elementTypeID == element_identity && CTI->m_nDim == 0)
            return true;
    }
    return false;
}

bool typeIsArrayNull(Type *this) {
    if (this->m_typeId == typeid_ndarray) {
        ArrayType *CTI = this->m_compoundTypeInfo;
        if (CTI->m_elementTypeID == element_null)
            return true;
    }
    return false;
}

bool typeIsArrayIdentity(Type *this) {
    if (this->m_typeId == typeid_ndarray) {
        ArrayType *CTI = this->m_compoundTypeInfo;
        if (CTI->m_elementTypeID == element_identity)
            return true;
    }
    return false;
}

bool typeIsUnknown(Type *this) {
    return this->m_typeId == typeid_unknown;
}

bool typeIsBasicType(Type *this) {
    TypeID id = this->m_typeId;
    if (id != typeid_ndarray)
        return false;
    ArrayType *CTI = this->m_compoundTypeInfo;
    if (CTI->m_elementTypeID == element_mixed || CTI->m_elementTypeID == element_null || CTI->m_elementTypeID == element_identity)
        return false;
    return CTI->m_nDim == 0;
}

bool typeIsLValueType(Type *this) {
    switch (this->m_typeId) {
        case typeid_ndarray:
            return arrayTypeCanBeLValue(this->m_compoundTypeInfo);
        case typeid_interval:
        case typeid_string:
        case typeid_tuple:
            return true;
        default: break;
    }
    return false;
}

bool typeIsVectorOrString(Type *this) {
    TypeID id = this->m_typeId;
    if (id != typeid_ndarray && id != typeid_string)
        return false;
    ArrayType *CTI = this->m_compoundTypeInfo;
    return CTI->m_nDim == 1;
}

bool typeIsMatrix(Type *this) {
    TypeID id = this->m_typeId;
    if (id != typeid_ndarray)
        return false;
    ArrayType *CTI = this->m_compoundTypeInfo;
    return CTI->m_nDim == 2;
}

void tupleTypeInitFromCopy(TupleType *this, TupleType *other) {
    int64_t n = other->m_nField;
    this->m_nField = n;
    this->m_strIdToField = malloc(2 * n * sizeof(int64_t));
    this->m_fieldTypeArr = malloc(n * sizeof(Type *));
    memcpy(this->m_strIdToField, other->m_strIdToField, 2 * n * sizeof(int64_t));
    memcpy(this->m_fieldTypeArr, other->m_fieldTypeArr, n * sizeof(Type *));
}

void *tupleTypeMallocDataFromNull(TupleType *this) {
    // recursively initialize every field as null
    int64_t n = this->m_nField;
    Variable **vars = malloc(n * sizeof(Variable *));
    for (int64_t i = 0; i < n; i++) {
        Type *fieldType = &(this->m_fieldTypeArr[i]);
        vars[i] = variableMalloc();
        variableInitFromNull(vars[i], fieldType);
        vars[i]->m_parent = vars;
        vars[i]->m_fieldPos = i;
    }
    return vars;
}

void *tupleTypeMallocDataFromIdentity(TupleType *this) {
    int64_t n = this->m_nField;
    Variable **vars = malloc(n * sizeof(Variable *));
    for (int64_t i = 0; i < n; i++) {
        Type *fieldType = &(this->m_fieldTypeArr[i]);
        vars[i] = variableMalloc();
        variableInitFromIdentity(vars[i], fieldType);
        vars[i]->m_parent = vars;
        vars[i]->m_fieldPos = i;
    }
    return vars;
}

void *tupleTypeMallocDataFromCopy(TupleType *this, void *otherTupleData) {
    int64_t n = this->m_nField;
    Variable **vars = malloc(n * sizeof(Variable *));
    Variable **otherVars = otherTupleData;
    for (int64_t i = 0; i < n; i++) {
        vars[i] = variableMalloc();
        variableInitFromMemcpy(vars[i], otherVars[i]);
        vars[i]->m_parent = vars;
        vars[i]->m_fieldPos = i;
    }
    return vars;
}

void *tupleTypeMallocDataFromPCADP(TupleType *this, Variable *src, PCADPConfig *config) {
    int64_t n = this->m_nField;
    Variable **vars = malloc(n * sizeof(Variable *));
    Variable **otherVars = src->m_data;
    for (int64_t i = 0; i < n; i++) {
        vars[i] = variableMalloc();
        variableInitFromPCADP(vars[i], &(this->m_fieldTypeArr[i]), otherVars[i], config);
        vars[i]->m_parent = vars;
        vars[i]->m_fieldPos = i;
    }
    return vars;
}

void tupleTypeFreeData(TupleType *this, void *data) {
    Variable **vars = data;
    for (int64_t i = 0; i < this->m_nField; i++) {
        variableDestructor(vars[i]);
        free(vars[i]);
    }
    free(vars);
}

VecToVecRHSSizeRestriction arrayTypeMinimumConpatibleRestriction(ArrayType *this, ArrayType *target) {
    VecToVecRHSSizeRestriction compatibleRestriction = vectovec_rhs_must_be_same_size;

    int8_t nDim = target->m_nDim;
    int64_t *dims = this->m_dims;
    int64_t *targetDims = target->m_dims;

    int64_t i = 0;
    while (i < nDim) {
        if (targetDims[i] != dims[i]) {
            compatibleRestriction = vectovec_rhs_size_must_not_be_greater;
            break;
        }
        i++;
    }
    while (i < nDim) {
        if (targetDims[i] < dims[i]) {
            compatibleRestriction = vectovec_rhs_size_can_be_any;
            break;
        }
        i++;
    }
    return compatibleRestriction;
}