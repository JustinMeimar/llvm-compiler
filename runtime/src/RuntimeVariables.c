#include "RuntimeVariables.h"
#include <stdlib.h>
#include "RuntimeErrors.h"
#include "NDArray.h"
#include "FreeList.h"
#include <string.h>
#include "Literal.h"
#include "VariableStdio.h"
#include "NDArrayVariable.h"

///------------------------------DATA---------------------------------------------------------------
// below explains how m_data is interpreted in each type of data
// integer - int32_t * - a pointer to single integer malloc
// real - float * - a pointer to single float malloc
// boolean - bool * - a pointer to single bool (which is int in c) malloc
// character - int8_t * - a pointer to single character malloc
// integer interval - int32_t * - a pointer to integer array of size 2
// integer[3] - int32_t * - a pointer to 3 int32_t element array
// integer[3, 3] - int32_t * - a pointer to 3 * 3 = 9 int32_t element array
// string[2] - int8_t * - a pointer to 2 int8_t element array
// tuple(character c, integer i) - Variable ** - a pointer to 2 Variable * pointers
// null & identity & [] & stream_in & stream_out - these types do not need to store associated data, m_data is ignored
// unknown - unknown type should not be associated with a variable

///------------------------------INTERFACES---------------------------------------------------------------

PCADPConfig pcadpParameterConfig = {
        true, true,
        true, false,
        vectovec_rhs_size_must_not_be_greater, false,
        true, false,
        true, true,
};
PCADPConfig pcadpCastConfig = {
        false, true,
        false, false,
        vectovec_rhs_size_can_be_any, true,
        true, true,
        true, false,
};
PCADPConfig pcadpAssignmentConfig = {
        false, false,
        false, true,
        vectovec_rhs_must_be_same_size, false,
        true, false,
        true, false,
};
PCADPConfig pcadpDeclarationConfig = {
        true, true,
        false, false,
        vectovec_rhs_size_must_not_be_greater, false,
        true, false,
        true, true,
};
PCADPConfig pcadpPromotionConfig = {
        true, false,
        false, false,
        vectovec_rhs_must_be_same_size, false,
        true, false,
        true, false,
};
PCADPConfig pcadpDomainExpressionConfig = {
        false, true,
        false, false,
        vectovec_rhs_must_be_same_size, false,
        true, false,
        false, false,
};

