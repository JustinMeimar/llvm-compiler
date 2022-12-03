#include "NDArray.h"
#include "RuntimeErrors.h"
#include "string.h"
#include "NDArrayVariable.h"
#include "Literal.h"
#include "FreeList.h"
#include "VariableStdio.h"

ArrayType *arrayTypeMalloc() {
    return malloc(sizeof(ArrayType));
}

void arrayTypeInitFromVectorSize(ArrayType *this, ElementTypeID elementTypeID, int64_t vecLength, bool isString) {
    int64_t dims[1] = {vecLength};
    arrayTypeInitFromDims(this, elementTypeID, 1, dims, isString, NULL, false, false);
}

void arrayTypeInitFromMatrixSize(ArrayType *this, ElementTypeID elementTypeID, int64_t dim1, int64_t dim2) {
    int64_t dims[2] = {dim1, dim2};
    arrayTypeInitFromDims(this, elementTypeID, 2, dims, false, NULL, false, false);
}

void arrayTypeInitFromCopy(ArrayType *this, ArrayType *other) {
    arrayTypeInitFromDims(this, other->m_elementTypeID, other->m_nDim, other->m_dims,
                          other->m_isString, NULL, other->m_isRef, other->m_isSelfRef);
}

void arrayTypeInitFromCopyByRef(ArrayType *this, ArrayType *other) {
    arrayTypeInitFromDims(this, other->m_elementTypeID, other->m_nDim, other->m_dims,
                          other->m_isString, other->m_refCount, other->m_isRef, other->m_isSelfRef);
}

void arrayTypeInitFromDims(ArrayType *this, ElementTypeID elementTypeID, int8_t nDim, int64_t *dims,
                           bool isString, int32_t *refCount, bool isRef, bool isSelfRef) {
    if (nDim == DIM_INVALID) {
        errorAndExit("Attempt to initialize an ndarray to nDim=DIM_INVALID!");
    }
    this->m_isString = isString;
    this->m_elementTypeID = elementTypeID;
    this->m_nDim = nDim;
    if (nDim != 0 && nDim != DIM_UNSPECIFIED) {
        this->m_dims = malloc(nDim * sizeof(int64_t));
        memcpy(this->m_dims, dims, nDim * sizeof(int64_t));
    } else
        this->m_dims = NULL;

    if (refCount != NULL)  {
        this->m_refCount = refCount;
        arrayTypeIncReferenceCount(this);
    } else {
        this->m_refCount = malloc(sizeof(int32_t));
        *this->m_refCount = 1;
    }
#ifdef DEBUG_PRINT
    fprintf(stderr, "rc:%p->%d\n", this->m_refCount, *this->m_refCount);
#endif

    this->m_isRef = isRef;
    this->m_isSelfRef = isSelfRef;
}

void arrayTypeDestructor(ArrayType *this) {
    if (arrayTypeGetReferenceCount(this) <= 1) {
        free(this->m_refCount);
    } else {
        arrayTypeDecReferenceCount(this);
    }
    free(this->m_dims);
}

// Array type methods

int32_t arrayTypeGetReferenceCount(ArrayType *this) {
    return *this->m_refCount;
}

void arrayTypeDecReferenceCount(ArrayType *this) {
    (*this->m_refCount)--;
}

void arrayTypeIncReferenceCount(ArrayType *this) {
    (*this->m_refCount)++;
}

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

// empty vector has size 0 thus is not unknown
bool arrayTypeHasUnknownSize(ArrayType *this) {
    bool hasUnknown = false;
    for (int8_t i = 0; i < this->m_nDim; i++) {
        hasUnknown = hasUnknown || this->m_dims[i] < 0;
    }
    return hasUnknown;
}

int64_t arrayTypeElementSize(ArrayType *this) {
    return elementGetSize(this->m_elementTypeID);
}
int64_t arrayTypeGetTotalLength(ArrayType *this) {
    int8_t nDim = this->m_nDim;
    if (nDim == DIM_UNSPECIFIED) {
        return 0;
    } else if (nDim == 0) {
        return 1;
    } else if (nDim == 1) {
        return this->m_dims[0];
    } else if (nDim == 2) {
        return this->m_dims[0] * this->m_dims[1];
    }
}

