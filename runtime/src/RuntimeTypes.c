#include <stddef.h>
#include "RuntimeTypes.h"
#include <stdlib.h>
#include "RuntimeErrors.h"

///------------------------------TYPE AND VARIABLE---------------------------------------------------------------

Type *typeMalloc() {
    return malloc(sizeof(Type));
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