void variableInitFromPCADP(Variable *this, Type *targetType, Variable *rhs, PCADPConfig *config) {
    // in every possible case, we need to do one of the below two things
    // 1. throw an error when input types do not make sense
    // 2. initialize every field of this fully, including this->m_type and this->m_data

    // modify rhs and re-entry if rhs is array ref type
    if (variableGetIndexRefTypeID(rhs) != NDARRAY_INDEX_REF_NOT_A_REF) {
        Variable *rhsModified = variableMalloc();
#ifdef DEBUG_PRINT
        fprintf(stderr, "calling refToValue from PCADP\n");
#endif
        variableInitFromNDArrayIndexRefToValue(rhsModified, rhs);
        variableInitFromPCADP(this, targetType, rhsModified, config);
#ifdef DEBUG_PRINT
        fprintf(stderr, "daf#14\n");
#endif
        variableDestructThenFreeImpl(rhsModified);
        return;
    }

#ifdef DEBUG_PRINT
    if (config == &pcadpParameterConfig) {
        fprintf(stderr, "Param\n");
    } else if (config == &pcadpAssignmentConfig) {
        fprintf(stderr, "Assign\n");
    } else if (config == &pcadpCastConfig) {
        fprintf(stderr, "Cast\n");
    } else if (config == &pcadpDeclarationConfig) {
        fprintf(stderr, "Decl\n");
    } else if (config == &pcadpPromotionConfig) {
        fprintf(stderr, "Promote\n");
    }
#endif

    /// common local variables
    Type *rhsType = rhs->m_type;
    TypeID rhsTypeID = rhsType->m_typeId;
    // targetType
    TypeID targetTypeID = targetType->m_typeId;

    NDArrayTypeID rhsArrayTypeID = typeGetNDArrayTypeID(rhsType);
    NDArrayTypeID targetArrayTypeID = typeGetNDArrayTypeID(targetType);

    if (rhsTypeID == TYPEID_STREAM_IN || rhsTypeID == TYPEID_STREAM_OUT ||
            !typeIsVariableClassCompatible(rhsType)) {

        ///- stream_in/stream_out/unknown/unknown-size-array -> any: invalid rhs type
        singleTypeError(rhsType, "Invalid rhs type:");
    } else if (targetTypeID == TYPEID_STREAM_IN || targetTypeID == TYPEID_STREAM_OUT ||
            targetArrayTypeID == NDARRAY_MIXED || typeIsUnspecifiedInterval(targetType)) {

        ///- any -> stream_in/stream_out/array literal: invalid target type
        singleTypeError(targetType, "Invalid target type:");
    } else if (!config->m_allowRHSNullIdentity && (typeIsArrayNull(rhsType) || typeIsArrayIdentity(rhsType))) {
        singleTypeError(rhsType, "Null or identity not allowed to be rhs:");
    }

    if (typeIsArrayOrString(targetType)) {
        if (!config->m_allowUnknownTargetArraySize && arrayTypeHasUnknownSize(targetType->m_compoundTypeInfo)) {
            singleTypeError(targetType, "Attempt to convert into unknown size array:");
        }
    }

    ///- scalar null/identity -> any
    if (typeIsScalarNull(rhsType)) {
        variableInitFromNull(this, targetType);
        variableSetIsBlockScoped(this, config->m_resultIsBlockScoped);
        return;
    } else if (typeIsScalarIdentity(rhsType)) {
        variableInitFromIdentity(this, targetType);
        variableSetIsBlockScoped(this, config->m_resultIsBlockScoped);
        return;
    }

    /// any -> interval
    if (targetTypeID == TYPEID_INTERVAL) {
        // ndarray/interval -> interval
        if (rhsTypeID == TYPEID_NDARRAY) {
            // only possible case is when the ndarray is a scalar null/identity, which is checked before
            singleTypeError(rhsType, "Attempt to convert to interval from:");
        } else if (rhsTypeID == TYPEID_INTERVAL) {
            // we just copy it
            variableInitFromMemcpy(this, rhs);
            variableSetIsBlockScoped(this, config->m_resultIsBlockScoped);
        } else {
            singleTypeError(rhsType, "Attempt to convert to interval from:");
        }
        return;
    }

    /// any -> unknown
    if (targetTypeID == TYPEID_UNKNOWN) {
        if (!config->m_allowUnknownTargetType || typeIsArrayNull(rhsType) || typeIsArrayIdentity(rhsType)) {
            singleTypeError(rhsType, "Attempt to convert to unknown. rhsType:");
        } else if (rhsTypeID == TYPEID_NDARRAY) {
            ArrayType *rhsCTI = rhsType->m_compoundTypeInfo;
            if (rhsCTI->m_elementTypeID == ELEMENT_MIXED) {
                variableInitFromMixedArrayPromoteToSameType(this, rhs);
                variableSetIsBlockScoped(this, config->m_resultIsBlockScoped);
            } else {
                variableInitFromMemcpy(this, rhs);
                variableSetIsBlockScoped(this, config->m_resultIsBlockScoped);
            }
        } else {
            variableInitFromMemcpy(this, rhs);
            variableSetIsBlockScoped(this, config->m_resultIsBlockScoped);
        }
        return;
    }

    /// interval -> ndarray
    if (rhsTypeID == TYPEID_INTERVAL) {
        this->m_type = typeMalloc();
        if (targetTypeID == TYPEID_NDARRAY) {
            // interval -> vector of integer or real
            ArrayType *CTI = targetType->m_compoundTypeInfo;
            ElementTypeID eid = CTI->m_elementTypeID;
            if (CTI->m_nDim != 1)
                singleTypeError(targetType, "Attempt to convert interval to:");
            int64_t arrayLength = CTI->m_dims[0];

            int32_t *interval = rhs->m_data;
            int64_t size = interval[1] - interval[0] + 1;
            if (arrayLength < 0)
                arrayLength = size;  // for unspecified/unknown size we just assume it is the same size
            if (config->m_rhsSizeRestriction == vectovec_rhs_must_be_same_size && arrayLength != size
            || config->m_rhsSizeRestriction == vectovec_rhs_size_must_not_be_greater && size > arrayLength) {
                singleTypeError(targetType, "Attempt to convert interval to:");
            }
            int32_t *vec = arrayMallocFromNull(ELEMENT_INTEGER, arrayLength);

            int64_t resultSize = size < arrayLength ? size : arrayLength;
            for (int64_t i = 0; i < resultSize; i++) {
                vec[i] = interval[0] + (int32_t)i;
            }
            int64_t dims[1] = {resultSize};
            typeInitFromArrayType(this->m_type, false, eid, 1, dims);
            if (eid == ELEMENT_INTEGER) {
                this->m_data = vec;
            } else if (eid != ELEMENT_REAL) {
                singleTypeError(targetType, "Attempt to convert interval to:");
            } else {
                arrayMallocFromCast(eid, ELEMENT_INTEGER, resultSize, vec, &this->m_data);
                free(vec);
            }
        } else {
            singleTypeError(targetType, "Attempt to convert interval to:");
        }
        variableAttrInitHelper(this, -1, this->m_data, config->m_resultIsBlockScoped);
#ifdef DEBUG_PRINT
        variableInitDebugPrint(this, "interval -> ndarray");
#endif
        return;
    }

    /// tuple -> any
    /// any -> tuple
    if (rhsTypeID == TYPEID_TUPLE || targetTypeID == TYPEID_TUPLE) {
        if (rhsTypeID != TYPEID_TUPLE || targetTypeID != TYPEID_TUPLE) {
            singleTypeError(rhsType, "Tuple can not be converted to/from any other type, rhsType:");
        } else {
            // tuple -> tuple
            // we convert variables pair-wise
            this->m_type = typeMalloc();
            TupleType *rhsCTI = rhsType->m_compoundTypeInfo;
            TupleType *targetCTI = targetType->m_compoundTypeInfo;
            if (rhsCTI->m_nField != targetCTI->m_nField) {
                errorAndExit("Tuple can not be converted to/from tuple of different size!");
            }
            typeInitFromCopy(this->m_type, targetType);
            this->m_data = tupleTypeMallocDataFromPCADP(targetCTI, rhs, config);
            variableAttrInitHelper(this, -1, this->m_data, config->m_resultIsBlockScoped);
        }
#ifdef DEBUG_PRINT
        variableInitDebugPrint(this, "any -> tuple");
#endif
        return;
    }

    /// array -> array
    {
        ArrayType *rhsCTI = rhsType->m_compoundTypeInfo;
        int8_t rhsNDim = rhsCTI->m_nDim;
        int64_t *rhsDims = rhsCTI->m_dims;

        this->m_type = typeMalloc();
        ArrayType *targetCTI = targetType->m_compoundTypeInfo;
        typeInitFromNDArray(this->m_type, targetCTI->m_elementTypeID, targetCTI->m_nDim, targetCTI->m_dims,
                            targetCTI->m_isString, NULL, false, false);
        ArrayType *CTI = this->m_type->m_compoundTypeInfo;

        if (rhsNDim == DIM_UNSPECIFIED) {  // empty array -> array
            if (CTI->m_nDim == 0 || CTI->m_nDim == 2)
                singleTypeError(targetType, "Attempt to convert empty array into:");

            // otherwise we do conversion to null/identity/basic type vector/string/matrix, of unspecified/unknown/any size
            for (int64_t i = 0; i < CTI->m_nDim; i++) {
                int64_t dim = CTI->m_dims[i];
                if (dim > 0 && config->m_rhsSizeRestriction == vectovec_rhs_must_be_same_size) {
                    singleTypeError(targetType, "Attempt to convert empty array into a >= 1 size array:");
                }
                CTI->m_dims[i] = dim >= 0 ? dim : 0;
            }
            this->m_data = arrayMallocFromNull(CTI->m_elementTypeID, arrayTypeGetTotalLength(CTI));
        } else {  // non-empty array -> array
            int8_t nDim = CTI->m_nDim;
            int64_t *dims = CTI->m_dims;
            // TODO: check if this satisfies spec
            if (!config->m_allowArrToArrDifferentElementTypeConversion && rhsNDim != 0 &&
                CTI->m_elementTypeID != rhsCTI->m_elementTypeID && rhsCTI->m_elementTypeID != ELEMENT_MIXED) {
                errorAndExit("No vector/matrix to vector/matrix different element type conversion allowed");
            } else if (nDim < rhsNDim) {
                errorAndExit("Cannot convert to a lower dimension array!");
            } else if (nDim == 2 && rhsNDim == 1) {
                errorAndExit("Cannot convert from vector to matrix!");
            }

            void (*conversion)(ElementTypeID, ElementTypeID, int64_t, void *, void **)
            = config->m_isCast ? arrayMallocFromCast : arrayMallocFromPromote;
            void *convertedArray;
            conversion(CTI->m_elementTypeID, rhsCTI->m_elementTypeID, arrayTypeGetTotalLength(rhsCTI), rhs->m_data,
                       &convertedArray);

            if (rhsNDim == 0) {
                if (arrayTypeHasUnknownSize(CTI))
                    singleTypeError(targetType, "Attempt to convert scalar to unknown size array or string:");
                this->m_data = arrayMallocFromElementValue(CTI->m_elementTypeID, arrayTypeGetTotalLength(CTI),
                                                           convertedArray);
            } else if (rhsNDim == nDim) {
                for (int64_t i = 0; i < nDim; i++) {
                    if (dims[i] < 0)
                        dims[i] = rhsDims[i];
                }
                if (config->m_rhsSizeRestriction < arrayTypeMinimumCompatibleRestriction(rhsCTI, CTI)) {
                    errorAndExit("Incompatible vector size in convertion!");
                }
                // resize
                if (rhsNDim == 1) {
                    arrayMallocFromVectorResize(CTI->m_elementTypeID, convertedArray,
                                                rhsDims[0], dims[0], &this->m_data);
                } else {
                    arrayMallocFromMatrixResize(CTI->m_elementTypeID, convertedArray,
                                                rhsDims[0], rhsDims[1], dims[0], dims[1], &this->m_data);
                }
            }
            free(convertedArray);
        }
        variableAttrInitHelper(this, -1, this->m_data, config->m_resultIsBlockScoped);
#ifdef DEBUG_PRINT
        variableInitDebugPrint(this, "array -> array");
#endif
        return;
    }
}