// Type

void typeInitFromVectorSizeSpecificationFromLiteral(Type *this, int64_t size, Type *baseType) {
    if (size < 0 && size != SIZE_UNKNOWN && size != SIZE_UNSPECIFIED) {
        errorAndExit("Invalid size in vector size specification!");
    }
    ArrayType *baseTypeCTI = (ArrayType *) baseType->m_compoundTypeInfo;
    this->m_typeId = baseType->m_typeId;
    if (typeIsScalarBasic(baseType)) {  // do nothing
    } else if (baseTypeCTI->m_isString) {
        if (baseTypeCTI->m_dims[0] != SIZE_UNSPECIFIED)
            singleTypeError(baseType, "Attempt to specify size for a string that has already specified type! ");
    } else {
        singleTypeError(baseType, "Invalid base type: ");
    }
    ArrayType *CTI = arrayTypeMalloc();
    arrayTypeInitFromVectorSize(CTI, baseTypeCTI->m_elementTypeID, size, baseTypeCTI->m_isString);
    this->m_compoundTypeInfo = (void *) CTI;
}

void typeInitFromMatrixSizeSpecificationFromLiteral(Type *this, int64_t nRow, int64_t nCol, Type *baseType) {
    if (nRow < 0 && nRow != SIZE_UNKNOWN) {  // matrix can't have unspecified size
        errorAndExit("Invalid nRow size in matrix size specification!");
    }
    if (nCol < 0 && nCol != SIZE_UNKNOWN) {  // matrix can't have unspecified size
        errorAndExit("Invalid nCol size in matrix size specification!");
    }
    ArrayType *baseTypeCTI = (ArrayType *) baseType->m_compoundTypeInfo;
    this->m_typeId = baseType->m_typeId;
    if (typeIsScalarBasic(baseType)) {  // do nothing
    } else {
        singleTypeError(baseType, "Invalid base type: ");
    }
    ArrayType *CTI = arrayTypeMalloc();
    arrayTypeInitFromMatrixSize(CTI, baseTypeCTI->m_elementTypeID, nRow, nCol);
    this->m_compoundTypeInfo = (void *) CTI;
}

void typeInitFromArrayType(Type *this, bool isString, ElementTypeID eid, int8_t nDim, int64_t *dims) {
    this->m_typeId = TYPEID_NDARRAY;
    this->m_compoundTypeInfo = arrayTypeMalloc();
    arrayTypeInitFromDims(this->m_compoundTypeInfo, eid, nDim, dims,
                          isString, NULL, false, false);
}

void typeInitFromNDArray(Type *this, ElementTypeID eid, int8_t nDim, int64_t *dims,
                         bool isString, int32_t *refCount, bool isRef, bool isSelfRef) {
    this->m_typeId = TYPEID_NDARRAY;
    this->m_compoundTypeInfo = arrayTypeMalloc();
    arrayTypeInitFromDims(this->m_compoundTypeInfo, eid, nDim, dims,
                          isString, refCount, isRef, isSelfRef);
}

// Methods

