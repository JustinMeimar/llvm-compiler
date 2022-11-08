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
    if (!typeIsScalarBasic(other) || otherID == typeid_interval) {
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
    if (typeIsScalarBasic(baseType)) {  // do nothing
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
    if (typeIsScalarBasic(baseType)) {  // do nothing
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

void typeInitFromArrayType(Type *this, ElementTypeID eid, int8_t nDim, int64_t *dims) {
    this->m_typeId = typeid_ndarray;
    this->m_compoundTypeInfo = arrayTypeMalloc();
    arrayTypeInitFromDims(this->m_compoundTypeInfo, eid, nDim, dims);
}

//void typeInitBinOpResultAndPromotionTypes(Variable *op1, Variable *op2, BinOpCode opcode, Type **result, Type **promoteOp1, Type **promoteOp2) {
//    *promoteOp1 = NULL;  // a null promote op indicates the promotion is unnecessary
//    *promoteOp2 = NULL;
//    *result = NULL;  // a null result indicates the binop is impossible
//    Type *op1Type = op1->m_type;
//    Type *op2Type = op2->m_type;
//    switch (opcode) {
//        case binary_index:
//            break;
//        case binary_range_construct:
//            break;
//        case binary_exponent:
//        case binary_multiply:
//        case binary_divide:
//        case binary_remainder:
//        case binary_dot_product:
//        case binary_plus:
//        case binary_minus:
//        case binary_lt:
//        case binary_bt:
//        case binary_leq:
//        case binary_beq:
//        case binary_eq:
//        case binary_ne:
//        case binary_and:
//        case binary_or:
//        case binary_xor: {
//
//        } break;
//        case binary_by: {
//
//        } break;
//        default:
//            errorAndExit("unexpected op!"); break;
//    }
//
//}

void typeDestructor(Type *this) {
    switch (this->m_typeId) {
        case typeid_ndarray:
        case typeid_string:
            arrayTypeDestructor(this->m_compoundTypeInfo);
            free(this->m_compoundTypeInfo);
        case typeid_stream_in:
        case typeid_stream_out:
        case typeid_empty_array:
            break;
        case typeid_tuple: {
            TupleType *tupleType = this->m_compoundTypeInfo;
            tupleTypeDestructor(tupleType);
            free(tupleType);
        }    break;  // m_data is ignored for these types
        case typeid_interval:
            break;
        case typeid_unknown:
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

bool typeIsStream(Type *this) {
    return this->m_typeId == typeid_stream_in || this->m_typeId == typeid_stream_out;
}

bool typeIsScalar(Type *this) {
    TypeID id = this->m_typeId;
    if (id != typeid_ndarray)
        return false;
    ArrayType *CTI = this->m_compoundTypeInfo;
    return CTI->m_nDim == 0;
}

bool typeIsScalarBasic(Type *this) {
    TypeID id = this->m_typeId;
    if (id != typeid_ndarray)
        return false;
    ArrayType *CTI = this->m_compoundTypeInfo;
    if (CTI->m_elementTypeID == element_mixed || CTI->m_elementTypeID == element_null || CTI->m_elementTypeID == element_identity)
        return false;
    return CTI->m_nDim == 0;
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

bool typeIsArrayOrString(Type *this) {
    return this->m_typeId == typeid_ndarray || this->m_typeId == typeid_string;
}

bool typeIsVectorOrString(Type *this) {
    if (!typeIsArrayOrString(this))
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

bool typeIsMixedArray(Type *this) {
    if (this->m_typeId == typeid_ndarray) {
        ArrayType *CTI = this->m_compoundTypeInfo;
        return CTI->m_elementTypeID == element_mixed;
    }
    return false;
}

///------------------------------COMPOUND TYPE INFO---------------------------------------------------------------

void arrayTypeInitFromDims(ArrayType *this, ElementTypeID elementTypeID, int8_t nDim, int64_t *dims) {
    this->m_elementTypeID = elementTypeID;
    this->m_nDim = nDim;
    if (nDim != 0) {
        this->m_dims = malloc(nDim * sizeof(int64_t));
        memcpy(this->m_dims, dims, nDim * sizeof(int64_t));
    } else
        this->m_dims = NULL;
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

void *intervalTypeMallocDataFromNull() {
    int *interval = malloc(sizeof(int32_t) * 2);
    interval[0] = 0;
    interval[1] = 0;
    return interval;
}

void *intervalTypeMallocDataFromIdentity() {
    int *interval = malloc(sizeof(int32_t) * 2);
    interval[0] = 1;
    interval[1] = 1;
    return interval;
}

void *intervalTypeMallocDataFromHeadTail(int32_t head, int32_t tail) {
    int *interval = malloc(sizeof(int32_t) * 2);
    if (head > tail) {
        errorAndExit("Interval head is greater than tail!");
    }
    interval[0] = head;
    interval[1] = tail;
    return interval;
}

void *intervalTypeMallocDataFromCopy(void *otherIntervalData) {
    int *interval = malloc(sizeof(int32_t) * 2);
    memcpy(interval, otherIntervalData, sizeof(int32_t) * 2);
    return interval;
}

void intervalTypeFreeData(void *data) {
    free(data);
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

void tupleTypeInitFromCopy(TupleType *this, TupleType *other) {
    int64_t n = other->m_nField;
    this->m_nField = n;
    this->m_idToIndex = malloc(2 * n * sizeof(int64_t));
    this->m_fieldTypeArr = malloc(n * sizeof(Type *));
    memcpy(this->m_idToIndex, other->m_idToIndex, 2 * n * sizeof(int64_t));
    for (int64_t i = 0; i < n; i++) {
        typeInitFromCopy(&(this->m_fieldTypeArr[i]), &(other->m_fieldTypeArr[i]));
    }
}

void tupleTypeDestructor(TupleType *this) {
    for (int64_t i = 0; i < this->m_nField; i++) {
        typeDestructor(&this->m_fieldTypeArr[i]);
    }
    free(this->m_fieldTypeArr);
    free(this->m_idToIndex);
}

int64_t tupleTypeResolveId(TupleType *this, int64_t id) {
    for (int64_t i = 0; i < this->m_nField; i++) {
        if (id == this->m_idToIndex[i * 2])
            return this->m_idToIndex[i * 2 + 1];
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
        variableDestructThenFree(vars[i]);
    }
    free(vars);
}