#include "RuntimeVariables.h"
#include <stdlib.h>
#include "RuntimeErrors.h"
#include "NDArray.h"
#include "FreeList.h"
#include <string.h>
#include "Literal.h"

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

///------------------------------HELPER FUNCTIONS---------------------------------------------------------------

///------------------------------INTERFACES---------------------------------------------------------------

void variableInitFromPCADP(Variable *this, Type *targetType, Variable *rhs, PCADPConfig *config) {
    // in every possible case, we need to do one of the below two things
    // 1. throw an error when input types do not make sense
    // 2. initialize every field of this fully, including this->m_type and this->m_data

    /// common local variables
    Type *rhsType = rhs->m_type;
    TypeID rhsTypeID = rhsType->m_typeId;
    // targetType
    TypeID targetTypeID = targetType->m_typeId;

    if (rhsTypeID == TYPEID_STREAM_IN || rhsTypeID == TYPEID_STREAM_OUT || !typeIsConcreteType(rhsType)) {
        ///- stream_in/stream_out/unknown/unknown-size-array -> any: invalid rhs type
        targetTypeError(rhsType, "Invalid rhs type:");
    } else if (targetTypeID == TYPEID_STREAM_IN || targetTypeID == TYPEID_STREAM_OUT || targetTypeID == TYPEID_EMPTY_ARRAY) {
        ///- any -> stream_in/stream_out/empty array: invalid target type
        targetTypeError(targetType, "Invalid target type:");
    } else if (typeIsArrayOrString(targetType)) {
        ArrayType *CTI = targetType->m_compoundTypeInfo;
        if (CTI->m_elementTypeID == ELEMENT_MIXED) {
            targetTypeError(targetType, "Attempt to convert into a mixed element array:");
        } else if (!config->m_allowUnknownTargetArraySize && arrayTypeHasUnknownSize(CTI)) {
            targetTypeError(targetType, "Attempt to convert into unknown size array:");
        }
    } else if (config->m_isCast && (typeIsArrayNull(rhsType) || typeIsArrayIdentity(rhsType))) {
        targetTypeError(rhsType, "Null or identity not allowed to be rhs:");
    }

    ///- empty array -> any
    if (rhsTypeID == TYPEID_EMPTY_ARRAY) {
        // empty array -> ndarray/string
        if (typeIsArrayOrString(targetType)) {
            this->m_type = typeMalloc();
            typeInitFromCopy(this->m_type, targetType);  // same type as target type
            ArrayType *CTI = this->m_type->m_compoundTypeInfo;

            if (CTI->m_nDim == 0)
                targetTypeError(targetType, "Attempt to convert empty array into:");

            // otherwise we do conversion to null/identity/basic type vector/string/matrix, of unspecified/unknown/any size
            for (int64_t i = 0; i < CTI->m_nDim; i++) {
                int64_t dim = CTI->m_dims[i];
                if (dim > 0 && config->m_rhsSizeRestriction == vectovec_rhs_must_be_same_size) {
                    targetTypeError(targetType, "Attempt to convert empty array into a >= 1 size array:");
                }
                CTI->m_dims[i] = dim >= 0 ? dim : 0;
            }
            this->m_data = arrayMallocFromNull(CTI->m_elementTypeID, arrayTypeGetTotalLength(CTI));
        } else {
            targetTypeError(targetType, "Attempt to convert empty array into:");
        }
        this->m_fieldPos = -1;
        this->m_parent = this->m_data;
        return;
    }

    /// any -> interval
    if (targetTypeID == TYPEID_INTERVAL) {
        // ndarray/interval -> interval
        if (rhsTypeID == TYPEID_NDARRAY) {
            // only possible case is when the ndarray is a scalar null/identity
            ArrayType *CTI = rhsType->m_compoundTypeInfo;
            ElementTypeID eid = CTI->m_elementTypeID;
            if (CTI->m_nDim != 0 || (eid != ELEMENT_NULL && eid != ELEMENT_IDENTITY))
                targetTypeError(rhsType, "Attempt to convert to interval from:");
            this->m_type = typeMalloc();
            typeInitFromIntervalType(this->m_type, INTEGER_BASE_INTERVAL);
            if (eid == ELEMENT_NULL) {
                variableInitFromNull(this, NULL);
            } else if (eid == ELEMENT_IDENTITY) {
                variableInitFromIdentity(this, NULL);
            } else {
                targetTypeError(rhsType, "Attempt to convert to interval from:");
            }
        } else if (rhsTypeID == TYPEID_INTERVAL) {
            // we just copy it
            variableInitFromMemcpy(this, rhs);
        } else {
            targetTypeError(rhsType, "Attempt to convert to interval from:");
        }
        return;
    }

    /// any -> unknown
    if (targetTypeID == TYPEID_UNKNOWN) {
        if (!config->m_allowUnknownTargetType || typeIsArrayNull(rhsType) || typeIsArrayIdentity(rhsType)) {
            targetTypeError(rhsType, "Attempt to convert to unknown. rhsType:");
        } else if (rhsTypeID == TYPEID_NDARRAY) {
            ArrayType *rhsCTI = rhsType->m_compoundTypeInfo;
            if (rhsCTI->m_elementTypeID == ELEMENT_MIXED) {
                variableInitFromMixedArrayPromoteToSameType(this, rhs);
            } else {
                variableInitFromMemcpy(this, rhs);
            }
        } else {
            variableInitFromMemcpy(this, rhs);
        }
        return;
    }

    /// interval -> ndarray
    if (rhsTypeID == TYPEID_INTERVAL) {
        if (targetTypeID == TYPEID_NDARRAY) {
            // interval -> vector of integer or real
            ArrayType *CTI = targetType->m_compoundTypeInfo;
            ElementTypeID eid = CTI->m_elementTypeID;
            if (CTI->m_nDim != 1)
                targetTypeError(targetType, "Attempt to convert interval to:");
            int64_t arrayLength = CTI->m_dims[0];

            int32_t *interval = rhs->m_data;
            int64_t size = interval[1] - interval[0] + 1;
            if (arrayLength < 0)
                arrayLength = size;  // for unspecified/unknown size we just assume it is the same size
            if (config->m_rhsSizeRestriction == vectovec_rhs_must_be_same_size && arrayLength != size
            || config->m_rhsSizeRestriction == vectovec_rhs_size_must_not_be_greater && size > arrayLength) {
                targetTypeError(targetType, "Attempt to convert interval to:");
            }
            int32_t *vec = arrayMallocFromNull(ELEMENT_INTEGER, arrayLength);

            int64_t resultSize = size < arrayLength ? size : arrayLength;
            for (int64_t i = 0; i < resultSize; i++) {
                vec[i] = interval[0] + (int32_t)i;
            }
            if (eid != ELEMENT_INTEGER) {
                arrayMallocFromCast(eid, ELEMENT_INTEGER, resultSize, vec, &this->m_data);
                free(vec);
            } else {
                this->m_data = vec;
            }
        } else {
            targetTypeError(targetType, "Attempt to convert interval to:");
        }
        this->m_fieldPos = -1;
        this->m_parent = this->m_data;
        return;
    }

    /// tuple -> any
    /// any -> tuple
    if (rhsTypeID == TYPEID_TUPLE || targetTypeID == TYPEID_TUPLE) {
        if (rhsTypeID != TYPEID_TUPLE || targetTypeID != TYPEID_TUPLE) {
            errorAndExit("Tuple can not be converted to/from any other type!");
        }
        // tuple -> tuple
        // we convert variables pair-wise
        TupleType *rhsCTI = rhsType->m_compoundTypeInfo;
        TupleType *targetCTI = targetType->m_compoundTypeInfo;
        if (rhsCTI->m_nField != targetCTI->m_nField) {
            errorAndExit("Tuple can not be converted to/from tuple of different size!");
        }
        this->m_type = typeMalloc();
        typeInitFromCopy(this->m_type, targetType);
        this->m_data = tupleTypeMallocDataFromPCADP(targetCTI, rhs, config);
        this->m_fieldPos = -1;
        this->m_parent = this->m_data;
        return;
    }

    /// array/string -> array/string
    {
        ArrayType *rhsCTI = rhsType->m_compoundTypeInfo;
        int8_t rhsNDim = rhsCTI->m_nDim;
        int64_t *rhsDims = rhsCTI->m_dims;

        this->m_type = typeMalloc();
        typeInitFromCopy(this->m_type, targetType);
        ArrayType *CTI = this->m_type->m_compoundTypeInfo;
        int8_t nDim = CTI->m_nDim;
        int64_t *dims = CTI->m_dims;
        if (!config->m_allowArrToArrDifferentElementTypeConversion) {
            errorAndExit("No array to array conversion allowed");
        } else if (nDim < rhsNDim) {
            errorAndExit("Cannot convert to a lower dimension array!");
        } else if (nDim == 2 && rhsNDim == 1) {
            errorAndExit("Cannot convert from vector to matrix!");
        }

        void (*conversion)(ElementTypeID, ElementTypeID, int64_t, void *, void **)
            = config->m_isCast ? arrayMallocFromCast : arrayMallocFromPromote;
        void *convertedArray;
        conversion(CTI->m_elementTypeID, rhsCTI->m_elementTypeID, arrayTypeGetTotalLength(rhsCTI), rhs->m_data, &convertedArray);

        if (rhsNDim == 0) {
            if (arrayTypeHasUnknownSize(CTI))
                targetTypeError(targetType, "Attempt to convert scalar to unknown size array or string:");
            this->m_data = arrayMallocFromElementValue(CTI->m_elementTypeID, arrayTypeGetTotalLength(CTI),convertedArray);
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
                arrayMallocFromVectorResize(CTI->m_elementTypeID, convertedArray, rhsDims[0], dims[0], &this->m_data);
            } else {
                arrayMallocFromMatrixResize(CTI->m_elementTypeID, rhs->m_data, rhsDims[0], rhsDims[1], dims[0], dims[1], &this->m_data);
            }
        }
        free(convertedArray);

        this->m_fieldPos = -1;
        this->m_parent = this->m_data;
        return;
    }
}