Variable *variableMalloc() {
    Variable *var = malloc(sizeof(Variable));
#ifdef DEBUG_PRINT
    if (!reentry)
        fprintf(stderr, "(malloc var %p)\n", (void *)var);
#endif
    return var;
}

void variableInitFromMemcpy(Variable *this, Variable *other) {
    if (other->m_type->m_typeId == TYPEID_NDARRAY) {
        variableInitFromNDArrayCopy(this, other);
        return;
    }
    this->m_type = typeMalloc();
    typeInitFromCopy(this->m_type, other->m_type);
    TypeID id = this->m_type->m_typeId;
    this->m_data = NULL;
    switch (id) {
        case TYPEID_TUPLE: {
            TupleType *CTI = other->m_type->m_compoundTypeInfo;
            this->m_data = tupleTypeMallocDataFromCopy(CTI, other->m_data);
        } break;
        case TYPEID_STREAM_IN:
        case TYPEID_STREAM_OUT:
            break;  // m_data is ignored for these types
        case TYPEID_INTERVAL: {
            this->m_data = intervalTypeMallocDataFromCopy(other->m_data);
        } break;
        case TYPEID_UNKNOWN:
        case NUM_TYPE_IDS:
        default:
            singleTypeError(this->m_type, "Unexpected type: ");
            break;
    }
    variableAttrInitHelper(this, -1, this->m_data, false);
#ifdef DEBUG_PRINT
    variableInitDebugPrint(this, "memcpy");
#endif
}

void variableInitFromIdentifier(Variable *this, Variable *other) {
    variableInitFromMemcpy(this, other);
}

// <type> var = null;
void variableInitFromNull(Variable *this, Type *type) {
    this->m_type = typeMalloc();
    if (type == NULL)
        type = this->m_type;  // use its own type;
    else
        typeInitFromCopy(this->m_type, type);
    TypeID typeid = type->m_typeId;

    switch (typeid) {
        case TYPEID_NDARRAY: {
            ArrayType *CTI = type->m_compoundTypeInfo;
            if (arrayTypeHasUnknownSize(CTI))
                singleTypeError(type, "Attempt to promote null into unknown type: ");
            this->m_data = arrayMallocFromNull(CTI->m_elementTypeID, arrayTypeGetTotalLength(CTI));
        } break;
        case TYPEID_INTERVAL: {
            IntervalType *CTI = type->m_compoundTypeInfo;
            if (intervalTypeIsUnspecified(CTI))
                singleTypeError(type, "Attempt to promote null into unknown type: ");
            this->m_data = intervalTypeMallocDataFromNull();
        } break;
        case TYPEID_TUPLE: {
            this->m_data = tupleTypeMallocDataFromNull(type->m_compoundTypeInfo);
        } break;
        case TYPEID_STREAM_IN:
        case TYPEID_STREAM_OUT:
        case TYPEID_UNKNOWN:
        default:
            singleTypeError(type, "Can't init variable of type from null: ");
            break;
    }
    variableAttrInitHelper(this, -1, this->m_data, false);
#ifdef DEBUG_PRINT
    variableInitDebugPrint(this, "null -> any");
#endif
}

// <type> var = identity;
void variableInitFromIdentity(Variable *this, Type *type) {
    this->m_type = typeMalloc();
    typeInitFromCopy(this->m_type, type);
    TypeID typeid = type->m_typeId;

    switch (typeid) {
        case TYPEID_NDARRAY: {
            ArrayType *CTI = type->m_compoundTypeInfo;
            if (arrayTypeHasUnknownSize(CTI))
                singleTypeError(type, "Attempt to promote identity into unknown type: ");
            this->m_data = arrayMallocFromIdentity(CTI->m_elementTypeID, arrayTypeGetTotalLength(CTI));
        } break;
        case TYPEID_INTERVAL: {
            IntervalType *CTI = type->m_compoundTypeInfo;
            if (intervalTypeIsUnspecified(CTI))
                singleTypeError(type, "Attempt to promote null into unknown type: ");
            this->m_data = intervalTypeMallocDataFromIdentity();
        } break;
        case TYPEID_TUPLE: {
            this->m_data = tupleTypeMallocDataFromIdentity(type->m_compoundTypeInfo);
        } break;
        case TYPEID_STREAM_IN:
        case TYPEID_STREAM_OUT:
        case TYPEID_UNKNOWN:
        default:
            singleTypeError(type, "Can't init variable of type from null: ");
            break;
    }
    variableAttrInitHelper(this, -1, this->m_data, false);
#ifdef DEBUG_PRINT
    variableInitDebugPrint(this, "identity -> any");
#endif
}

void variableInitFromUnaryOp(Variable *this, Variable *operand, UnaryOpCode opcode) {
    // modify rhs and re-entry if rhs is array ref type
    if (variableGetIndexRefTypeID(operand) != NDARRAY_INDEX_REF_NOT_A_REF) {
        Variable *rhsModified = variableMalloc();
#ifdef DEBUG_PRINT
        fprintf(stderr, "calling refToValue from unary op\n");
#endif
        variableInitFromNDArrayIndexRefToValue(rhsModified, operand);
        variableInitFromUnaryOp(this, rhsModified, opcode);
#ifdef DEBUG_PRINT
        fprintf(stderr, "daf#15\n");
#endif
        variableDestructThenFreeImpl(rhsModified);
        return;
    }

    Type *operandType = operand->m_type;
    if (!typeIsVariableClassCompatible(operandType)) {
        singleTypeError(operandType, "Attempt to perform unary op on type that is not a concrete type: ");
    }
    this->m_type = typeMalloc();
    typeInitFromCopy(this->m_type, operandType);

    if (operandType->m_typeId == TYPEID_INTERVAL) {
        // +ivl or -ivl
        int32_t *interval = intervalTypeMallocDataFromNull();
        if (opcode == UNARY_MINUS) {
            intervalTypeUnaryMinus(interval, operand->m_data);
        } else if (opcode == UNARY_PLUS) {
            intervalTypeUnaryPlus(interval, operand->m_data);
        } else {
            singleTypeError(operandType, "Attempt to perform unary 'not' on type: ");
        }
        this->m_data = interval;
    } else if (operandType->m_typeId == TYPEID_NDARRAY) {
        ArrayType *CTI = this->m_type->m_compoundTypeInfo;
        arrayMallocFromUnaryOp(CTI->m_elementTypeID, opcode, operand->m_data,
                               arrayTypeGetTotalLength(CTI), &this->m_data);
    } else {
        singleTypeError(operandType, "Attempt to perform unary operation on type: ");
    }
    variableAttrInitHelper(this, -1, this->m_data, false);
#ifdef DEBUG_PRINT
    variableInitDebugPrint(this, "unary op");
#endif
}

