#include "BuiltInFunctions.h"
#include "VariableStdio.h"
#include "RuntimeTypes.h"
#include "Literal.h"
#include "RuntimeErrors.h"
#include "NDArrayVariable.h"

Variable *BuiltInStreamState() {
    Variable *state = variableMalloc();
    variableInitFromIntegerScalar(state, global_stream_state);
    return state;
}

Variable *BuiltInLength(Variable *vec) {
    Variable *length = variableMalloc();
    int8_t nDim = variableGetNDim(vec);
    if (nDim != DIM_UNSPECIFIED && nDim != 1) {
        errorAndExit("Invalid dimension of vector passed in when calling built-in length() function!");
    }
    variableInitFromIntegerScalar(length, (int32_t)variableGetLength(vec));
    return length;
}

Variable *BuiltInReverse(Variable *vec) {
    // convert literal and ref

    Variable *result = variableMalloc();
    int8_t nDim = variableGetNDim(vec);
    if (nDim != DIM_UNSPECIFIED && nDim != 1) {
        errorAndExit("Invalid dimension of vector passed in when calling built-in reverse() function!");
    } else if (nDim == DIM_UNSPECIFIED) {
        variableInitFromEmptyArray(result);
    } else {  // vec is a 1d vector or integer interval
        switch (vec->m_type->m_typeId) {
            case TYPEID_NDARRAY: {
                ArrayType *vecCTI = vec->m_type->m_compoundTypeInfo;
                ElementTypeID eid = vecCTI->m_elementTypeID;
                Variable *pop = variableConvertLiteralAndRefToConcreteArray(vec);
                pop = pop ? pop : vec;

                variableInitFromMemcpy(result, pop);
                int64_t len = variableGetLength(pop);
                for (int64_t i = 0; i < len; i++) {
                    void *src = variableNDArrayGet(pop, i);
                    void *target = variableNDArrayGet(result, len - i - 1);
                    elementAssign(eid, target, src);
                }

                if (pop != vec) {
#ifdef DEBUG_PRINT
                    fprintf(stderr, "daf#1\n");
#endif
                    variableDestructThenFreeImpl(pop);
                }
            }
                break;
            case TYPEID_INTERVAL: {
                variableInitFromPCADPToIntegerVector(result, vec, &pcadpPromotionConfig);
                int64_t len = variableGetLength(vec);
                for (int64_t i = 0; i < len; i++) {
                    int32_t *arr = result->m_data;
                    arr[len - i - 1] = intervalTypeGetElementAtIndex(vec->m_data, i);
                }
            }
                break;
            default:
                singleTypeError(vec->m_type, "Invalid type to reverse: ");
        }
    }

    return result;
}

Variable *BuiltInRows(Variable *mat) {
    Variable *result = variableMalloc();
    int8_t nDim = variableGetNDim(mat);
    if (nDim != DIM_UNSPECIFIED && nDim != 2) {
        errorAndExit("Invalid dimension of matrix passed in when calling built-in rows() function!");
    } else if (nDim == DIM_UNSPECIFIED) {
        variableInitFromIntegerScalar(result, 0);
    } else {
        ArrayType *CTI = mat->m_type->m_compoundTypeInfo;
        variableInitFromIntegerScalar(result, (int32_t)CTI->m_dims[0]);
    }
    return result;
}

Variable *BuiltInColumns(Variable *mat) {
    Variable *result = variableMalloc();
    int8_t nDim = variableGetNDim(mat);
    if (nDim != DIM_UNSPECIFIED && nDim != 2) {
        errorAndExit("Invalid dimension of matrix passed in when calling built-in columns() function!");
    } else if (nDim == DIM_UNSPECIFIED) {
        variableInitFromIntegerScalar(result, 0);
    } else {
        ArrayType *CTI = mat->m_type->m_compoundTypeInfo;
        variableInitFromIntegerScalar(result, (int32_t)CTI->m_dims[1]);
    }
    return result;
}