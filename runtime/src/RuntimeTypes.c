#include <stddef.h>
#include "RuntimeTypes.h"
#include <stdlib.h>
#include <string.h>
#include "RuntimeErrors.h"
#include "RuntimeVariables.h"
#include "NDArray.h"

///------------------------------TYPE AND VARIABLE---------------------------------------------------------------

Type *typeMalloc() {
    return malloc(sizeof(Type));
}

void typeInitFromCopy(Type *this, Type *other) {
    TypeID otherID = other->m_typeId;
    this->m_typeId = otherID;
    switch (other->m_typeId) {
        case TYPEID_NDARRAY:
        case TYPEID_STRING:
            this->m_compoundTypeInfo = arrayTypeMalloc();
            arrayTypeInitFromCopy(this->m_compoundTypeInfo, other->m_compoundTypeInfo);
            break;
        case TYPEID_INTERVAL:
            this->m_compoundTypeInfo = intervalTypeMalloc();
            intervalTypeInitFromCopy(this->m_compoundTypeInfo, other->m_compoundTypeInfo);
            break;
        case TYPEID_STREAM_IN:
        case TYPEID_STREAM_OUT:
        case TYPEID_EMPTY_ARRAY:
        case TYPEID_UNKNOWN:
            this->m_compoundTypeInfo = NULL;
            break;
        case TYPEID_TUPLE:
            this->m_compoundTypeInfo = tupleTypeMalloc();
            tupleTypeInitFromCopy(this->m_compoundTypeInfo, other->m_compoundTypeInfo);
            break;
        default:
            targetTypeError(other, "Failed to copy from type: "); break;
    }
}

void typeInitFromTwoSingleTerms(Type *this, Type *first, Type *second) {
    if (!typeIsScalarInteger(first))
        targetTypeError(first, "attempt to init from two single terms where the first term is:");
    if (second->m_typeId != TYPEID_INTERVAL) {
        targetTypeError(second, "attempt to init from two single terms where the second term is:");
    } else {
        IntervalType *CTI = second->m_compoundTypeInfo;
        if (CTI->m_baseTypeID != UNSPECIFIED_BASE_INTERVAL) {
            targetTypeError(second, "attempt to init from two single terms where the second term is:");
        }
    }
    typeInitFromIntervalType(this, INTEGER_BASE_INTERVAL);
}

void typeInitFromVectorSizeSpecificationFromLiteral(Type *this, int64_t size, Type *baseType) {
    ArrayType *baseTypeCTI = (ArrayType *) baseType->m_compoundTypeInfo;
    this->m_typeId = baseType->m_typeId;  // whether baseType is scalar ndarray or string, the result of specification has the same type as baseType
    if (typeIsScalarBasic(baseType)) {  // do nothing
    } else if (baseType->m_typeId == TYPEID_STRING) {
        if (baseTypeCTI->m_dims[0] != SIZE_UNSPECIFIED)
            targetTypeError(baseType, "Attempt to specify size for a string that has already specified type! ");
    } else {
        targetTypeError(baseType, "Invalid base type: ");
    }
    ArrayType *CTI = arrayTypeMalloc();
    arrayTypeInitFromVectorSize(CTI, baseTypeCTI->m_elementTypeID, size);
    this->m_compoundTypeInfo = (void *) CTI;
}

void typeInitFromMatrixSizeSpecificationFromLiteral(Type *this, int64_t nRow, int64_t nCol, Type *baseType) {
    ArrayType *baseTypeCTI = (ArrayType *) baseType->m_compoundTypeInfo;
    this->m_typeId = baseType->m_typeId;
    if (typeIsScalarBasic(baseType)) {  // do nothing
    } else {
        targetTypeError(baseType, "Invalid base type: ");
    }
    ArrayType *CTI = arrayTypeMalloc();
    arrayTypeInitFromMatrixSize(CTI, baseTypeCTI->m_elementTypeID, nRow, nCol);
    this->m_compoundTypeInfo = (void *) CTI;
}

void typeInitFromIntervalType(Type *this, IntervalTypeBaseTypeID id) {
    this->m_compoundTypeInfo = intervalTypeMalloc();
    intervalTypeInitFromBase(this->m_compoundTypeInfo, id);
    this->m_typeId = TYPEID_INTERVAL;
}

void typeInitFromArrayType(Type *this, TypeID typeID, ElementTypeID eid, int8_t nDim, int64_t *dims) {
    this->m_typeId = TYPEID_NDARRAY;
    this->m_compoundTypeInfo = arrayTypeMalloc();
    arrayTypeInitFromDims(this->m_compoundTypeInfo, eid, nDim, dims);
}

