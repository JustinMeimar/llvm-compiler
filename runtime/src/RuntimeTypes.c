#include <stddef.h>
#include "RuntimeTypes.h"
#include <stdlib.h>
#include <string.h>
#include "RuntimeErrors.h"
#include "RuntimeVariables.h"
#include "NDArray.h"
#include "VariableStdio.h"
#include "NDArrayVariable.h"

///------------------------------TYPE AND VARIABLE---------------------------------------------------------------

Type *typeMalloc() {
    Type *type = malloc(sizeof(Type));
#ifdef DEBUG_PRINT
    if (!reentry)
        fprintf(stderr, "(malloc type %p)\n", (void *)type);
#endif
    return type;
}

void typeInitFromCopy(Type *this, Type *other) {
    TypeID otherID = other->m_typeId;
    this->m_typeId = otherID;
    switch (other->m_typeId) {
        case TYPEID_NDARRAY:
            this->m_compoundTypeInfo = arrayTypeMalloc();
            arrayTypeInitFromCopy(this->m_compoundTypeInfo, other->m_compoundTypeInfo);
            break;
        case TYPEID_INTERVAL:
            this->m_compoundTypeInfo = intervalTypeMalloc();
            intervalTypeInitFromCopy(this->m_compoundTypeInfo, other->m_compoundTypeInfo);
            break;
        case TYPEID_STREAM_IN:
        case TYPEID_STREAM_OUT:
        case TYPEID_UNKNOWN:
            this->m_compoundTypeInfo = NULL;
            break;
        case TYPEID_TUPLE:
            this->m_compoundTypeInfo = tupleTypeMalloc();
            tupleTypeInitFromCopy(this->m_compoundTypeInfo, other->m_compoundTypeInfo);
            break;
        default:
            singleTypeError(other, "Failed to copy from type: "); break;
    }
}

void typeInitFromTwoSingleTerms(Type *this, Type *first, Type *second) {
    if (!typeIsScalarInteger(first))
        singleTypeError(first, "attempt to init from two single terms where the first term is:");
    if (second->m_typeId != TYPEID_INTERVAL) {
        singleTypeError(second, "attempt to init from two single terms where the second term is:");
    } else {
        IntervalType *CTI = second->m_compoundTypeInfo;
        if (CTI->m_baseTypeID != UNSPECIFIED_BASE_INTERVAL) {
            singleTypeError(second, "attempt to init from two single terms where the second term is:");
        }
    }
    typeInitFromIntervalType(this, INTEGER_BASE_INTERVAL);
}

void typeInitFromIntervalType(Type *this, IntervalTypeBaseTypeID id) {
    this->m_compoundTypeInfo = intervalTypeMalloc();
    intervalTypeInitFromBase(this->m_compoundTypeInfo, id);
    this->m_typeId = TYPEID_INTERVAL;
}

void typeDestructor(Type *this) {
#ifdef DEBUG_PRINT
    if (!reentry) {
        fprintf(stderr, "(destruct type %p)", (void *) this);
        typeDebugPrint(this);
        fprintf(stderr, "\n");
    }
#endif
    switch (this->m_typeId) {
        case TYPEID_NDARRAY:
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
        case TYPEID_UNKNOWN:
            break;
        case NUM_TYPE_IDS:
        default:
            singleTypeError(this, "Unexpected type on typeDestructor() call: ");
            break;
    }
}

void typeDestructThenFree(Type *this) {
    typeDestructor(this);
#ifdef DEBUG_PRINT
    if (!reentry)
        fprintf(stderr, "(free type %p)\n", (void *)this);
#endif
    free(this);
}

// Type Methods
bool typeIsVariableClassCompatible(Type *this) {
    switch (this->m_typeId) {

        case TYPEID_NDARRAY:
            return !arrayTypeHasUnknownSize(this->m_compoundTypeInfo);
        case TYPEID_INTERVAL:
            return !intervalTypeIsUnspecified(this->m_compoundTypeInfo);
        case TYPEID_TUPLE:
        case TYPEID_STREAM_IN:
        case TYPEID_STREAM_OUT:
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

bool typeIsUnknown(Type *this) {
    return this->m_typeId == TYPEID_UNKNOWN;
}

bool typeIsIntegerInterval(Type *this) {
    if (this->m_typeId != TYPEID_INTERVAL)
        return false;
    IntervalType *CTI = this->m_compoundTypeInfo;
    return CTI->m_baseTypeID == INTEGER_BASE_INTERVAL;
}

bool typeIsUnspecifiedInterval(Type *this) {
    if (this->m_typeId == TYPEID_INTERVAL) {
        return intervalTypeIsUnspecified(this->m_compoundTypeInfo);
    }
    return false;
}

bool typeIsIdentical(Type *this, Type *other) {
    // TODO: implement this (if this is ever needed)
}

bool typeIsDomainExprCompatible(Type *this) {
    return typeIsIntegerVector(this) || typeIsIntegerInterval(this) || typeIsEmptyArray(this);
}

///------------------------------COMPOUND TYPE INFO---------------------------------------------------------------
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

int32_t intervalTypeGetElementAtIndex(const int32_t *ivl, int64_t idx) {
    int32_t offset = (int32_t)idx;
    if (offset < 0 || offset > ivl[1] - ivl[0])
        errorAndExit("Index out of range for integer interval!");
    return ivl[0] + offset;
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
        variableAttrInitHelper(vars[i], i, vars, false);
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
        variableAttrInitHelper(vars[i], i, vars, false);
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
        variableAttrInitHelper(vars[i], i, vars, false);
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
        variableAttrInitHelper(vars[i], i, vars, config->m_resultIsBlockScoped);
    }
    return vars;
}

void tupleTypeFreeData(TupleType *this, void *data) {
    Variable **vars = data;
    for (int64_t i = 0; i < this->m_nField; i++) {
#ifdef DEBUG_PRINT
        fprintf(stderr, "daf#13\n");
#endif
        variableDestructThenFreeImpl(vars[i]);
    }
    free(vars);
}

// Some Utility for LLVM passing parameters---------------------------------------------------------------------------------------------

void *variableArrayMalloc(int64_t size) { return malloc(size * sizeof(Variable *)); }
void variableArraySet(Variable **arr, int64_t idx, Variable *var) { arr[idx] = var; }
void variableArrayFree(Variable **arr) { free(arr); }
void freeArrayContents(Variable **arr, int64_t size) {
    for (int64_t i = 0; i< size; i++) {
        variableDestructThenFree(arr[i]);
    }
}

void *typeArrayMalloc(int64_t size) { return malloc(size * sizeof(Type *)); }
void typeArraySet(Type **arr, int64_t idx, Type *type) { arr[idx] = type; }
void typeArrayFree(Type **arr) { free(arr); }

void *stridArrayMalloc(int64_t size) { return malloc(size * sizeof(int64_t)); }
void stridArraySet(int64_t *arr, int64_t idx, int64_t val) { arr[idx] = val; }
void stridArrayFree(int64_t *arr) { free(arr); }

void *acceptMatrixMalloc(int64_t nFilter, int64_t domainSize) { return malloc(nFilter * domainSize * sizeof(bool)); }
void acceptArraySet(bool *accept, int64_t domainSize, int64_t filterIdx, int64_t domainIdx, bool val) {
    accept[domainSize * filterIdx + domainIdx] = val;
}
void acceptMatrixFree(bool *accept) { free(accept); }