void computeIvlByIntBinop(Variable *this, Variable *op1, Variable *op2, BinOpCode opcode) {
    variableInitFromIntervalStep(this, op1, op2);
}

void computeSameTypeSameSizeArrayArrayBinop(Variable *this, Variable *op1, Variable *op2, BinOpCode opcode) {
    Type *op1Type = op1->m_type;
    ArrayType *op1CTI = op1->m_type->m_compoundTypeInfo;
    int64_t arrSize = arrayTypeGetTotalLength(op1CTI);
    ElementTypeID resultEID;
    bool resultCollapseToScalar;
    if (!arrayBinopResultType(op1CTI->m_elementTypeID, opcode, &resultEID, &resultCollapseToScalar)) {
        errorAndExit("Cannot perform binop between two array variables!");
    }

    // init type
    this->m_type = typeMalloc();
    if (resultCollapseToScalar) {
        typeInitFromArrayType(this->m_type, false, resultEID, 0, NULL);
    } else {
        typeInitFromArrayType(this->m_type, false, resultEID, op1CTI->m_nDim, op1CTI->m_dims);
    }
    ArrayType *CTI = this->m_type->m_compoundTypeInfo;
    if (CTI->m_elementTypeID != resultEID) {
        errorAndExit("The newly initialized type has a different eid than the expected result type from arrayBinopResultType!");
    }

    // init data
    arrayMallocFromBinOp(op1CTI->m_elementTypeID, opcode, op1->m_data, arrSize,
                         op2->m_data, arrSize, &this->m_data, NULL);
}

void computeIvlIvlBinop(Variable *this, Variable *op1, Variable *op2, BinOpCode opcode) {
    this->m_type = typeMalloc();
    if (opcode == BINARY_EQ || opcode == BINARY_NE) {
        // result is boolean
        typeInitFromArrayType(this->m_type, false, ELEMENT_BOOLEAN, 0, NULL);
        this->m_data = arrayMallocFromNull(ELEMENT_BOOLEAN, 1);
    } else {
        typeInitFromIntervalType(this->m_type, INTEGER_BASE_INTERVAL);
        this->m_data = intervalTypeMallocDataFromNull();
    }
    switch(opcode) {
        case BINARY_MULTIPLY:
            intervalTypeBinaryMultiply(this->m_data, op1->m_data, op2->m_data); break;
        case BINARY_PLUS:
            intervalTypeBinaryPlus(this->m_data, op1->m_data, op2->m_data); break;
        case BINARY_MINUS:
            intervalTypeBinaryMinus(this->m_data, op1->m_data, op2->m_data); break;
        case BINARY_EQ:
            intervalTypeBinaryEq(this->m_data, op1->m_data, op2->m_data); break;
        case BINARY_NE:
            intervalTypeBinaryNe(this->m_data, op1->m_data, op2->m_data); break;
        default:
            errorAndExit("Unexpected opcode!"); break;
    }
}

// compute function is responsible for initializing this->m_type and this->m_data from the two promoted variables
void binopPromoteComputationAndDispose(Variable *this, Variable *op1, Variable *op2, BinOpCode opcode, Type *promoteOp1To, Type *promoteOp2To,
                                       void compute(Variable *this, Variable *op1, Variable *op2, BinOpCode opcode)) {
    Variable *op1Promoted = variableMalloc();
    Variable *op2Promoted = variableMalloc();
    variableInitFromPromotion(op1Promoted, promoteOp1To, op1);
    variableInitFromPromotion(op2Promoted, promoteOp2To, op2);

    compute(this, op1Promoted, op2Promoted, opcode);

#ifdef DEBUG_PRINT
    fprintf(stderr, "daf#16&17\n");
#endif
    variableDestructThenFreeImpl(op1Promoted);
    variableDestructThenFreeImpl(op2Promoted);

    variableAttrInitHelper(this, -1, this->m_data, false);
#ifdef DEBUG_PRINT
    variableInitDebugPrint(this, "from binop helper");
#endif
}


