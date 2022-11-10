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

    if (rhsTypeID == typeid_stream_in || rhsTypeID == typeid_stream_out || rhsTypeID == typeid_unknown) {
        ///- stream_in/stream_out/unknown -> any: invalid rhs type
        targetTypeError(rhsType, "Invalid rhs type:");
    } else if (targetTypeID == typeid_stream_in || targetTypeID == typeid_stream_out || targetTypeID == typeid_empty_array) {
        ///- any -> stream_in/stream_out/empty array: invalid target type
        targetTypeError(targetType, "Invalid target type:");
    } else if (typeIsArrayOrString(targetType)) {
        ArrayType *CTI = targetType->m_compoundTypeInfo;
        if (CTI->m_elementTypeID == element_mixed) {
            targetTypeError(targetType, "Attempt to convert into a mixed element array:");
        } else if (!config->m_allowUnknownTargetArraySize) {
            for (int64_t i = 0; i < CTI->m_nDim; i++) {
                if (CTI->m_dims[i] < 0) {
                    targetTypeError(targetType, "Attempt to convert into unknown size array:");
                }
            }
        }
    } else if (config->m_isCast && (typeIsArrayNull(rhsType) || typeIsArrayIdentity(rhsType))) {
        targetTypeError(rhsType, "Null or identity not allowed to be rhs:");
    } else if (typeIsArrayOrString(rhsType)) {
        if (arrayTypeHasUnknownSize(rhsType->m_compoundTypeInfo))
            targetTypeError(rhsType, "RHS type has unknown size:");
    }

    ///- empty array -> any
    if (rhsTypeID == typeid_empty_array) {
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
    if (targetTypeID == typeid_interval) {
        // ndarray/interval -> interval
        if (rhsTypeID == typeid_ndarray) {
            // only possible case is when the ndarray is a scalar null/identity
            ArrayType *CTI = rhsType->m_compoundTypeInfo;
            ElementTypeID eid = CTI->m_elementTypeID;
            if (CTI->m_nDim != 0 || (eid != element_null && eid != element_identity))
                targetTypeError(rhsType, "Attempt to convert to interval from:");
            this->m_type = typeMalloc();
            typeInitFromIntervalType(this->m_type);
            if (eid == element_null) {
                variableInitFromNull(this, NULL);
            } else if (eid == element_identity) {
                variableInitFromIdentity(this, NULL);
            } else {
                targetTypeError(rhsType, "Attempt to convert to interval from:");
            }
        } else if (rhsTypeID == typeid_interval) {
            // we just copy it
            variableInitFromMemcpy(this, rhs);
        } else {
            targetTypeError(rhsType, "Attempt to convert to interval from:");
        }
        return;
    }

    /// any -> unknown
    if (targetTypeID == typeid_unknown) {
        if (!config->m_allowUnknownTargetType || typeIsArrayNull(rhsType) || typeIsArrayIdentity(rhsType)) {
            targetTypeError(rhsType, "Attempt to convert to unknown. rhsType:");
        } else if (rhsTypeID == typeid_ndarray) {
            ArrayType *rhsCTI = rhsType->m_compoundTypeInfo;
            if (rhsCTI->m_elementTypeID == element_mixed) {
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
    if (rhsTypeID == typeid_interval) {
        if (targetTypeID == typeid_ndarray) {
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
            int32_t *vec = arrayMallocFromNull(element_integer, arrayLength);

            int64_t resultSize = size < arrayLength ? size : arrayLength;
            for (int64_t i = 0; i < resultSize; i++) {
                vec[i] = interval[0] + (int32_t)i;
            }
            if (eid != element_integer) {
                arrayMallocFromCast(eid, element_integer, resultSize, vec, &this->m_data);
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
    if (rhsTypeID == typeid_tuple || targetTypeID == typeid_tuple) {
        if (rhsTypeID != typeid_tuple || targetTypeID != typeid_tuple) {
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
            if (config->m_rhsSizeRestriction < arrayTypeMinimumConpatibleRestriction(rhsCTI, CTI)) {
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
        case typeid_ndarray:
        case typeid_string: {
            ArrayType *CTI = other->m_type->m_compoundTypeInfo;
            this->m_data = arrayMallocFromMemcpy(CTI->m_elementTypeID, arrayTypeGetTotalLength(CTI), other->m_data);
        } break;
        case typeid_tuple: {
            TupleType *CTI = other->m_type->m_compoundTypeInfo;
            this->m_data = tupleTypeMallocDataFromCopy(CTI, other->m_data);
        } break;
        case typeid_stream_in:
        case typeid_stream_out:
        case typeid_empty_array:
            break;  // m_data is ignored for these types
        case typeid_interval:
            this->m_data = intervalTypeMallocDataFromCopy(other->m_data);
            break;
        case typeid_unknown:
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

    if (typeid == typeid_ndarray || typeid == typeid_string) {
        ArrayType *CTI = type->m_compoundTypeInfo;
        if (arrayTypeHasUnknownSize(CTI))
            targetTypeError(type, "Attempt to promote null into unknown type: ");
        this->m_data = arrayMallocFromNull(CTI->m_elementTypeID, arrayTypeGetTotalLength(CTI));
    } else if (typeid == typeid_interval) {
        this->m_data = intervalTypeMallocDataFromNull();
    } else if (typeid == typeid_tuple) {
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

    if (typeid == typeid_ndarray || typeid == typeid_string) {
        ArrayType *CTI = type->m_compoundTypeInfo;
        if (arrayTypeHasUnknownSize(CTI))
            targetTypeError(type, "Attempt to promote identity into unknown type: ");
        this->m_data = arrayMallocFromIdentity(CTI->m_elementTypeID, arrayTypeGetTotalLength(CTI));
    } else if (typeid == typeid_interval) {
        this->m_data = intervalTypeMallocDataFromIdentity();
    } else if (typeid == typeid_tuple) {
        this->m_data = tupleTypeMallocDataFromIdentity(type->m_compoundTypeInfo);
    }
    this->m_parent = this->m_data;
    this->m_fieldPos = -1;
}

void variableInitFromUnaryOp(Variable *this, Variable *operand, UnaryOpCode opcode) {
    this->m_type = typeMalloc();
    Type *operandType = operand->m_type;
    typeInitFromCopy(this->m_type, operandType);

    if (operandType->m_typeId == typeid_interval) {
        // +ivl or -ivl
        int32_t *interval = intervalTypeMallocDataFromNull();
        if (opcode == unary_minus) {
            intervalTypeUnaryMinus(interval, operand->m_data);
        } else if (opcode == unary_plus) {
            intervalTypeUnaryPlus(interval, operand->m_data);
        } else {
            targetTypeError(operandType, "Attempt to perform unary 'not' on type: ");
        }
        this->m_data = interval;
    } else if (operandType->m_typeId == typeid_ndarray) {
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
        errorAndExit("Cannot perform binop between two arrays variables!");
    }

    // init type
    this->m_type = typeMalloc();
    if (resultCollapseToScalar) {
        typeInitFromArrayType(this->m_type, resultType, 0, NULL);
    } else {
        typeInitFromCopy(this->m_type, op1Type);
    }

    // init data
    arrayMallocFromBinOp(resultType, opcode, op1->m_data, arrSize,
                         op2->m_data, arrSize, &this->m_data, NULL);
}

void computeIvlIvlBinop(Variable *this, Variable *op1, Variable *op2, BinOpCode opcode) {
    this->m_type = typeMalloc();
    if (opcode == binary_eq || opcode == binary_ne) {
        // result is boolean
        typeInitFromArrayType(this->m_type, element_boolean, 0, NULL);
        this->m_data = arrayMallocFromNull(element_boolean, 1);
    } else {
        typeInitFromIntervalType(this->m_type);
        this->m_data = intervalTypeMallocDataFromNull();
    }
    switch(opcode) {
        case binary_multiply:
            intervalTypeBinaryMultiply(this->m_data, op1->m_data, op2->m_data); break;
        case binary_plus:
            intervalTypeBinaryPlus(this->m_data, op1->m_data, op2->m_data); break;
        case binary_minus:
            intervalTypeBinaryMinus(this->m_data, op1->m_data, op2->m_data); break;
        case binary_eq:
            intervalTypeBinaryEq(this->m_data, op1->m_data, op2->m_data); break;
        case binary_ne:
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

    if (op2Type->m_typeId == typeid_empty_array && typeIsVectorOrString(op1Type)) {
        // return 0 size array of the same element type as op1
        this->m_type = typeMalloc();
        typeInitFromCopy(this->m_type, op1Type);
        ArrayType *CTI = this->m_type->m_compoundTypeInfo;
        CTI->m_dims[0] = 0;

        this->m_data = arrayMallocFromNull(CTI->m_elementTypeID, 0);

        this->m_fieldPos = -1;
        this->m_parent = this->m_data;
        return;
    } else if (op1Type->m_typeId == typeid_empty_array || op2Type->m_typeId == typeid_empty_array) {
        errorAndExit("Unexpected empty array in binary op!");
    }

    if (opcode == binary_range_construct) {
        // promote both sides to integer and retrieve the result then create a new interval from it
        variableInitFromIntervalHeadTail(this, op1, op2);
        return;
    }

    switch (opcode) {
        case binary_index: {
            // TODO
        } break;
        case binary_exponent:
        case binary_multiply:
        case binary_divide:
        case binary_remainder:
        case binary_dot_product:
        case binary_plus:
        case binary_minus:
        case binary_lt:
        case binary_bt:
        case binary_leq:
        case binary_beq:
        case binary_eq:
        case binary_ne:
        case binary_and:
        case binary_or:
        case binary_xor: {  /// same type same size
            /// interval op interval
            if (op1Type->m_typeId == typeid_interval && !typeIsVectorOrString(op2Type)
                || op2Type->m_typeId == typeid_interval && !typeIsVectorOrString(op1Type)) {
                Type *ivlType = typeMalloc();
                typeInitFromIntervalType(ivlType);
                binopPromoteComputationAndDispose(this, op1, op2, opcode, ivlType, ivlType,
                                                  computeIvlIvlBinop);
                typeDestructThenFree(ivlType);
            } else if (op1Type->m_typeId == typeid_interval || op2Type->m_typeId == typeid_interval) {
                Variable *vec = typeIsArrayOrString(op1Type) ? op1 : op2;
                Type *targetType = typeMalloc();
                typeInitFromCopy(targetType, vec->m_type);
                // in this case we may have vector null element promote to interval element
                ArrayType *CTI = targetType->m_compoundTypeInfo;
                if (!elementCanBePromotedBetween(element_integer, CTI->m_elementTypeID, &CTI->m_elementTypeID)) {
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
            } else if (op1Type->m_typeId == typeid_tuple || op2Type->m_typeId == typeid_tuple) {
                // must both be tuple now because we have ruled out possibility of null/identity before
                TupleType *op1CTI = op1Type->m_compoundTypeInfo;
                TupleType *op2CTI = op2Type->m_compoundTypeInfo;
                Variable **op1Vars = op1->m_data;
                Variable **op2Vars = op2->m_data;

                bool result = true;
                for (int64_t i = 0; i < op1CTI->m_nField; i++) {
                    Variable *temp = variableMalloc();
                    variableInitFromBinaryOp(temp, op1Vars[i], op2Vars[i], binary_eq);
                    bool *resultBool = temp->m_data;
                    result = result && *resultBool;
                    variableDestructThenFree(temp);
                }
                if (opcode == binary_ne)
                    result = !result;
                variableInitFromBooleanScalar(this, result);
            } else {
                errorAndExit("Unexpected type on binary operation!");
            }
        } break;
        case binary_by: {
            Type *ivlType = typeMalloc();
            typeInitFromIntervalType(ivlType);
            Type *intType = typeMalloc();
            typeInitFromArrayType(intType, element_integer, 0, NULL);

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
    if ((!typeIsArrayOrString(op1Type) && op1Type->m_typeId != typeid_empty_array) || typeIsMatrix(op1Type)) {
        targetTypeError(op1Type, "Attempt to concat with type: ");
    } else if (!typeIsArrayOrString(op2Type) && op2Type->m_typeId != typeid_empty_array || typeIsMatrix(op2Type)) {
        targetTypeError(op2Type, "Attempt to concat with type: ");
    }

    // [] || [] -> []
    // [] || A -> A
    // A || [] -> A
    if (op1Type->m_typeId == typeid_empty_array) {
        variableInitFromMemcpy(this, op2);
        return;
    } else if (op2Type->m_typeId == typeid_empty_array) {
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

    arrayMallocFromBinOp(resultEID, binary_concat,
                         pop1Data, arr1Length,
                         pop2Data, arr2Length,
                         &this->m_data, NULL);
    this->m_type = typeMalloc();
    int64_t dims[1] = {arr1Length + arr2Length};
    typeInitFromArrayType(this->m_type, resultEID, 1, dims);
    // if one of the two operands is string then the result is string
    if (op1Type->m_typeId == typeid_string || op2Type->m_typeId == typeid_string)
        this->m_type->m_typeId = typeid_string;

    freeListFreeAll(freeList, free);

    this->m_fieldPos = -1;
    this->m_parent = this->m_data;
}

void variableInitFromBinaryOp(Variable *this, Variable *op1, Variable *op2, BinOpCode opcode) {
    // this function is responsible for dealing with null/identity/mixed arrays
    Type *op1Type = op1->m_type;
    Type *op2Type = op2->m_type;
    if (typeIsStream(op1->m_type) || typeIsStream(op2->m_type) || op1Type->m_typeId == typeid_unknown || op2Type->m_typeId == typeid_unknown) {
        errorAndExit("binary op cannot be performed on stream/unknown object!");
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
    if (opcode == binary_concat) {
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
    arrayMallocFromPromote(CTI->m_elementTypeID, element_mixed, size, mixed->m_data, &this->m_data);
    this->m_fieldPos = -1;
    this->m_parent = this->m_data;
}

void computeIntervalConstructBinop(Variable *this, Variable *head, Variable *tail, BinOpCode opcode) {
    int *headValue = head->m_data;
    int *tailValue = tail->m_data;
    this->m_type = typeMalloc();
    typeInitFromIntervalType(this->m_type);
    this->m_data = intervalTypeMallocDataFromHeadTail(*headValue, *tailValue);
}

void variableInitFromIntervalHeadTail(Variable *this, Variable *head, Variable *tail) {
    Type *intType = typeMalloc();
    typeInitFromArrayType(intType, element_integer, 0, NULL);
    binopPromoteComputationAndDispose(this, head, tail, binary_range_construct,
                                      intType, intType, computeIntervalConstructBinop);
    typeDestructThenFree(intType);
}

void variableInitFromIntervalStep(Variable *this, Variable *ivl, Variable *step) {
    int32_t *interval = ivl->m_data;
    int32_t k = *((int32_t *)step->m_data);
    if (k <= 0) {
        errorAndExit("ivl by step has a step value of 0 or negative!");
    }
    int8_t nDim = 1;
    int64_t dims[1] = {(interval[1] - interval[0]) / k + 1};

    this->m_type = typeMalloc();
    typeInitFromArrayType(this->m_type, element_integer, nDim, dims);
    int32_t *vec = arrayMallocFromNull(element_integer, dims[0]);
    for (int64_t i = 0; i < dims[0]; i++) {
        vec[i] = interval[0] + (int32_t)i * k;
    }
    this->m_data = vec;
    this->m_parent = this->m_data;
    this->m_fieldPos = -1;
}

void variableDestructor(Variable *this) {
    TypeID id = this->m_type->m_typeId;

    if (id == typeid_ndarray || id == typeid_string) {
        // destructor does different things depending on the element type
        ArrayType *CTI = this->m_type->m_compoundTypeInfo;
        arrayFree(CTI->m_elementTypeID, this->m_data, arrayTypeGetTotalLength(CTI));
    } else if (id == typeid_tuple) {
        TupleType *CTI = this->m_type->m_compoundTypeInfo;
        tupleTypeFreeData(CTI, this->m_data);
    }

    switch (id) {
        case typeid_ndarray:
        case typeid_string:
        case typeid_stream_in:
        case typeid_stream_out:
        case typeid_empty_array:
        case typeid_tuple:
            break;  // m_data is ignored for these types
        case typeid_interval:
            intervalTypeFreeData(this->m_data);
            break;
        case typeid_unknown:
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