Variable *variableMalloc() {
    return malloc(sizeof(Variable));
}

void variableInitFromMemcpy(Variable *this, Variable *other) {
    this->m_type = typeMalloc();
    typeInitFromCopy(this->m_type, other->m_type);
    TypeID id = this->m_type->m_typeId;
    this->m_data = NULL;
    switch (id) {
        case TYPEID_NDARRAY:
        case TYPEID_STRING: {
            ArrayType *CTI = other->m_type->m_compoundTypeInfo;
            this->m_data = arrayMallocFromMemcpy(CTI->m_elementTypeID, arrayTypeGetTotalLength(CTI), other->m_data);
        } break;
        case TYPEID_TUPLE: {
            TupleType *CTI = other->m_type->m_compoundTypeInfo;
            this->m_data = tupleTypeMallocDataFromCopy(CTI, other->m_data);
        } break;
        case TYPEID_STREAM_IN:
        case TYPEID_STREAM_OUT:
        case TYPEID_EMPTY_ARRAY:
            break;  // m_data is ignored for these types
        case TYPEID_INTERVAL:
            this->m_data = intervalTypeMallocDataFromCopy(other->m_data);
            break;
        case TYPEID_UNKNOWN:
        case NUM_TYPE_IDS:
        default:
            unknownTypeVariableError();
            break;
    }
    this->m_fieldPos = -1;
    this->m_parent = this->m_data;
}