// guarantee op1 and op2 can only be spec types
void variableInitFromBinaryOpWithSpecTypes(Variable *this, Variable *op1, Variable *op2, BinOpCode opcode) {
    ///remaining types: empty array, ndarray/string, interval, tuple
    ///remaining opcodes: every op besides concat
    Type *op1Type = op1->m_type;
    Type *op2Type = op2->m_type;

    if (typeIsEmptyArray(op1Type) || typeIsEmptyArray(op2Type)) {
        errorAndExit("Unexpected empty array in binary op!");
    }

    if (opcode == BINARY_RANGE_CONSTRUCT) {
        // promote both sides to integer and retrieve the result then create a new interval from it
        variableInitFromIntervalHeadTail(this, op1, op2);
        return;
    } else if (opcode == BINARY_INDEX) {
        errorAndExit("Call index interfaces instead of binary op for index operations!");
    }

    switch (opcode) {
        case BINARY_EXPONENT:
        case BINARY_MULTIPLY:
        case BINARY_DIVIDE:
        case BINARY_REMAINDER:
        case BINARY_DOT_PRODUCT:
        case BINARY_PLUS:
        case BINARY_MINUS:
        case BINARY_LT:
        case BINARY_BT:
        case BINARY_LEQ:
        case BINARY_BEQ:
        case BINARY_EQ:
        case BINARY_NE:
        case BINARY_AND:
        case BINARY_OR:
        case BINARY_XOR: {  /// same type same size
            /// interval op interval
            if (op1Type->m_typeId == TYPEID_INTERVAL && !typeIsVectorOrString(op2Type)
                || op2Type->m_typeId == TYPEID_INTERVAL && !typeIsVectorOrString(op1Type)) {
                Type *ivlType = typeMalloc();
                typeInitFromIntervalType(ivlType, INTEGER_BASE_INTERVAL);
                binopPromoteComputationAndDispose(this, op1, op2, opcode, ivlType, ivlType,
                                                  computeIvlIvlBinop);
                typeDestructThenFree(ivlType);
            } else if (op1Type->m_typeId == TYPEID_INTERVAL || op2Type->m_typeId == TYPEID_INTERVAL) {
                Variable *vec = typeIsArrayOrString(op1Type) ? op1 : op2;
                Type *targetType = typeMalloc();
                typeInitFromCopy(targetType, vec->m_type);
                // in this case we may have vector null element promote to interval element
                ArrayType *CTI = targetType->m_compoundTypeInfo;
                if (!elementCanBePromotedBetween(ELEMENT_INTEGER, CTI->m_elementTypeID, &CTI->m_elementTypeID)) {
                    errorAndExit("Cannot promote between interval and vector!");
                }
                binopPromoteComputationAndDispose(this, op1, op2, opcode, targetType, targetType,
                                                  computeSameTypeSameSizeArrayArrayBinop);
                typeDestructThenFree(targetType);
            } else if (typeIsArrayOrString(op1Type) && typeIsArrayOrString(op2Type)) {
                // a special case is matrix multiplication, which is not an element-wise operation
                ArrayType *op1CTI = op1Type->m_compoundTypeInfo;
                ArrayType *op2CTI = op2Type->m_compoundTypeInfo;
                int8_t op1NDim = variableGetNDim(op1);
                int8_t op2NDim = variableGetNDim(op2);
                if (opcode == BINARY_DOT_PRODUCT && (op1NDim == 2 || op2NDim == 2)) {
                    if (!(op1NDim == 2 && op2NDim == 2))
                        errorAndExit("Matrix multiplication can only happen between two matrices");

                    ElementTypeID resultEID;
                    if (!elementCanBePromotedBetween(op1CTI->m_elementTypeID, op2CTI->m_elementTypeID,&resultEID)) {
                        errorAndExit("Cannot promote between interval and vector!");
                    } else if (op1CTI->m_dims[1] != op2CTI->m_dims[0]) {
                        errorAndExit("Can't multiply two matrices with non-matching size!");
                    }

                    Type *targetType1 = typeMalloc();
                    typeInitFromCopy(targetType1, op1Type);
                    ArrayType *target1CTI = targetType1->m_compoundTypeInfo;
                    target1CTI->m_elementTypeID = resultEID;
                    Variable *target1 = variableMalloc();
                    variableInitFromPromotion(target1, targetType1, op1);
                    typeDestructThenFree(targetType1);

                    Type *targetType2 = typeMalloc();
                    typeInitFromCopy(targetType2, op2Type);
                    ArrayType *target2CTI = targetType2->m_compoundTypeInfo;
                    target1CTI->m_elementTypeID = resultEID;
                    Variable *target2 = variableMalloc();
                    variableInitFromPromotion(target2, targetType2, op2);
                    typeDestructThenFree(targetType2);

                    Type *resultType = typeMalloc();
                    int64_t dims[2] = {op1CTI->m_dims[0], op2CTI->m_dims[1]};
                    typeInitFromNDArray(resultType, resultEID, 2, dims,
                                        false, NULL, false, false);
                    this->m_type = resultType;
                    arrayMallocFromMatrixMultiplication(resultEID, target1->m_data, target2->m_data,
                                                        op1CTI->m_dims[0], op1CTI->m_dims[1], op2CTI->m_dims[1],
                                                        &this->m_data);
                    variableAttrInitHelper(this, -1, this->m_data, false);

#ifdef DEBUG_PRINT
                    fprintf(stderr, "daf#18&19\n");
#endif
                    variableDestructThenFreeImpl(target1);
                    variableDestructThenFreeImpl(target2);
                } else {
                    Type *targetType = typeMalloc();
                    typeInitFromCopy(targetType, op1Type);
                    ArrayType *CTI = targetType->m_compoundTypeInfo;
                    if (!elementCanBePromotedBetween(op1CTI->m_elementTypeID, op2CTI->m_elementTypeID,
                                                     &CTI->m_elementTypeID)) {
                        errorAndExit("Cannot promote between vectors!");
                    }
                    binopPromoteComputationAndDispose(this, op1, op2, opcode, targetType, targetType,
                                                      computeSameTypeSameSizeArrayArrayBinop);
                    typeDestructThenFree(targetType);
                }
            } else if (op1Type->m_typeId == TYPEID_TUPLE || op2Type->m_typeId == TYPEID_TUPLE) {
                // must both be tuple now because we have ruled out possibility of null/identity before
                TupleType *op1CTI = op1Type->m_compoundTypeInfo;
                TupleType *op2CTI = op2Type->m_compoundTypeInfo;
                Variable **op1Vars = op1->m_data;
                Variable **op2Vars = op2->m_data;

                bool result = true;
                for (int64_t i = 0; i < op1CTI->m_nField; i++) {
                    Variable *temp = variableMalloc();
                    variableInitFromBinaryOp(temp, op1Vars[i], op2Vars[i], BINARY_EQ);
                    bool *resultBool = temp->m_data;
                    result = result && *resultBool;
#ifdef DEBUG_PRINT
                    fprintf(stderr, "daf#20\n");
#endif
                    variableDestructThenFreeImpl(temp);
                }
                if (opcode == BINARY_NE)
                    result = !result;
                variableInitFromBooleanScalar(this, result);
            } else {
                errorAndExit("Unexpected type on binary operation!");
            }
        } break;
        case BINARY_BY: {
            Type *ivlType = typeMalloc();
            typeInitFromIntervalType(ivlType, INTEGER_BASE_INTERVAL);
            Type *intType = typeMalloc();
            typeInitFromArrayType(intType, false, ELEMENT_INTEGER, 0, NULL);

            binopPromoteComputationAndDispose(this, op1, op2, opcode, ivlType, intType, computeIvlByIntBinop);

            typeDestructThenFree(ivlType);
            typeDestructThenFree(intType);
        } break;
        default:
            errorAndExit("unexpected binop!"); break;
    }
}

void variableInitFromConcat(Variable *this, Variable *op1, Variable *op2) {
    Type *op1Type = op1->m_type;
    Type *op2Type = op2->m_type;
    int8_t op1nDim = typeGetNDArrayNDims(op1Type);
    int8_t op2nDim = typeGetNDArrayNDims(op2Type);
    if (op1nDim == DIM_INVALID || op1nDim == 2) {
        singleTypeError(op1Type, "Attempt to concat with type: ");
    } else if (op1nDim == DIM_INVALID || op2nDim == 2) {
        singleTypeError(op2Type, "Attempt to concat with type: ");
    }

    // [] || [] -> []
    // [] || A -> A
    // A || [] -> A
    if (op1nDim == DIM_UNSPECIFIED) {
        if (typeIsScalar(op2Type)) {
            ArrayType *CTI = op2Type->m_compoundTypeInfo;
            int64_t dims[1] = {1};
            variableInitFromNDArray(this, false, CTI->m_elementTypeID, 1, dims, op2->m_data, true);
        } else {
            variableInitFromMemcpy(this, op2);
        }
        return;
    } else if (op2nDim == DIM_UNSPECIFIED) {
        if (typeIsScalar(op1Type)) {
            ArrayType *CTI = op1Type->m_compoundTypeInfo;
            int64_t dims[1] = {1};
            variableInitFromNDArray(this, false, CTI->m_elementTypeID, 1, dims, op1->m_data, true);
        } else {
            variableInitFromMemcpy(this, op1);
        }
        return;
    }

    if (op1nDim == 0 && op2nDim == 0) {
        errorAndExit("Cannot concat two scalar varaibles!");
    }
    ArrayType *op1CTI = op1Type->m_compoundTypeInfo;
    ElementTypeID op1EID = op1CTI->m_elementTypeID;
    int64_t arr1Length = arrayTypeGetTotalLength(op1CTI);

    ArrayType *op2CTI = op2Type->m_compoundTypeInfo;
    ElementTypeID op2EID = op2CTI->m_elementTypeID;
    int64_t arr2Length = arrayTypeGetTotalLength(op2CTI);

    ElementTypeID resultEID;
    if (!elementCanBePromotedBetween(op1EID, op2EID, &resultEID)) {
        errorAndExit("Attempt to concat two vectors of incompatible element types");
    }

    FreeList *freeList = NULL;
    void *pop1Data = op1->m_data;
    if (op1EID != resultEID) {
        arrayMallocFromPromote(resultEID, op1EID, arr1Length, op1->m_data, &pop1Data);
        freeList = freeListAppend(freeList, pop1Data);
    }
    void *pop2Data = op2->m_data;
    if (op2EID != resultEID) {
        arrayMallocFromPromote(resultEID, op2EID, arr2Length, op2->m_data, &pop2Data);
        freeList = freeListAppend(freeList, pop2Data);
    }

    arrayMallocFromBinOp(resultEID, BINARY_CONCAT,
                         pop1Data, arr1Length,
                         pop2Data, arr2Length,
                         &this->m_data, NULL);
    this->m_type = typeMalloc();
    int64_t dims[1] = {arr1Length + arr2Length};
    bool isString = op1CTI->m_isString || op2CTI->m_isString;
    typeInitFromArrayType(this->m_type, isString, resultEID, 1, dims);

    freeListFreeAll(freeList, free);

    variableAttrInitHelper(this, -1, this->m_data, false);
#ifdef DEBUG_PRINT
    variableInitDebugPrint(this, "binop concat");
#endif
}