void typeDestructor(Type *this) {
    switch (this->m_typeId) {
        case TYPEID_NDARRAY:
        case TYPEID_STRING:
            arrayTypeDestructor(this->m_compoundTypeInfo);
            free(this->m_compoundTypeInfo);
            break;
        case TYPEID_TUPLE: {
            TupleType *tupleType = this->m_compoundTypeInfo;
            tupleTypeDestructor(tupleType);
            free(tupleType);
        }    break;  // m_data is ignored for these types
        case TYPEID_INTERVAL: {
            free(this->m_compoundTypeInfo);
        }
        case TYPEID_STREAM_IN:
        case TYPEID_STREAM_OUT:
        case TYPEID_EMPTY_ARRAY:
        case TYPEID_UNKNOWN:
            break;
        case NUM_TYPE_IDS:
        default:
            unknownTypeVariableError();
            break;
    }
}

void typeDestructThenFree(Type *this) {
    typeDestructor(this);
    free(this);
}

// Type Methods

bool typeIsSpecifiable(Type *this) {
    if (this->m_typeId == TYPEID_STRING) {
        ArrayType *CTI = this->m_compoundTypeInfo;
        if (CTI->m_dims[0] == -2)
            return true;
    }
    return typeIsScalarBasic(this);
}

// if the type does not have any unknown/unspecified part i.e. a variable can have this type
bool typeIsConcreteType(Type *this) {
    switch (this->m_typeId) {

        case TYPEID_NDARRAY:
        case TYPEID_STRING:
            return !arrayTypeHasUnknownSize(this->m_compoundTypeInfo);
        case TYPEID_INTERVAL:
            return !intervalTypeIsUnspecified(this->m_compoundTypeInfo);
        case TYPEID_TUPLE:
        case TYPEID_STREAM_IN:
        case TYPEID_STREAM_OUT:
        case TYPEID_EMPTY_ARRAY:
            return true;
        case TYPEID_UNKNOWN:
        default:
            return false;
    }
    errorAndExit("This should not happen!");
}

bool typeIsStream(Type *this) {
    return this->m_typeId == TYPEID_STREAM_IN || this->m_typeId == TYPEID_STREAM_OUT;
}

bool typeIsScalar(Type *this) {
    TypeID id = this->m_typeId;
    if (id != TYPEID_NDARRAY)
        return false;
    ArrayType *CTI = this->m_compoundTypeInfo;
    return CTI->m_nDim == 0;
}

bool typeIsScalarBasic(Type *this) {
    TypeID id = this->m_typeId;
    if (id != TYPEID_NDARRAY)
        return false;
    ArrayType *CTI = this->m_compoundTypeInfo;
    if (CTI->m_elementTypeID == ELEMENT_MIXED || CTI->m_elementTypeID == ELEMENT_NULL || CTI->m_elementTypeID == ELEMENT_IDENTITY)
        return false;
    return CTI->m_nDim == 0;
}

bool typeIsScalarNull(Type *this) {
    if (this->m_typeId == TYPEID_NDARRAY) {
        ArrayType *CTI = this->m_compoundTypeInfo;
        if (CTI->m_elementTypeID == ELEMENT_NULL && CTI->m_nDim == 0)
            return true;
    }
    return false;
}

bool typeIsScalarIdentity(Type *this) {
    if (this->m_typeId == TYPEID_NDARRAY) {
        ArrayType *CTI = this->m_compoundTypeInfo;
        if (CTI->m_elementTypeID == ELEMENT_IDENTITY && CTI->m_nDim == 0)
            return true;
    }
    return false;
}

bool typeIsScalarInteger(Type *this) {
    if (this->m_typeId == TYPEID_NDARRAY) {
        ArrayType *CTI = this->m_compoundTypeInfo;
        if (CTI->m_elementTypeID == ELEMENT_INTEGER && CTI->m_nDim == 0)
            return true;
    }
    return false;
}

bool typeIsArrayNull(Type *this) {
    if (this->m_typeId == TYPEID_NDARRAY) {
        ArrayType *CTI = this->m_compoundTypeInfo;
        if (CTI->m_elementTypeID == ELEMENT_NULL)
            return true;
    }
    return false;
}

bool typeIsArrayIdentity(Type *this) {
    if (this->m_typeId == TYPEID_NDARRAY) {
        ArrayType *CTI = this->m_compoundTypeInfo;
        if (CTI->m_elementTypeID == ELEMENT_IDENTITY)
            return true;
    }
    return false;
}

bool typeIsUnknown(Type *this) {
    return this->m_typeId == TYPEID_UNKNOWN;
}

bool typeIsArrayOrString(Type *this) {
    return this->m_typeId == TYPEID_NDARRAY || this->m_typeId == TYPEID_STRING;
}

bool typeIsVectorOrString(Type *this) {
    if (!typeIsArrayOrString(this))
        return false;
    ArrayType *CTI = this->m_compoundTypeInfo;
    return CTI->m_nDim == 1;
}