// <type> var = null;
void variableInitFromNull(Variable *this, Type *type) {
    this->m_type = typeMalloc();
    if (type == NULL)
        type = this->m_type;  // use its own type;
    else
        typeInitFromCopy(this->m_type, type);
    TypeID typeid = type->m_typeId;

    if (typeid == TYPEID_NDARRAY || typeid == TYPEID_STRING) {
        ArrayType *CTI = type->m_compoundTypeInfo;
        if (arrayTypeHasUnknownSize(CTI))
            targetTypeError(type, "Attempt to promote null into unknown type: ");
        this->m_data = arrayMallocFromNull(CTI->m_elementTypeID, arrayTypeGetTotalLength(CTI));
    } else if (typeid == TYPEID_INTERVAL) {
        IntervalType *CTI = type->m_compoundTypeInfo;
        if (intervalTypeIsUnspecified(CTI))
            targetTypeError(type, "Attempt to promote null into unknown type: ");
        this->m_data = intervalTypeMallocDataFromNull();
    } else if (typeid == TYPEID_TUPLE) {
        this->m_data = tupleTypeMallocDataFromNull(type->m_compoundTypeInfo);
    }
    this->m_parent = this->m_data;
    this->m_fieldPos = -1;
}

// <type> var = identity;
void variableInitFromIdentity(Variable *this, Type *type) {
    this->m_type = typeMalloc();
    typeInitFromCopy(this->m_type, type);
    TypeID typeid = type->m_typeId;

    if (typeid == TYPEID_NDARRAY || typeid == TYPEID_STRING) {
        ArrayType *CTI = type->m_compoundTypeInfo;
        if (arrayTypeHasUnknownSize(CTI))
            targetTypeError(type, "Attempt to promote identity into unknown type: ");
        this->m_data = arrayMallocFromIdentity(CTI->m_elementTypeID, arrayTypeGetTotalLength(CTI));
    } else if (typeid == TYPEID_INTERVAL) {
        IntervalType *CTI = type->m_compoundTypeInfo;
        if (intervalTypeIsUnspecified(CTI))
            targetTypeError(type, "Attempt to promote null into unknown type: ");
        this->m_data = intervalTypeMallocDataFromIdentity();
    } else if (typeid == TYPEID_TUPLE) {
        this->m_data = tupleTypeMallocDataFromIdentity(type->m_compoundTypeInfo);
    }
    this->m_parent = this->m_data;
    this->m_fieldPos = -1;
}