/// binary op only cares about the content of two operands
/// therefore array ref types are converted to array values before binary op
/// and index operation is not a binary operator
void variableInitFromBinaryOp(Variable *this, Variable *op1, Variable *op2, BinOpCode opcode) {
    // this function is responsible for dealing with null/identity/mixed arrays
    Type *op1Type = op1->m_type;
    Type *op2Type = op2->m_type;
    if (typeIsStream(op1Type) || typeIsStream(op2Type)) {
        errorAndExit("binary op cannot be performed on stream object!");
    } else if (!typeIsVariableClassCompatible(op1Type) || !typeIsVariableClassCompatible(op2Type)) {
        errorAndExit("binary op can only be performed on concrete type!");
    }
    Variable *pop1 = op1;
    Variable *pop2 = op2;
    // mixed array and index refs
    FreeList *freeList = NULL;
    Variable *temp = variableConvertLiteralAndRefToConcreteArray(op1);
    if (temp) {
        pop1 = temp;
        freeList = freeListAppend(freeList, temp);
    }
    temp = variableConvertLiteralAndRefToConcreteArray(op2);
    if (temp) {
        pop2 = temp;
        freeList = freeListAppend(freeList, temp);
    }
    // null/identity array
    // note special case: cocat op "||" promotes to one element array instead of two
    // second special case: interval by integer
    if (opcode == BINARY_CONCAT) {
        variableInitFromConcat(this, pop1, pop2);
    } else {
        if (typeIsArrayNull(op1Type) || typeIsArrayIdentity((op1Type))) {
            Variable *temp = pop1;
            pop1 = variableMalloc();
            variableInitFromPromotion(pop1, pop2->m_type, temp);
            freeList = freeListAppend(freeList, pop1);
        }
        if (typeIsArrayNull(op2Type) || typeIsArrayIdentity((op2Type))) {
            Variable *temp = pop2;
            pop2 = variableMalloc();
            variableInitFromPromotion(pop2, pop1->m_type, temp);
            freeList = freeListAppend(freeList, pop2);
        }
        variableInitFromBinaryOpWithSpecTypes(this, pop1, pop2, opcode);
    }

#ifdef DEBUG_PRINT
    fprintf(stderr, "daf#21\n");
#endif
    freeListFreeAll(freeList, (void (*)(void *)) variableDestructThenFreeImpl);
}

void variableInitFromParameter(Variable *this, Type *lhsType, Variable *rhs) {
    variableInitFromPCADP(this, lhsType, rhs, &pcadpParameterConfig);
}
void variableInitFromCast(Variable *this, Type *lhsType, Variable *rhs) {
    variableInitFromPCADP(this, lhsType, rhs, &pcadpCastConfig);
}
void variableInitFromAssign(Variable *this, Type *lhsType, Variable *rhs) {
    variableInitFromPCADP(this, lhsType, rhs, &pcadpAssignmentConfig);
}
void variableInitFromDeclaration(Variable *this, Type *lhsType, Variable *rhs) {
    variableInitFromPCADP(this, lhsType, rhs, &pcadpDeclarationConfig);
}
void variableInitFromPromotion(Variable *this, Type *lhsType, Variable *rhs) {
    variableInitFromPCADP(this, lhsType, rhs, &pcadpPromotionConfig);
}

void variableInitFromDomainExpression(Variable *this, Variable *rhs) {
    variableInitFromPCADP1dVariableToVector(this, rhs, &pcadpDomainExpressionConfig);
}

void variableInitFromPCADP1dVariableToVector(Variable *this, Variable *rhs, PCADPConfig *config) {
    Type *vec = typeMalloc();
    int64_t dims[1] = { SIZE_UNKNOWN };
    ElementTypeID resultEID = ELEMENT_INTEGER;
    switch (rhs->m_type->m_typeId) {
        case TYPEID_NDARRAY: {
            ArrayType *rhsCTI = rhs->m_type->m_compoundTypeInfo;
            resultEID = rhsCTI->m_elementTypeID;
            switch (resultEID) {

                case ELEMENT_INTEGER:
                case ELEMENT_REAL:
                case ELEMENT_BOOLEAN:
                case ELEMENT_CHARACTER:
                    break;
                case ELEMENT_MIXED:
                    variableInitFromMixedArrayPromoteToSameType(this, rhs);
                    return;  // the responsibility of initializing this variable is transferred to mixed array promotion, so we don't need to go further
                default:
                    singleTypeError(rhs->m_type, "RHS is null or identity when trying to convert to vector!");
            }
        } break;
        case TYPEID_INTERVAL: {
            resultEID = ELEMENT_INTEGER;
        } break;
        default:
            singleTypeError(rhs->m_type, "Invalid rhs type!");
    }
    typeInitFromArrayType(vec, false, resultEID, 1, dims);
    variableInitFromPCADP(this, vec, rhs, config);
    typeDestructThenFree(vec);
}

void variableInitFromPCADPToIntegerVector(Variable *this, Variable *rhs, PCADPConfig *config) {
    Type *intVec = typeMalloc();
    int64_t dims[1] = { SIZE_UNKNOWN };
    typeInitFromArrayType(intVec, false, ELEMENT_INTEGER, 1, dims);
    variableInitFromPCADP(this, intVec, rhs, config);
    typeDestructThenFree(intVec);
}

void variableInitFromPCADPToIntegerScalar(Variable *this, Variable *rhs, PCADPConfig *config) {
    Type *intTy = typeMalloc();
    typeInitFromArrayType(intTy, false, ELEMENT_INTEGER, 0, NULL);
    variableInitFromPCADP(this, intTy, rhs, config);
    typeDestructThenFree(intTy);
}

void variableInitFromScalarToConcreteArray(Variable *this, Variable *scalar, int8_t nDim, int64_t *dims, bool isString) {
    void *val = variableNDArrayGet(scalar, 0);
    ArrayType *scalarCTI = scalar->m_type->m_compoundTypeInfo;
    variableInitFromNDArray(this, isString, scalarCTI->m_elementTypeID, nDim, dims, val, true);
}