bool typeIsSpecifiable(Type *this) {
    TypeID id = this->m_typeId;
    if (id != TYPEID_NDARRAY)
        return false;
    ArrayType *CTI = this->m_compoundTypeInfo;
    if (CTI->m_elementTypeID == ELEMENT_MIXED || CTI->m_elementTypeID == ELEMENT_NULL || CTI->m_elementTypeID == ELEMENT_IDENTITY)
        return false;
    return CTI->m_nDim == 0 || (CTI->m_isString && CTI->m_dims[0] == -2);  // scalar basic types or string
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

bool typeIsArrayOrString(Type *this) {
    return this->m_typeId == TYPEID_NDARRAY;
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
    return typeGetNDArrayTypeID(this) == NDARRAY_MIXED;
}

bool typeIsEmptyArray(Type *this) {
    if (!typeIsArrayOrString(this))
        return false;
    ArrayType *CTI = this->m_compoundTypeInfo;
    return CTI->m_elementTypeID == ELEMENT_MIXED && CTI->m_nDim == DIM_UNSPECIFIED;
}

bool typeIsIntegerVector(Type *this) {
    int8_t nDim = typeGetNDArrayNDims(this);
    ArrayType *CTI = this->m_compoundTypeInfo;
    return nDim == 1 && CTI->m_elementTypeID == ELEMENT_INTEGER;
}

bool typeIsArraySameTypeSameSize(Type *this, Type *other) {
    ArrayType *CTI = this->m_compoundTypeInfo;
    ArrayType *otherCTI = other->m_compoundTypeInfo;
    bool isEqual = CTI->m_nDim == otherCTI->m_nDim &&
            CTI->m_elementTypeID == otherCTI->m_elementTypeID &&
            CTI->m_isString == otherCTI->m_isString;
    if (!isEqual)
        return false;
    for (int8_t i = 0; i < CTI->m_nDim; i++) {
        if (CTI->m_dims[i] != otherCTI->m_dims[i])
            isEqual = false;
    }
    return isEqual;
}

int8_t typeGetNDArrayNDims(Type *this) {
    if (this->m_typeId == TYPEID_NDARRAY) {
        ArrayType *CTI = this->m_compoundTypeInfo;
        return CTI->m_nDim;
    }
    return DIM_INVALID;
}

NDArrayTypeID typeGetNDArrayTypeID(Type *this) {
    if (this->m_typeId == TYPEID_NDARRAY) {
        ArrayType *CTI = this->m_compoundTypeInfo;
        if (CTI->m_isRef)
            return CTI->m_elementTypeID == ELEMENT_MIXED ? NDARRAY_INVALID : NDARRAY_REFERENCE;  // a reference can't have mixed types
        else
            return CTI->m_elementTypeID == ELEMENT_MIXED ? NDARRAY_MIXED : NDARRAY_CONCRETE;
    }
    return NDARRAY_NOT_AN_NDARRAY;
}

///------------------------------Variable---------------------------------------------------------------

void variableInitFromArrayIndexingHelper(Variable *this, Variable *arr, Variable *rowIndex, Variable *colIndex, int64_t nIndex) {
    Type *arrType = arr->m_type;
    Type *rowIndexType = rowIndex->m_type;
    Type *colIndexType = NULL;
    if (nIndex >= 2)
        colIndexType = colIndex->m_type;

    /// if arr is empty array, then only possible cases are [][[]] or [][[],[]]
    if (typeIsEmptyArray(arrType)) {
        if (!typeIsEmptyArray(rowIndexType) &&
            !(typeIsIntegerVector(rowIndexType) && variableGetLength(rowIndex) == 0)) {
            errorAndExit("Array index can only be integer array of size 0 or empty array!");
        }
        if (colIndex != NULL && !typeIsEmptyArray(colIndexType) &&
            !(typeIsIntegerVector(colIndexType) && variableGetLength(colIndex) == 0)) {
            errorAndExit("Array index can only be integer array of size 0 or empty array!");
        }
        variableInitFromEmptyArray(this);
        return;
    }

    int8_t arrNDim = variableGetNDim(arr);          // 1 or 2
    int8_t rowNDim = variableGetNDim(rowIndex);     // DIM_UNSPECIFIED, 0 or 1
    int8_t colNDim = 0;                                  // DIM_UNSPECIFIED, 0 or 1
    if (nIndex >= 2)
        colNDim = variableGetNDim(colIndex);

    /// dimension check
    if (arrNDim <= 0 || arrNDim != nIndex) {
        singleTypeError(arrType, "Incorrect array dimension for indexing: ");
    } else if (rowNDim == DIM_INVALID || rowNDim == 2 || colNDim == DIM_INVALID || colNDim == 2) {
        errorAndExit("Incorrect index dimensions!");
    }

    FreeList *freeList = NULL;
    Variable *pop1 = arr;
    Variable *pop2 = rowIndex;
    Variable *pop3 = colIndex;
    if (typeIsMixedArray(arrType)) {
        pop1 = variableMalloc();
        variableInitFromMixedArrayPromoteToSameType(pop1, arr);
#ifdef DEBUG_PRINT
        fprintf(stderr, "daf#10.1\n");
#endif
        freeList = freeListAppend(freeList, pop1);
    } else if (typeIsIntegerInterval(arrType)) {
        pop1 = variableMalloc();
        variableInitFromPCADPToIntegerVector(pop1, arr, &pcadpPromotionConfig);
#ifdef DEBUG_PRINT
        fprintf(stderr, "daf#10.2\n");
#endif
        freeList = freeListAppend(freeList, pop1);
    }

    ArrayType *pop1CTI = pop1->m_type->m_compoundTypeInfo;

    if (nIndex == 1 && pop1 == pop2) {
        // self indexed vector
        this->m_type = typeMalloc();
        if (pop1CTI->m_elementTypeID != ELEMENT_INTEGER) {
            errorAndExit("Self indexed vector must be integer vector!");
        }

        Variable *newSelf = NULL;
        typeInitFromNDArray(this->m_type, ELEMENT_INTEGER, 1, pop1CTI->m_dims,
                            pop1CTI->m_isString, NULL, true, true);
        Variable **vars = malloc(sizeof(Variable *));
        vars[0] = variableMalloc();
        variableInitFromNDArrayCopyByRef(vars[0], pop1);
        this->m_data = vars;
        variableAttrInitHelper(this, pop1->m_fieldPos, pop1->m_parent, false);
    } else {
        // promote both indices to either scalar integer or vector integer
        if (rowNDim != 1 && rowNDim != DIM_UNSPECIFIED) {  // rowIndex is scalar
            pop2 = variableMalloc();
            variableInitFromPCADPToIntegerScalar(pop2, rowIndex, &pcadpPromotionConfig);
            rowNDim = 0;
        } else {  // is a vector
            pop2 = variableMalloc();
            variableInitFromPCADPToIntegerVector(pop2, rowIndex, &pcadpPromotionConfig);
            rowNDim = 1;
        }
        if (nIndex == 2) {
            if (colNDim != 1 && colNDim != DIM_UNSPECIFIED) {
                pop3 = variableMalloc();
                variableInitFromPCADPToIntegerScalar(pop3, colIndex, &pcadpPromotionConfig);
                colNDim = 0;
            } else {  // is a vector
                pop3 = variableMalloc();
                variableInitFromPCADPToIntegerVector(pop3, colIndex, &pcadpPromotionConfig);
                colNDim = 1;
            }
        }

        Variable **vars = NULL;
        this->m_type = typeMalloc();
        if (pop1CTI->m_isRef) {
            Variable **pop1Vars = pop1->m_data;
            // merge the index, do not keep them

            ArrayType *selfCTI = pop1Vars[0]->m_type->m_compoundTypeInfo;
#ifdef DEBUG_PRINT
            fprintf(stderr, "calling refToValue from merging index\n");
#endif
            if (nIndex == 1) {
                ArrayType *pop2CTI = pop2->m_type->m_compoundTypeInfo;
                int8_t resultNDim = 0;
                int64_t *resultDims = NULL;
                if (rowNDim == 1) {
                    resultNDim = 1;
                    resultDims = pop2CTI->m_dims;
                }

                // the ownership is determined whether the self array is blocked scoped or a temporary vector
                Variable *newSelf = variableMalloc();
                variableInitFromNDArrayCopyByRef(newSelf, pop1Vars[0]);
                typeInitFromNDArray(this->m_type, selfCTI->m_elementTypeID, resultNDim, resultDims,
                                    selfCTI->m_isString && resultNDim == 1, NULL, true, false);

                switch (variableGetIndexRefTypeID(pop1)) {
                    case NDARRAY_INDEX_REF_SELF: {
                        vars = malloc(sizeof(Variable *) * 2);
                        vars[0] = newSelf;
                        vars[1] = variableMalloc();

                        Variable *temp1 = variableMalloc();
                        variableInitFromArrayIndexingHelper(temp1, pop1Vars[0], pop2, NULL, 1);
                        variableInitFromNDArrayIndexRefToValue(vars[1], temp1);
#ifdef DEBUG_PRINT
                        fprintf(stderr, "daf#3\n");
#endif
                        variableDestructThenFreeImpl(temp1);
                    } break;
                    case NDARRAY_INDEX_REF_1D: {
                        vars = malloc(sizeof(Variable *) * 2);
                        vars[0] = newSelf;
                        vars[1] = variableMalloc();

                        Variable *temp1 = variableMalloc();
                        variableInitFromArrayIndexingHelper(temp1, pop1Vars[1], pop2, NULL, 1);
                        variableInitFromNDArrayIndexRefToValue(vars[1], temp1);
#ifdef DEBUG_PRINT
                        fprintf(stderr, "daf#4\n");
#endif
                        variableDestructThenFreeImpl(temp1);
                    } break;
                    case NDARRAY_INDEX_REF_2D: {
                        vars = malloc(sizeof(Variable *) * 3);
                        vars[0] = newSelf;
                        vars[1] = variableMalloc();
                        vars[2] = variableMalloc();
                        int8_t pop1SelfRowIndexNDim = typeGetNDArrayNDims(pop1Vars[1]->m_type);

                        Variable *temp1 = variableMalloc();
                        if (pop1SelfRowIndexNDim == 1) {  // overwrite the row index
                            variableInitFromArrayIndexingHelper(temp1, pop1Vars[1], pop2, NULL, 1);
                            variableInitFromNDArrayIndexRefToValue(vars[1], temp1);
                            variableInitFromMemcpy(vars[2], pop1Vars[2]);
                        } else {  // overwrite the column index
                            variableInitFromArrayIndexingHelper(temp1, pop1Vars[2], pop2, NULL, 1);
                            variableInitFromNDArrayIndexRefToValue(vars[2], temp1);
                            variableInitFromMemcpy(vars[1], pop1Vars[1]);
                        }
#ifdef DEBUG_PRINT
                        fprintf(stderr, "daf#5\n");
#endif
                        variableDestructThenFreeImpl(temp1);
                    } break;
                    default:
                        errorAndExit("This should not happen!");
                }
            } else {
                ArrayType *pop2CTI = pop2->m_type->m_compoundTypeInfo;
                ArrayType *pop3CTI = pop3->m_type->m_compoundTypeInfo;
                int8_t resultNDim = 0;
                int64_t *resultDims = NULL;
                int64_t tempDims[2] = {0, 0};
                if (rowNDim == 1) {
                    resultNDim = 1;
                    resultDims = pop2CTI->m_dims;
                }
                if (colNDim == 1) {
                    resultNDim += 1;
                    if (resultNDim == 1)
                        resultDims = pop3CTI->m_dims;
                    else {  // matrix
                        tempDims[0] = pop2CTI->m_dims[0];
                        tempDims[1] = pop3CTI->m_dims[0];
                    }
                }

                // the ownership is determined whether the self array is blocked scoped or a temporary vector
                Variable *newSelf = variableMalloc();
                variableInitFromNDArrayCopyByRef(newSelf, pop1Vars[0]);
                typeInitFromNDArray(this->m_type, selfCTI->m_elementTypeID, resultNDim, resultDims,
                                    false, NULL, true, false);

                vars = malloc(sizeof(Variable *) * 3);
                vars[0] = newSelf;
                vars[1] = variableMalloc();
                vars[2] = variableMalloc();
                Variable *temp1 = variableMalloc();
                Variable *temp2 = variableMalloc();
                variableInitFromArrayIndexingHelper(temp1, pop1Vars[1], pop2, NULL, 1);
                variableInitFromArrayIndexingHelper(temp2, pop1Vars[2], pop2, NULL, 1);
                variableInitFromNDArrayIndexRefToValue(vars[1], temp1);
                variableInitFromNDArrayIndexRefToValue(vars[2], temp2);
#ifdef DEBUG_PRINT
                fprintf(stderr, "daf#6\n");
#endif
                variableDestructThenFreeImpl(temp1);
#ifdef DEBUG_PRINT
                fprintf(stderr, "daf#7\n");
#endif
                variableDestructThenFreeImpl(temp2);
            }
#ifdef DEBUG_PRINT
            fprintf(stderr, "daf#8\n");
#endif
            variableDestructThenFreeImpl(pop2);
            if (nIndex == 2) {
#ifdef DEBUG_PRINT
                fprintf(stderr, "daf#9\n");
#endif
                variableDestructThenFreeImpl(pop3);
            }
        } else {
            // not ref
            if (nIndex == 1) {
                ArrayType *pop2CTI = pop2->m_type->m_compoundTypeInfo;
                int8_t resultNDim = 0;
                int64_t *resultDims = NULL;
                if (rowNDim == 1) {
                    resultNDim = 1;
                    resultDims = pop2CTI->m_dims;
                }

                // the ownership is determined by whether the self array is blocked scoped or a temporary vector
                Variable *newSelf = variableMalloc();
                variableInitFromNDArrayCopyByRef(newSelf, pop1);
                typeInitFromNDArray(this->m_type, pop1CTI->m_elementTypeID, resultNDim, resultDims,
                                    pop1CTI->m_isString && resultNDim == 1, NULL, true, false);
                vars = malloc(sizeof(Variable *) * 2);
                vars[0] = newSelf;
                vars[1] = pop2;
            } else {
                ArrayType *pop2CTI = pop2->m_type->m_compoundTypeInfo;
                ArrayType *pop3CTI = pop3->m_type->m_compoundTypeInfo;
                int8_t resultNDim = 0;
                int64_t *resultDims = NULL;
                int64_t tempDims[2] = {0, 0};
                if (rowNDim == 1) {
                    resultNDim = 1;
                    resultDims = pop2CTI->m_dims;
                }
                if (colNDim == 1) {
                    resultNDim += 1;
                    if (resultNDim == 1)
                        resultDims = pop3CTI->m_dims;
                    else {  // matrix
                        tempDims[0] = pop2CTI->m_dims[0];
                        tempDims[1] = pop3CTI->m_dims[0];
                    }
                }

                Variable *newSelf = variableMalloc();
                variableInitFromNDArrayCopyByRef(newSelf, pop1);
                typeInitFromNDArray(this->m_type, pop1CTI->m_elementTypeID, resultNDim, resultDims,
                                    false, NULL, true, false);
                vars = malloc(sizeof(Variable *) * 3);
                vars[0] = newSelf;
                vars[1] = pop2;
                vars[2] = pop3;
            }
        }

        this->m_data = vars;
        variableAttrInitHelper(this, pop1->m_fieldPos, pop1->m_parent, false);
    }
#ifdef DEBUG_PRINT
    variableInitDebugPrint(this, "array index");
    fprintf(stderr, "daf#10\n");
#endif
    freeListFreeAll(freeList, (void (*)(void *)) variableDestructThenFreeImpl);
}

void variableInitFromNDArrayIndexRefToValue(Variable *this, Variable *ref) {
#ifdef DEBUG_PRINT
    if (!reentry)
        fprintf(stderr, "index ref to value from %p\n", ref);
#endif

    // first get total length and data type
    ArrayType *CTI = ref->m_type->m_compoundTypeInfo;
    ElementTypeID eid = CTI->m_elementTypeID;
    int64_t len = arrayTypeGetTotalLength(CTI);
    this->m_type = typeMalloc();
    typeInitFromNDArray(this->m_type, eid, CTI->m_nDim, CTI->m_dims,
                        CTI->m_isString && CTI->m_nDim == 1, NULL, false, false);
    this->m_data = arrayMallocFromNull(eid, len);
    variableAttrInitHelper(this, -1, this->m_data, false);

    for (int64_t i = 0; i < len; i++) {
        void *srcPtr = variableNDArrayGet(ref, i);
        void *targetPtr = variableNDArrayGet(this, i);
        elementAssign(eid, targetPtr, srcPtr);
    }

#ifdef DEBUG_PRINT
    variableInitDebugPrint(this, "index ref to value");
#endif
}

void variableInitFromVectorIndexing(Variable *this, Variable *arr, Variable *index) {
    variableInitFromArrayIndexingHelper(this, arr, index, NULL, 1);
}

void variableInitFromMatrixIndexing(Variable *this, Variable *arr, Variable *rowIndex, Variable *colIndex) {
    if (colIndex == NULL) {
        errorAndExit("Matrix indexing received NULL argument!");
    }
    variableInitFromArrayIndexingHelper(this, arr, rowIndex, colIndex, 2);
}

void variableNDArrayDestructor(Variable *this) {
    ArrayType *CTI = this->m_type->m_compoundTypeInfo;

    NDArrayIndexRefTypeID id = variableGetIndexRefTypeID(this);
    switch (id) {
        case NDARRAY_INDEX_REF_SELF:
        case NDARRAY_INDEX_REF_1D:
        case NDARRAY_INDEX_REF_2D: {
            Variable **vars = this->m_data;

            int nVar = 1;
            if (id == NDARRAY_INDEX_REF_1D) nVar = 2;
            if (id == NDARRAY_INDEX_REF_2D) nVar = 3;

            if (arrayTypeGetReferenceCount(CTI) <= 1) {
#ifdef DEBUG_PRINT
                fprintf(stderr, "daf#11\n");
#endif
                variableDestructThenFreeImpl(vars[0]);
                for (int i = 1; i < nVar; i++) {
#ifdef DEBUG_PRINT
                    fprintf(stderr, "daf#12\n");
#endif
                    variableDestructThenFreeImpl(vars[i]);
                }
                free(vars);
            }
        } break;
        case NDARRAY_INDEX_REF_NOT_A_REF: {
            if (arrayTypeGetReferenceCount(CTI) <= 1) {
                arrayFree(CTI->m_elementTypeID, this->m_data, arrayTypeGetTotalLength(CTI));
            }
        } break;
        default:
            errorAndExit("Unexpected typeid!");
    }
    typeDestructThenFree(this->m_type);
}

void variableInitFromNDArrayCopy(Variable *this, Variable *other) {
    NDArrayIndexRefTypeID id = variableGetIndexRefTypeID(other);
    if(id == NDARRAY_INDEX_REF_NOT_A_REF) {
        ArrayType *otherCTI = other->m_type->m_compoundTypeInfo;
        this->m_type = typeMalloc();
        typeInitFromCopy(this->m_type, other->m_type);
        this->m_data = arrayMallocFromMemcpy(otherCTI->m_elementTypeID, arrayTypeGetTotalLength(otherCTI),
                                             other->m_data);
        variableAttrInitHelper(this, -1, this->m_data, false);
    } else {
        variableInitFromNDArrayIndexRefToValue(this, other);
    }
#ifdef DEBUG_PRINT
    variableInitDebugPrint(this, "NDArray copy");
#endif
}

void variableInitFromNDArrayCopyByRef(Variable *this, Variable *other) {
    ArrayType *otherCTI = other->m_type->m_compoundTypeInfo;
    this->m_type = typeMalloc();
    this->m_type->m_typeId = other->m_type->m_typeId;
    this->m_type->m_compoundTypeInfo = arrayTypeMalloc();
    arrayTypeInitFromCopyByRef(this->m_type->m_compoundTypeInfo, otherCTI);
    this->m_data = other->m_data;
    variableAttrInitHelper(this, other->m_fieldPos, other->m_parent, false);
#ifdef DEBUG_PRINT
    variableInitDebugPrint(this, "NDArray copy by ref");
#endif
}

Variable *variableNDArrayIndexRefGetRootVariable(Variable *indexRef) {
    while (variableGetIndexRefTypeID(indexRef) != NDARRAY_INDEX_REF_NOT_A_REF) {
        Variable **vars = indexRef->m_data;
        indexRef = vars[0];
    }
    return indexRef;
}

void *ndarrayNonRefElementPtrGetter(Variable *this, int64_t pos) {
    ArrayType *CTI = this->m_type->m_compoundTypeInfo;
    int64_t len = arrayTypeGetTotalLength(CTI);
    if (pos < 0 || pos >= len)
        singleTypeError(this->m_type, "Array access out of range with type:");
    return arrayGetElementPtrAtIndex(CTI->m_elementTypeID, this->m_data, pos);
}

void *ndarrayRefElementPtrGetter(Variable *this, int64_t pos) {
    ArrayType *CTI = this->m_type->m_compoundTypeInfo;
    int64_t len = arrayTypeGetTotalLength(CTI);
    if (pos < 0 || pos >= len)
        singleTypeError(this->m_type, "Array access out of range with type:");

    Variable **vars = this->m_data;
    Variable *self = vars[0];
    if (CTI->m_isSelfRef) {
        int32_t selfIndex = variableGetIntegerElementAtIndex(self, pos) - 1;
        return variableNDArrayGet(self, selfIndex);
    } else {  // can be a vector or a matrix
        int8_t selfDim = variableGetNDim(self);
        if (selfDim == 1) {
            int32_t selfIndex = variableGetIntegerElementAtIndex(vars[1], pos) - 1;
            return variableNDArrayGet(self, selfIndex);
        } else if (selfDim == 2) {
            ArrayType *selfCTI = self->m_type->m_compoundTypeInfo;
            int64_t nCol = selfCTI->m_dims[1];

            int64_t colIndexArrayLen = arrayTypeGetTotalLength(vars[2]->m_type->m_compoundTypeInfo);
            int32_t selfRowIndex = variableGetIntegerElementAtIndex(vars[1], pos / colIndexArrayLen);
            int32_t selfColIndex = variableGetIntegerElementAtIndex(vars[2], pos % colIndexArrayLen);
            int32_t selfIndex = (selfRowIndex - 1) * (int32_t)nCol + (selfColIndex - 1);
            return variableNDArrayGet(self, selfIndex);
        } else {
            singleTypeError(self->m_type, "Attempt to call ndarray getter on type:");
        }
    }
}

void *variableNDArrayGet(Variable *this, int64_t pos) {
    NDArrayTypeID arrayTypeID = typeGetNDArrayTypeID(this->m_type);
    if (arrayTypeID == NDARRAY_NOT_AN_NDARRAY || arrayTypeID == NDARRAY_INVALID) {
        singleTypeError(this->m_type, "Attempt to call getter for variable of type:");
    }
    ArrayType *CTI = this->m_type->m_compoundTypeInfo;

    if (CTI->m_isRef) {
        return ndarrayRefElementPtrGetter(this, pos);
    } else {
        return ndarrayNonRefElementPtrGetter(this, pos);
    }
}

void *variableNDArrayCopyGet(Variable *this, int64_t pos) {
    void *target = variableNDArrayGet(this, pos);
    ArrayType *CTI = this->m_type->m_compoundTypeInfo;
    void *result = arrayMallocFromElementValue(CTI->m_elementTypeID, 1, target);
}

void variableNDArraySet(Variable *this, int64_t pos, void *val) {
    void *target = variableNDArrayGet(this, pos);
    ArrayType *CTI = this->m_type->m_compoundTypeInfo;
    elementAssign(CTI->m_elementTypeID, target, val);
}

NDArrayIndexRefTypeID variableGetIndexRefTypeID(Variable *this) {
    if (!typeIsArrayOrString(this->m_type))
        return NDARRAY_INDEX_REF_NOT_A_REF;
    ArrayType *CTI = this->m_type->m_compoundTypeInfo;
    if (!CTI->m_isRef)
        return NDARRAY_INDEX_REF_NOT_A_REF;

    if (CTI->m_isSelfRef) {
        return NDARRAY_INDEX_REF_SELF;
    } else {  // can be a vector or a matrix
        Variable **vars = this->m_data;
        Variable *self = vars[0];
        int8_t selfDim = variableGetNDim(self);
        if (selfDim == 1) {
            return NDARRAY_INDEX_REF_1D;
        } else if (selfDim == 2) {
            return NDARRAY_INDEX_REF_2D;
        } else {
            singleTypeError(self->m_type, "Attempt get index ref type id of type:");
        }
    }
}