void variableInitFromUnaryOp(Variable *this, Variable *operand, UnaryOpCode opcode) {
    Type *operandType = operand->m_type;
    if (!typeIsConcreteType(operandType)) {
        targetTypeError(operandType, "Attempt to perform unary op on type that is not a concrete type: ");
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
            targetTypeError(operandType, "Attempt to perform unary 'not' on type: ");
        }
        this->m_data = interval;
    } else if (operandType->m_typeId == TYPEID_NDARRAY) {
        ArrayType *CTI = this->m_type->m_compoundTypeInfo;
        arrayMallocFromUnaryOp(CTI->m_elementTypeID, opcode, operand->m_data,
                               arrayTypeGetTotalLength(CTI), &this->m_data);
    } else {
        targetTypeError(operandType, "Attempt to perform unary operation on type: ");
    }
    this->m_parent = this->m_data;
    this->m_fieldPos = -1;
}

void computeIvlByIntBinop(Variable *this, Variable *op1, Variable *op2, BinOpCode opcode) {
    variableInitFromIntervalStep(this, op1, op2);
}

void computeSameTypeSameSizeArrayArrayBinop(Variable *this, Variable *op1, Variable *op2, BinOpCode opcode) {
    Type *op1Type = op1->m_type;
    ArrayType *op1CTI = op1->m_type->m_compoundTypeInfo;
    int64_t arrSize = arrayTypeGetTotalLength(op1CTI);
    ElementTypeID resultType;
    bool resultCollapseToScalar;
    if (!arrayBinopResultType(op1CTI->m_elementTypeID, opcode, &resultType, &resultCollapseToScalar)) {
        errorAndExit("Cannot perform binop between two array variables!");
    }

    // init type
    this->m_type = typeMalloc();
    if (resultCollapseToScalar) {
        typeInitFromArrayType(this->m_type, TYPEID_NDARRAY, resultType, 0, NULL);
    } else {
        typeInitFromCopy(this->m_type, op1Type);
    }

    // init data
    arrayMallocFromBinOp(resultType, opcode, op1->m_data, arrSize,
                         op2->m_data, arrSize, &this->m_data, NULL);
}