void variableInitFromMixedArrayPromoteToSameType(Variable *this, Variable *mixed) {
    Type *rhsType = mixed->m_type;
    ArrayType *rhsCTI = rhsType->m_compoundTypeInfo;
    int64_t size = arrayTypeGetTotalLength(rhsCTI);
    if (size == 0)
        errorAndExit("Attempt to promote 0-size mixed array (empty array) to same type, unable to deduce the result type!");
    this->m_type = typeMalloc();
    typeInitFromCopy(this->m_type, rhsType);
    ArrayType *CTI = this->m_type->m_compoundTypeInfo;
    if (!arrayMixedElementCanBePromotedToSameType(mixed->m_data, size,
                                                  &CTI->m_elementTypeID)) {
        singleTypeError(rhsType, "Attempt to convert to homogenous array from:");
    }
    arrayMallocFromPromote(CTI->m_elementTypeID, ELEMENT_MIXED, size, mixed->m_data, &this->m_data);
    variableAttrInitHelper(this, -1, this->m_data, false);
#ifdef DEBUG_PRINT
    variableInitDebugPrint(this, "mixed array promote to same type");
#endif
}

void variableInitFromIntervalHeadTail(Variable *this, Variable *head, Variable *tail) {
    this->m_type = typeMalloc();
    typeInitFromIntervalType(this->m_type, INTEGER_BASE_INTERVAL);
    this->m_data = intervalTypeMallocDataFromHeadTail(variableGetIntegerValue(head), variableGetIntegerValue(tail));
    variableAttrInitHelper(this, -1, this->m_data, false);
#ifdef DEBUG_PRINT
    variableInitDebugPrint(this, "interval head tail");
#endif
}

void variableInitFromIntervalStep(Variable *this, Variable *ivl, Variable *step) {
    int32_t *interval = ivl->m_data;
    int32_t k = *((int32_t *)step->m_data);
    if (k <= 0) {
        errorAndExit("ivl by step has a step value of 0 or negative!");
    }
    int32_t value = 0;
    int64_t dims[1] = {(interval[1] - interval[0]) / k + 1};

    variableInitFromNDArray(this, false, ELEMENT_INTEGER, 1, dims, &value, true);
    int32_t *vec = this->m_data;
    for (int64_t i = 0; i < dims[0]; i++) {
        vec[i] = interval[0] + (int32_t)i * k;
    }
#ifdef DEBUG_PRINT
    variableInitDebugPrint(this, "interval step");
#endif
}

void variableInitFromNDArray(Variable *this, bool isString, ElementTypeID eid, int8_t nDim, int64_t *dims,
                             void *value, bool valueIsScalar) {
    this->m_type = typeMalloc();
    typeInitFromArrayType(this->m_type, isString, eid, nDim, dims);
    int64_t totalLength = arrayTypeGetTotalLength(this->m_type->m_compoundTypeInfo);
    int64_t elementSize = elementGetSize(eid);

    if (value != NULL) {
        if (valueIsScalar) {
            this->m_data = arrayMallocFromElementValue(eid, totalLength, value);
        } else {
            this->m_data = arrayMallocFromNull(eid, totalLength);
            memcpy(this->m_data, value, elementSize * totalLength);
        }
    } else {
        this->m_data = arrayMallocFromNull(eid, totalLength);
    }
    variableAttrInitHelper(this, -1, this->m_data, false);
#ifdef DEBUG_PRINT
    variableInitDebugPrint(this, "from ndarray");
#endif
}

Variable *variableGetTupleField(Variable *tuple, int64_t pos) {
    TupleType *CTI = tuple->m_type->m_compoundTypeInfo;
    if (pos <= 0 || pos > CTI->m_nField) {
        singleTypeError(tuple->m_type->m_compoundTypeInfo, "Tuple index out of range. Tuple type:");
    }
    Variable **vars = tuple->m_data;
    return vars[pos - 1];
}

Variable *variableGetTupleFieldFromID(Variable *tuple, int64_t id) {
    int64_t pos = tupleTypeResolveId(tuple->m_type->m_compoundTypeInfo, id);
    if (pos == -1) {
        singleTypeError(tuple->m_type->m_compoundTypeInfo, "Cannot resolve tuple field id at runtime. Tuple type:");
    }
    return variableGetTupleField(tuple, pos);
}

void variableDestructor(Variable *this) {
#ifdef DEBUG_PRINT
    if (!reentry) {
        fprintf(stderr, "(destruct var %p)", (void *) this);
        variableDebugPrint(this);
        fprintf(stderr, "\n");
    }
#endif
    TypeID id = this->m_type->m_typeId;

    if (id == TYPEID_NDARRAY) {
        variableNDArrayDestructor(this);
        return;
    } else if (id == TYPEID_TUPLE) {
        TupleType *CTI = this->m_type->m_compoundTypeInfo;
        tupleTypeFreeData(CTI, this->m_data);
    }

    switch (id) {
        case TYPEID_STREAM_IN:
        case TYPEID_STREAM_OUT:
        case TYPEID_TUPLE:
            break;  // m_data is ignored for these types
        case TYPEID_INTERVAL:
            intervalTypeFreeData(this->m_data);
            break;
        case TYPEID_UNKNOWN:
        case NUM_TYPE_IDS:
        default:
            singleTypeError(this->m_type, "Unexpected type on variableDestructor() call: ");
            break;
    }

    typeDestructThenFree(this->m_type);
}

void variableDestructThenFree(Variable *this) {
    variableDestructor(this);
#ifdef DEBUG_PRINT
    if (!reentry)
        fprintf(stderr, "(llvm side free var %p)\n", (void *)this);
#endif
    free(this);
}

void variableDestructThenFreeImpl(Variable *this) {
    variableDestructor(this);
#ifdef DEBUG_PRINT
    if (!reentry)
        fprintf(stderr, "(free var %p)\n", (void *)this);
#endif
    free(this);
}

bool variableIsIntegerInterval(Variable *this) { return typeIsIntegerInterval(this->m_type); }
bool variableIsIntegerArray(Variable *this) { return typeIsIntegerVector(this->m_type); }
bool variableIsDomainExprCompatible(Variable *this) { return typeIsDomainExprCompatible(this->m_type); }
int64_t variableGetLength(Variable *this) {
    switch(this->m_type->m_typeId) {
        case TYPEID_NDARRAY: {
            return arrayTypeGetTotalLength(this->m_type->m_compoundTypeInfo);
        } break;
        case TYPEID_INTERVAL: {
            int32_t *interval = this->m_data;
            return interval[1] - interval[0] + 1;
        } break;
        default: {
            singleTypeError(this->m_type, "Invalid type for variableGetLength!");
        } break;
    }
}