bool typeIsMatrix(Type *this) {
    TypeID id = this->m_typeId;
    if (id != TYPEID_NDARRAY)
        return false;
    ArrayType *CTI = this->m_compoundTypeInfo;
    return CTI->m_nDim == 2;
}

bool typeIsMixedArray(Type *this) {
    if (this->m_typeId == TYPEID_NDARRAY) {
        ArrayType *CTI = this->m_compoundTypeInfo;
        return CTI->m_elementTypeID == ELEMENT_MIXED;
    }
    return false;
}

bool typeIsIdentical(Type *this, Type *other) {
    // TODO: implement this (if this is ever needed)
}

///------------------------------COMPOUND TYPE INFO---------------------------------------------------------------

ArrayType *arrayTypeMalloc() {
    return malloc(sizeof(ArrayType));
}

void arrayTypeInitFromVectorSize(ArrayType *this, ElementTypeID elementTypeID, int64_t vecLength) {
    int64_t dims[1] = {vecLength};
    arrayTypeInitFromDims(this, elementTypeID, 1, dims);
}

void arrayTypeInitFromMatrixSize(ArrayType *this, ElementTypeID elementTypeID, int64_t dim1, int64_t dim2) {
    int64_t dims[2] = {dim1, dim2};
    arrayTypeInitFromDims(this, elementTypeID, 2, dims);
}

void arrayTypeInitFromCopy(ArrayType *this, ArrayType *other) {
    arrayTypeInitFromDims(this, other->m_elementTypeID, other->m_nDim, other->m_dims);
}

void arrayTypeInitFromDims(ArrayType *this, ElementTypeID elementTypeID, int8_t nDim, int64_t *dims) {
    this->m_elementTypeID = elementTypeID;
    this->m_nDim = nDim;
    if (nDim != 0) {
        this->m_dims = malloc(nDim * sizeof(int64_t));
        memcpy(this->m_dims, dims, nDim * sizeof(int64_t));
    } else
        this->m_dims = NULL;
}

void arrayTypeDestructor(ArrayType *this) {
    free(this->m_dims);
}

// Array type methods