void computeIvlIvlBinop(Variable *this, Variable *op1, Variable *op2, BinOpCode opcode) {
    this->m_type = typeMalloc();
    if (opcode == BINARY_EQ || opcode == BINARY_NE) {
        // result is boolean
        typeInitFromArrayType(this->m_type, TYPEID_NDARRAY, ELEMENT_BOOLEAN, 0, NULL);
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

    variableDestructThenFree(op1Promoted);
    variableDestructThenFree(op2Promoted);

    this->m_fieldPos = -1;
    this->m_parent = this->m_data;
}


// guarantee op1 and op2 can only be spec types
void variableInitFromBinaryOpWithSpecTypes(Variable *this, Variable *op1, Variable *op2, BinOpCode opcode) {
    ///remaining types: empty array, ndarray/string, interval, tuple
    ///remaining opcodes: every op besides concat
    Type *op1Type = op1->m_type;
    Type *op2Type = op2->m_type;

    if (op2Type->m_typeId == TYPEID_EMPTY_ARRAY && typeIsVectorOrString(op1Type)) {
        // return 0 size array of the same element type as op1
        this->m_type = typeMalloc();
        typeInitFromCopy(this->m_type, op1Type);
        ArrayType *CTI = this->m_type->m_compoundTypeInfo;
        CTI->m_dims[0] = 0;

        this->m_data = arrayMallocFromNull(CTI->m_elementTypeID, 0);

        this->m_fieldPos = -1;
        this->m_parent = this->m_data;
        return;
    } else if (op1Type->m_typeId == TYPEID_EMPTY_ARRAY || op2Type->m_typeId == TYPEID_EMPTY_ARRAY) {
        errorAndExit("Unexpected empty array in binary op!");
    }

    if (opcode == BINARY_RANGE_CONSTRUCT) {
        // promote both sides to integer and retrieve the result then create a new interval from it
        variableInitFromIntervalHeadTail(this, op1, op2);
        return;
    }

    switch (opcode) {
        case BINARY_INDEX: {
            // TODO
        } break;
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
                Type *targetType = typeMalloc();
                typeInitFromCopy(targetType, op1Type);
                ArrayType *op1CTI = op1Type->m_compoundTypeInfo;
                ArrayType *op2CTI = op2Type->m_compoundTypeInfo;
                ArrayType *CTI = targetType->m_compoundTypeInfo;
                if (!elementCanBePromotedBetween(op1CTI->m_elementTypeID, op2CTI->m_elementTypeID, &CTI->m_elementTypeID)) {
                    errorAndExit("Cannot promote between interval and vector!");
                }
                binopPromoteComputationAndDispose(this, op1, op2, opcode, targetType, targetType,
                                                  computeSameTypeSameSizeArrayArrayBinop);
                typeDestructThenFree(targetType);
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
                    variableDestructThenFree(temp);
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
            typeInitFromArrayType(intType, TYPEID_NDARRAY, ELEMENT_INTEGER, 0, NULL);

            binopPromoteComputationAndDispose(this, op1, op2, opcode, ivlType, intType, computeIvlByIntBinop);

            typeDestructThenFree(ivlType);
            typeDestructThenFree(intType);
        } break;
        default:
            errorAndExit("unexpected op!"); break;
    }
}

