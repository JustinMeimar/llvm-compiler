#include "RuntimeVariables.h"
#include <stdlib.h>
#include "RuntimeErrors.h"
#include "NDArray.h"
#include <string.h>

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
    } else if (typeIsArrayOrString(targetType) && !config->m_allowUnknownTargetArraySize) {
        ArrayType *CTI = targetType->m_compoundTypeInfo;
        for (int64_t i = 0; i < CTI->m_nDim; i++) {
            if (CTI->m_dims[i] < 0) {
                targetTypeError(targetType, "Attempt to convert empty array into unknown size array:");
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

            if (CTI->m_nDim == 0 || CTI->m_elementTypeID == element_mixed)
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
        if (!config->m_allowUnknownTargetType) {
            targetTypeError(rhsType, "Attempt to convert to unknown. rhsType:");
        }
        variableInitFromMemcpy(this, rhs);
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
        int32_t *interval = intervalTypeMallocDataFromCopy(operand->m_data);
        if (opcode == unary_minus) {
            int32_t r0 = -interval[1];
            int32_t r1 = -interval[0];
            interval[0] = r0;
            interval[1] = r1;
        } else if (opcode == unary_plus) {
        } else {
            targetTypeError(operandType, "Attempt to perform unary not on type: ");
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

void variableInitFromBinaryOp(Variable *this, Variable *op1, Variable *op2, BinOpCode opcode) {
    Type *op1Type = op1->m_type;
    Type *op2Type = op2->m_type;
    if (typeIsStream(op1->m_type) || typeIsStream(op2->m_type) || op1Type->m_typeId == typeid_unknown || op2Type->m_typeId == typeid_unknown) {
        errorAndExit("binary op cannot be performed on stream/unknown object!");
    }
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

    typeDestructor(this->m_type);
    free(this->m_type);
}