VecToVecRHSSizeRestriction arrayTypeMinimumCompatibleRestriction(ArrayType *this, ArrayType *target) {
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

bool arrayTypeHasUnknownSize(ArrayType *this) {
    bool hasUnknown = false;
    for (int8_t i = 0; i < this->m_nDim; i++) {
        hasUnknown = hasUnknown || this->m_dims[i] < 0;
    }
}

int64_t arrayTypeElementSize(ArrayType *this) {
    return elementGetSize(this->m_elementTypeID);
}
int64_t arrayTypeGetTotalLength(ArrayType *this) {
    int8_t nDim = this->m_nDim;
    if (nDim == 0) {
        return 1;
    } else if (nDim == 1) {
        return this->m_dims[0];
    } else if (nDim == 2) {
        return this->m_dims[0] * this->m_dims[1];
    }
}

bool arrayTypeCanBinopSameTypeSameSize(ArrayType *result, ArrayType *op1, ArrayType *op2) {
    ElementTypeID eid;
    if (!elementCanBePromotedBetween(op1->m_elementTypeID, op2->m_elementTypeID, &eid))
        return false;
    if (op1->m_nDim == 0 && op2->m_nDim != 0) {
        arrayTypeInitFromCopy(result, op2);
    } else if (op2->m_nDim == 0 && op1->m_nDim != 0) {
        arrayTypeInitFromCopy(result, op1);
    } else if (op1->m_nDim == 0 && op2->m_nDim == 0) {
        arrayTypeInitFromCopy(result, op1);  // either works, since the two only differs in eid
    } else if (op1->m_nDim != op2->m_nDim) {  // one vector one matrix
        return false;
    } else {  // two vectors or two matrices
        for (int8_t i = 0; i < op1->m_nDim; i++) {
            if (op1->m_dims[i] != op2->m_dims[i]) {
                return false;
            }
        }
        arrayTypeInitFromCopy(result, op1);
    }
    result->m_elementTypeID = eid;
    return true;
}

// IntervalType---------------------------------------------------------------------------------------------

IntervalType *intervalTypeMalloc() {
    return malloc(sizeof(IntervalType));
}
void *intervalTypeInitFromBase(IntervalType *this, IntervalTypeBaseTypeID id) {
    this->m_baseTypeID = id;
}
void *intervalTypeInitFromCopy(IntervalType *this, IntervalType *other) {
    this->m_baseTypeID = other->m_baseTypeID;
}

void *intervalTypeMallocDataFromNull() {
    int32_t *interval = malloc(sizeof(int32_t) * 2);
    interval[0] = 0;
    interval[1] = 0;
    return interval;
}

void *intervalTypeMallocDataFromIdentity() {
    int32_t *interval = malloc(sizeof(int32_t) * 2);
    interval[0] = 1;
    interval[1] = 1;
    return interval;
}

void *intervalTypeMallocDataFromHeadTail(int32_t head, int32_t tail) {
    int32_t *interval = malloc(sizeof(int32_t) * 2);
    if (head > tail) {
        errorAndExit("Interval head is greater than tail!");
    }
    interval[0] = head;
    interval[1] = tail;
    return interval;
}

void *intervalTypeMallocDataFromCopy(void *otherIntervalData) {
    int32_t *interval = malloc(sizeof(int32_t) * 2);
    memcpy(interval, otherIntervalData, sizeof(int32_t) * 2);
    return interval;
}

void intervalTypeFreeData(void *data) {
    free(data);
}

bool intervalTypeIsUnspecified(IntervalType *this) {
    return this->m_baseTypeID == UNSPECIFIED_BASE_INTERVAL;
}

void intervalTypeUnaryPlus(int32_t *result, const int32_t *op) {
    result[0] = op[0];
    result[1] = op[1];
}
void intervalTypeUnaryMinus(int32_t *result, const int32_t *op) {
    int32_t a = -op[1];
    int32_t b = -op[0];
    result[0] = a;
    result[1] = b;
}
void intervalTypeBinaryPlus(int32_t *result, const int32_t *op1, const int32_t *op2) {
    int32_t a = op1[0] + op2[0];
    int32_t b = op1[1] + op2[1];
    result[0] = a;
    result[1] = b;
}
void intervalTypeBinaryMinus(int32_t *result, const int32_t *op1, const int32_t *op2) {
    int32_t a = op1[0] - op2[1];
    int32_t b = op1[1] - op2[0];
    result[0] = a;
    result[1] = b;
}
void intervalTypeBinaryMultiply(int32_t *result, const int32_t *op1, const int32_t *op2) {
    // max/min must be obtained at endpoints
    int32_t a = op1[0] * op2[0];
    int32_t b = op1[0] * op2[0];
    for (int64_t i = 0; i < 2; i++) {
        for (int64_t j = 0; j < 2; j++) {
            int32_t product = op1[i] * op2[j];
            a = a < product ? a : product;
            b = b > product ? b : product;
        }
    }
    result[0] = a;
    result[1] = b;
}
void intervalTypeBinaryEq(bool *result, const int32_t *op1, const int32_t *op2) {
    *result = op1[0] == op2[0] && op1[1] == op2[1];
}
void intervalTypeBinaryNe(bool *result, const int32_t *op1, const int32_t *op2) {
    *result = !(op1[0] == op2[0] && op1[1] == op2[1]);
}

// TupleType---------------------------------------------------------------------------------------------

TupleType *tupleTypeMalloc() {
    return malloc(sizeof(TupleType));
}

void tupleTypeInitFromTypeAndId(TupleType *this, int64_t nField, Type **typeArray, int64_t *stridArray) {
    this->m_nField = nField;
    this->m_idxToStrid = malloc(nField * sizeof(int64_t));
    this->m_fieldTypeArr = malloc(nField * sizeof(Type));
    if (stridArray != NULL) {
        memcpy(this->m_idxToStrid, stridArray, nField * sizeof(int64_t));
    } else {
        for (int64_t i = 0; i < nField; i++)
            this->m_idxToStrid[i] = -1;
    }
    for (int64_t i = 0; i < nField; i++) {
        typeInitFromCopy(&(this->m_fieldTypeArr[i]), typeArray[i]);
    }
}

void tupleTypeInitFromCopy(TupleType *this, TupleType *other) {
    Type *(types[other->m_nField]);
    for (int64_t i = 0; i < other->m_nField; i++) {
        types[i] = &(other->m_fieldTypeArr[i]);
    }
    tupleTypeInitFromTypeAndId(this, other->m_nField, types, other->m_idxToStrid);
}

void tupleTypeDestructor(TupleType *this) {
    for (int64_t i = 0; i < this->m_nField; i++) {
        typeDestructor(&this->m_fieldTypeArr[i]);
    }
    free(this->m_fieldTypeArr);
    free(this->m_idxToStrid);
}

int64_t tupleTypeResolveId(TupleType *this, int64_t id) {
    for (int64_t i = 0; i < this->m_nField; i++) {
        if (id == this->m_idxToStrid[i])
            return i + 1;
    }
    return -1;  // the id is not found
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
    return tupleTypeMallocDataFromCopyVariableArray(this, (Variable **)otherTupleData);
}

void *tupleTypeMallocDataFromCopyVariableArray(TupleType *this, Variable **otherVars) {
    int64_t n = this->m_nField;
    Variable **vars = malloc(n * sizeof(Variable *));
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
        variableDestructThenFree(vars[i]);
    }
    free(vars);
}