void variableInitFromConcat(Variable *this, Variable *op1, Variable *op2) {
    Type *op1Type = op1->m_type;
    Type *op2Type = op2->m_type;
    if ((!typeIsArrayOrString(op1Type) && op1Type->m_typeId != TYPEID_EMPTY_ARRAY) || typeIsMatrix(op1Type)) {
        targetTypeError(op1Type, "Attempt to concat with type: ");
    } else if (!typeIsArrayOrString(op2Type) && op2Type->m_typeId != TYPEID_EMPTY_ARRAY || typeIsMatrix(op2Type)) {
        targetTypeError(op2Type, "Attempt to concat with type: ");
    }

    // [] || [] -> []
    // [] || A -> A
    // A || [] -> A
    if (op1Type->m_typeId == TYPEID_EMPTY_ARRAY) {
        variableInitFromMemcpy(this, op2);
        return;
    } else if (op2Type->m_typeId == TYPEID_EMPTY_ARRAY) {
        variableInitFromMemcpy(this, op1);
        return;
    }

    if (typeIsScalar(op1Type) && typeIsScalar(op2Type)) {
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
    TypeID tid = TYPEID_NDARRAY;
    if (op1Type->m_typeId == TYPEID_STRING || op2Type->m_typeId == TYPEID_STRING)
        tid = TYPEID_STRING;
    typeInitFromArrayType(this->m_type, tid, resultEID, 1, dims);

    freeListFreeAll(freeList, free);

    this->m_fieldPos = -1;
    this->m_parent = this->m_data;
}

void variableInitFromBinaryOp(Variable *this, Variable *op1, Variable *op2, BinOpCode opcode) {
    // this function is responsible for dealing with null/identity/mixed arrays
    Type *op1Type = op1->m_type;
    Type *op2Type = op2->m_type;
    if (typeIsStream(op1Type) || typeIsStream(op2Type)) {
        errorAndExit("binary op cannot be performed on stream object!");
    } else if (!typeIsConcreteType(op1Type) || !typeIsConcreteType(op2Type)) {
        errorAndExit("binary op can only be performed on concrete type!");
    }
    Variable *pop1 = op1;
    Variable *pop2 = op2;
    // mixed array
    FreeList *freeList = NULL;
    if (typeIsMixedArray(op1Type)) {
        pop1 = variableMalloc();
        variableInitFromMixedArrayPromoteToSameType(pop1, op1);
        freeList = freeListAppend(freeList, pop1);
    }
    if (typeIsMixedArray(op2Type)) {
        pop2 = variableMalloc();
        variableInitFromMixedArrayPromoteToSameType(pop2, op2);
        freeList = freeListAppend(freeList, pop2);
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

    freeListFreeAll(freeList, (void (*)(void *)) variableDestructThenFree);
}

PCADPConfig pcadpParameterConfig = {
        true, true,
        true, false,
        vectovec_rhs_size_must_not_be_greater, false,
        true, false
};
PCADPConfig pcadpCastConfig = {
        false, true,
        false, false,
        vectovec_rhs_size_can_be_any, true,
        true, true
};
PCADPConfig pcadpAssignmentConfig = {
        false, false,
        false, true,
        vectovec_rhs_must_be_same_size, false,
        true, false
};
PCADPConfig pcadpDeclarationConfig = {
        true, true,
        false, false,
        vectovec_rhs_size_must_not_be_greater, false,
        true, false
};
PCADPConfig pcadpPromotionConfig = {
        true, false,
        false, false,
        vectovec_rhs_must_be_same_size, false,
        true, false
};
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

void variableInitFromMixedArrayPromoteToSameType(Variable *this, Variable *mixed) {
    Type *rhsType = mixed->m_type;
    ArrayType *rhsCTI = rhsType->m_compoundTypeInfo;
    int64_t size = arrayTypeGetTotalLength(rhsCTI);
    this->m_type = typeMalloc();
    typeInitFromCopy(this->m_type, rhsType);
    ArrayType *CTI = this->m_type->m_compoundTypeInfo;
    if (!arrayMixedElementCanBePromotedToSameType(mixed->m_data, size,
                                                  &CTI->m_elementTypeID)) {
        targetTypeError(rhsType, "Attempt to convert to homogenous array from:");
    }
    arrayMallocFromPromote(CTI->m_elementTypeID, ELEMENT_MIXED, size, mixed->m_data, &this->m_data);
    this->m_fieldPos = -1;
    this->m_parent = this->m_data;
}

void variableInitFromIntervalHeadTail(Variable *this, Variable *head, Variable *tail) {
    this->m_type = typeMalloc();
    typeInitFromIntervalType(this->m_type, INTEGER_BASE_INTERVAL);
    this->m_data = intervalTypeMallocDataFromHeadTail(variableGetIntegerValue(head), variableGetIntegerValue(tail));
    this->m_fieldPos = -1;
    this->m_parent = this->m_data;
}

void variableInitFromIntervalStep(Variable *this, Variable *ivl, Variable *step) {
    int32_t *interval = ivl->m_data;
    int32_t k = *((int32_t *)step->m_data);
    if (k <= 0) {
        errorAndExit("ivl by step has a step value of 0 or negative!");
    }
    int32_t value = 0;
    int64_t dims[1] = {(interval[1] - interval[0]) / k + 1};

    variableInitFromNDArray(this, TYPEID_NDARRAY, ELEMENT_INTEGER, 1, dims, &value, true);
    int32_t *vec = this->m_data;
    for (int64_t i = 0; i < dims[0]; i++) {
        vec[i] = interval[0] + (int32_t)i * k;
    }
}

void variableInitFromNDArray(Variable *this, TypeID typeID, ElementTypeID eid, int8_t nDim, int64_t *dims,
                             void *value, bool valueIsScalar) {
    this->m_type = typeMalloc();
    typeInitFromArrayType(this->m_type, typeID, ELEMENT_IDENTITY, nDim, dims);
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
    this->m_fieldPos = -1;
    this->m_parent = this->m_data;
}

void variableDestructor(Variable *this) {
    TypeID id = this->m_type->m_typeId;

    if (id == TYPEID_NDARRAY || id == TYPEID_STRING) {
        // destructor does different things depending on the element type
        ArrayType *CTI = this->m_type->m_compoundTypeInfo;
        arrayFree(CTI->m_elementTypeID, this->m_data, arrayTypeGetTotalLength(CTI));
    } else if (id == TYPEID_TUPLE) {
        TupleType *CTI = this->m_type->m_compoundTypeInfo;
        tupleTypeFreeData(CTI, this->m_data);
    }

    switch (id) {
        case TYPEID_NDARRAY:
        case TYPEID_STRING:
        case TYPEID_STREAM_IN:
        case TYPEID_STREAM_OUT:
        case TYPEID_EMPTY_ARRAY:
        case TYPEID_TUPLE:
            break;  // m_data is ignored for these types
        case TYPEID_INTERVAL:
            intervalTypeFreeData(this->m_data);
            break;
        case TYPEID_UNKNOWN:
        case NUM_TYPE_IDS:
        default:
            unknownTypeVariableError();
            break;
    }

    typeDestructThenFree(this->m_type);
}

void variableDestructThenFree(Variable *this) {
    variableDestructor(this);
    free(this);
}

int32_t variableGetIntegerValue(Variable *this) {
    Type *intTy = typeMalloc();
    typeInitFromArrayType(intTy, TYPEID_NDARRAY, ELEMENT_INTEGER, 0, NULL);
    Variable *intVar = variableMalloc();
    variableInitFromPromotion(intVar, intTy, this);
    int32_t result = *(int32_t *)intVar->m_data;
    variableDestructThenFree(intVar);
    typeDestructThenFree(intTy);
    return result;
}

void variableEmptyInitFromTypeID(Variable *this, TypeID id) {
    this->m_type = typeMalloc();
    this->m_type->m_compoundTypeInfo = NULL;
    this->m_type->m_typeId = id;
    this->m_data = NULL;
    this->m_fieldPos = -1;
    this->m_parent = this->m_data;
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
    Variable *result = variableMalloc();
    variableInitFromAssign(result, this->m_type, rhs);

    // TODO: reference assignment for array and tuple
    variableDestructor(this);
    variableInitFromMemcpy(this, result);

    variableDestructThenFree(result);
}