void variableInitFromArrayElementAtIndex(Variable *this, Variable *arr, int64_t idx) {
    int64_t len = variableGetLength(arr);
    if (idx < 0 || idx >= len) {
        fprintf(stderr, "Variable integer array/interval index %ld out of range [%d, %ld)!", idx, 0, len);
        errorAndExit("Index out of range!");
    }
    switch(arr->m_type->m_typeId) {
        case TYPEID_NDARRAY: {
            if (variableGetNDim(arr) != 1)
                errorAndExit("Unexpected array dimension!");
            ArrayType *arrCTI = arr->m_type->m_compoundTypeInfo;
            void *elementPtr = variableNDArrayGet(arr, idx);
            variableInitFromNDArray(this, false, arrCTI->m_elementTypeID, 0, NULL, elementPtr, true);
        } break;
        case TYPEID_INTERVAL:
            variableInitFromIntegerScalar(this, intervalTypeGetElementAtIndex(arr->m_data, idx));
            break;
        default: {
            singleTypeError(arr->m_type, "Invalid type for variableInitFromArrayElementAtIndex!");
        } break;
    }
}

void variableInitFromIntegerArrayElementAtIndex(Variable *this, Variable *arr, int64_t idx) {
    int32_t value = variableGetIntegerElementAtIndex(arr, idx);
    variableInitFromIntegerScalar(this, value);
    variableSetIsBlockScoped(this, true);  //justin propose change
}

int32_t variableGetIntegerElementAtIndex(Variable *this, int64_t idx) {
    int64_t len = variableGetLength(this);
    if (idx < 0 || idx >= len) {
        fprintf(stderr, "Variable integer array/interval index %ld out of range [%d, %ld)!", idx, 0, len);
        errorAndExit("Index out of range!");
    }
    switch(this->m_type->m_typeId) {
        case TYPEID_NDARRAY:
            return arrayGetIntegerValue(this->m_data, idx);
        case TYPEID_INTERVAL:
            return intervalTypeGetElementAtIndex(this->m_data, idx);
        default: {
            singleTypeError(this->m_type, "Invalid type for variableGetIntegerElementAtIndex!");
        } break;
    }
}

void variableSetIsBlockScoped(Variable *this, bool isBlockScoped) {
    this->m_isBlockScoped = isBlockScoped;
    // if this is tuple, then all its children will also need to change
    if (this->m_type->m_typeId == TYPEID_TUPLE) {
        TupleType *CTI = this->m_type->m_compoundTypeInfo;
        Variable **vars = this->m_data;
        for (int64_t i = 0; i < CTI->m_nField; i++) {
            vars[i]->m_isBlockScoped = isBlockScoped;
        }
    }
}

Variable *variableConvertLiteralAndRefToConcreteArray(Variable *arr) {
    if (typeIsEmptyArray(arr->m_type))
        return NULL;

    Variable *result = NULL;
    if (typeIsMixedArray(arr->m_type)) {
        result = variableMalloc();
        variableInitFromMixedArrayPromoteToSameType(result, arr);
    } else if (variableGetIndexRefTypeID(arr) != NDARRAY_INDEX_REF_NOT_A_REF) {
        result = variableMalloc();
#ifdef DEBUG_PRINT
        fprintf(stderr, "calling refToValue from convertToConcreteArray\n");
#endif
        variableInitFromNDArrayIndexRefToValue(result, arr);
    }
    return result;
}

int32_t variableGetIntegerValue(Variable *this) {
    Variable *intVar = variableMalloc();
    variableInitFromPCADPToIntegerScalar(intVar, this, &pcadpCastConfig);
    int32_t result = *(int32_t *)intVar->m_data;
#ifdef DEBUG_PRINT
    fprintf(stderr, "daf#22\n");
#endif
    variableDestructThenFreeImpl(intVar);
    return result;
}

bool variableGetBooleanValue(Variable *this) {
    Type *boolTy = typeMalloc();
    typeInitFromArrayType(boolTy, false, ELEMENT_BOOLEAN, 0, NULL);
    Variable *intVar = variableMalloc();
    variableInitFromPromotion(intVar, boolTy, this);
    bool result = *(bool *)intVar->m_data;
#ifdef DEBUG_PRINT
    fprintf(stderr, "daf#23\n");
#endif
    variableDestructThenFreeImpl(intVar);
    typeDestructThenFree(boolTy);
    return result;
}

int64_t variableGetNumFieldInTuple(Variable *this) {
    if (this->m_type->m_typeId != TYPEID_TUPLE) {
        singleTypeError(this->m_type, "The given type is not a tuple: ");
    }
    TupleType *CTI = this->m_type->m_compoundTypeInfo;
    return CTI->m_nField;
}

int8_t variableGetNDim(Variable *this) {
    Type *type = this->m_type;
    switch (type->m_typeId) {
        case TYPEID_NDARRAY:
            return typeGetNDArrayNDims(type);
        case TYPEID_INTERVAL:
            return 1;
        default:
            return DIM_INVALID;
    }
}

void variableEmptyInitFromTypeID(Variable *this, TypeID id) {
    this->m_type = typeMalloc();
    this->m_type->m_compoundTypeInfo = NULL;
    this->m_type->m_typeId = id;
    this->m_data = NULL;
    variableAttrInitHelper(this, -1, this->m_data, false);
}

void variableAttrInitHelper(Variable *this, int64_t fieldPos, void *parent, bool isBlockScoped) {
    this->m_fieldPos = fieldPos;
    this->m_parent = parent;
    variableSetIsBlockScoped(this, isBlockScoped);
}

bool variableAliasWith(Variable *this, Variable *other) {
    // a variable with no data never alias to anything
    if (this->m_parent == NULL || other->m_parent == NULL)
        return false;
    if (this->m_parent == other->m_parent) {
        // -1 means the variable is not a tuple field, thus same parents implies alias
        return this->m_fieldPos == -1 || other->m_fieldPos == -1 || this->m_fieldPos == other->m_fieldPos;
    }
    return false;
}

void variableAssignment(Variable *this, Variable *rhs) {
//  TODO: check the case of assignment to temp reference point to block scoped variable

//    if (!this->m_isBlockScoped)
//        errorAndExit("Attempting to assign to a non-block-scoped variable!");

    Variable *result = variableMalloc();
    variableInitFromAssign(result, this->m_type, rhs);

    NDArrayIndexRefTypeID refTypeID = variableGetIndexRefTypeID(this);
    if (refTypeID == NDARRAY_INDEX_REF_NOT_A_REF) {
        variableDestructor(this);
        variableInitFromMemcpy(this, result);
        variableSetIsBlockScoped(this, true);
    } else {  // index assignment
        if (!typeIsArraySameTypeSameSize(this->m_type, result->m_type))
            singleTypeError(result->m_type, "Incompatible rhs type for index assignment: ");
        int64_t len = variableGetLength(result);
        ArrayType *CTI = this->m_type->m_compoundTypeInfo;
        ElementTypeID eid = CTI->m_elementTypeID;
        for (int64_t i = 0; i < len; i++) {
            void *target = variableNDArrayGet(this, i);
            void *src = variableNDArrayGet(result, i);
            elementAssign(eid, target, src);
        }
    }

#ifdef DEBUG_PRINT
    fprintf(stderr, "daf#24\n");
#endif
    variableDestructThenFreeImpl(result);
}