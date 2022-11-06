#include "NDArray.h"
#include "RuntimeErrors.h"
#include "math.h"
#include "string.h"

bool elementIsMixedType(ElementTypeID id) {
    return id == element_mixed;
}

bool elementIsNullIdentity(ElementTypeID id) {
    return id == element_null || id == element_identity;
}

bool elementIsBasicType(ElementTypeID id) {
    return id == element_boolean || id == element_character || id == element_integer || id == element_real;
}

int64_t elementGetSize(ElementTypeID id) {
    switch (id) {
        case element_real:
            return sizeof(float);
        case element_integer:
            return sizeof(int32_t);
        case element_character:
            return sizeof(int8_t);
        case element_boolean:
            return sizeof(bool);
        case element_mixed:
            return sizeof(void *);
        default:
            break;
    }
    return 0;
}

bool elementCanBeCastedFrom(ElementTypeID to, ElementTypeID from) {
    if (elementIsBasicType(to) && elementIsBasicType(from)) {
        if (from == element_real && (to == element_boolean || to == element_character))
            return false;
        return true;
    }
    return false;
}

bool elementCanBePromotedFrom(ElementTypeID to, ElementTypeID from) {
    if (elementIsMixedType(to) || elementIsMixedType(from)) {
        return false;
    }
    if (from == to)
        return true;

    // from and to are different types
    if (elementIsBasicType(to))
        return elementIsNullIdentity(from) || (from == element_integer && to == element_real);
}

bool elementCanBePromotedBetween(ElementTypeID op1, ElementTypeID op2, ElementTypeID *resultType) {
    if (elementCanBePromotedFrom(op1, op2)) {
        *resultType = op1;
        return true;
    } else if (elementCanBePromotedFrom(op2, op1)) {
        *resultType = op2;
        return true;
    }
    return false;
}

bool elementCanUseUnaryOp(ElementTypeID id, UnaryOpCode opcode) {
    if (!elementIsBasicType(id) || id == element_character)
        return false;
    if (id == element_boolean) {
        return opcode == unary_not;
    } else {
        // integer and real
        return opcode == unary_plus || opcode == unary_minus;
    }
}

bool elementBinOpResultType(ElementTypeID id, BinOpCode opcode, ElementTypeID *resultType) {
    // first filter the ops that are not always between two elements of same type
    // or not returning an element
    switch(opcode) {
        case binary_index:
        case binary_range_construct:
        case binary_dot_product:
        case binary_concat:
            return false;
        default:
            break;
    }
    // no character, null, identity or mixed type can be used in binop without some transformation e.g. promotion
    if (!elementIsBasicType(id) || id == element_character)
        return false;
    if (id == element_boolean) {
        switch(opcode) {
            case binary_eq:
            case binary_ne:
            case binary_and:
            case binary_or:
            case binary_xor:
                *resultType = element_boolean;
                return true;
            default:
                return false;
        }
    } else {
        // integer and real
        switch(opcode) {
            case binary_eq:
            case binary_ne:
            case binary_lt:
            case binary_bt:
            case binary_leq:
            case binary_beq:
                *resultType = element_boolean;
                return true;
            case binary_exponent:
            case binary_multiply:
            case binary_divide:
            case binary_remainder:
            case binary_plus:
            case binary_minus:
                *resultType = id;  // same as the original element type
                return true;
            default:
                return false;
        }
    }
}

void elementMallocFromPromotion(ElementTypeID resultID, ElementTypeID srcID, void *src, void **result) {
    if (!elementCanBePromotedFrom(resultID, srcID)) {
        errorAndExit("Attempt to promote to an invalid element type!");
    }
    if (resultID == srcID) {
        elementMallocFromAssignment(resultID, src, result);  // no promotion needed, just copy the element
        return;
    }

    if (elementIsBasicType(resultID)) {
        if (srcID == element_null)
            arrayMallocFromNull(resultID, 1);
        else if (srcID == element_identity)
            arrayMallocFromIdentity(resultID, 1);
        else if (srcID == element_integer && resultID == element_real) {  // the only remaining case is promoting from integer to real
            // TODO: Check if the promotion satisfy spec
            *result = arrayMallocFromRealValue(1, (float)*((int32_t *)src));
        } else {
            errorAndExit("This should not happen!");
        }
    } else {
        errorAndExit("This should not happen!");
    }
}

int8_t integerToCharacter(int32_t integer) {
    uint32_t unsigned_integer = (uint32_t)integer;
    uint8_t truncated_integer = (uint8_t)unsigned_integer;
    return (int8_t) truncated_integer;
}

void elementMallocFromCast(ElementTypeID resultID, ElementTypeID srcID, void *src, void **result) {
    // only basic types can be cast from/to
    if (!elementCanBeCastedFrom(resultID, srcID)) {
        errorAndExit("Attempt to cast to an invalid element type!");
    }
    if (resultID == srcID) {
        elementMallocFromAssignment(resultID, src, result);
        return;
    }
    if (resultID == element_boolean) {
        bool *resultBool = malloc(sizeof(bool));
        *result = resultBool;
        switch (srcID) {
            case element_character:
                *resultBool = *((int8_t *)src) == 0; break;
            case element_integer:
                *resultBool = *((int32_t *)src) == 0; break;
            case element_real:
                errorAndExit("Attempt to cast real to boolean!"); break;
            default: errorAndExit("This should not happen!"); break;
        }
    } else if (resultID == element_character) {
        int8_t *resultChar = malloc(sizeof(int8_t));
        *result = resultChar;
        switch (srcID) {
            case element_boolean:
                *resultChar = *((bool *)src) ? 1 : 0; break;
            case element_integer:
                *resultChar = integerToCharacter(*((int32_t *)src)); break;
            case element_real:
                errorAndExit("Attempt to cast real to character!"); break;
            default: errorAndExit("This should not happen!"); break;
        }
    } else if (resultID == element_integer) {
        int32_t *resultInt = malloc(sizeof(int32_t));
        *result = resultInt;
        switch (srcID) {
            case element_boolean:
                *resultInt = *((bool *)src) ? 1 : 0; break;
            case element_character:
                *resultInt = (int32_t)*((int32_t *)src); break;
            case element_real:
                // TODO: spec check
                *resultInt = (int32_t)*((float *)src); break;
            default: errorAndExit("This should not happen!"); break;
        }
    } else if (resultID == element_real) {
        float *resultFloat = malloc(sizeof(float));
        *result = resultFloat;
        switch (srcID) {
            case element_boolean:
                *resultFloat = *((bool *)src) ? 1.0f : 0.0f; break;
            case element_character:
                // TODO: spec check
                *resultFloat = (float)*((int8_t *)src); break;
            case element_integer:
                // TODO: spec check
                *resultFloat = (float)*((int32_t *)src); break;
            default: errorAndExit("This should not happen!"); break;
        }
    }
}

// operations that can only be done on the same element type
void elementMallocFromAssignment(ElementTypeID id, void *src, void **result) {
    *result = arrayMallocFromElementValue(id, 1, src);
}

void elementMallocFromUnaryOp(ElementTypeID id, UnaryOpCode opcode, void *src, void **result) {
    if(!elementCanUseUnaryOp(id, opcode)) {
        errorAndExit("Invalid unary operand type!");
    }
    if (id == element_boolean) {
        *result = arrayMallocFromBoolValue(1, *((bool *)src) ? false : true);
    } else if (id == element_integer) {
        int32_t value = *((int32_t *)src);
        *result = arrayMallocFromIntegerValue(1, (opcode == unary_minus) ? -value : value);
    } else if (id == element_real) {
        float value = *((float *)src);
        *result = arrayMallocFromRealValue(1, (opcode == unary_minus) ? -value : value);
    } else {
        errorAndExit("This should not happen!");
    }
}

int32_t integerExponentiation(int32_t base, int32_t exp) {
    // power by repeated squaring
    // TODO: what if we have negative exponent?
    int32_t result = 1;
    while (exp != 0) {
        if (exp % 2 == 1) {
            result *= base;
        }
        exp /= 2;
        base *= base;
    }
}

// binary operation may not return the same type e.g. 1 == 2
void elementMallocFromBinOp(ElementTypeID id, BinOpCode opcode, void *op1, void *op2, void **result) {
    if (id == element_boolean) {
        bool v1 = *((bool *)op1);
        bool v2 = *((bool *)op2);
        bool *resultBool = malloc(sizeof(bool));
        *result = resultBool;
        switch(opcode) {
            case binary_eq:
                *resultBool = v1 == v2; break;
            case binary_ne:
                *resultBool = v1 != v2; break;
            case binary_and:
                *resultBool = v1 && v2; break;
            case binary_or:
                *resultBool = v1 || v2; break;
            case binary_xor:
                *resultBool = v1 ^ v2; break;  // ^ is bitwise, but doesn't matter since all bits in bool are 0 except the last bit
            default:
                errorAndExit("This should not happen!"); break;
        }
    } else if (id == element_integer) {
        int32_t v1 = *((int32_t *)op1);
        int32_t v2 = *((int32_t *)op2);
        switch (opcode) {
            case binary_eq:
                *result = arrayMallocFromBoolValue(1, v1 == v2); break;
            case binary_ne:
                *result = arrayMallocFromBoolValue(1, v1 != v2); break;
            case binary_lt:
                *result = arrayMallocFromBoolValue(1, v1 < v2); break;
            case binary_bt:
                *result = arrayMallocFromBoolValue(1, v1 > v2); break;
            case binary_leq:
                *result = arrayMallocFromBoolValue(1, v1 <= v2); break;
            case binary_beq:
                *result = arrayMallocFromBoolValue(1, v1 >= v2); break;
            case binary_exponent:
                *result = arrayMallocFromIntegerValue(1, integerExponentiation(v1, v2)); break;
            case binary_multiply:
                *result = arrayMallocFromIntegerValue(1, v1 * v2); break;
            case binary_divide:
                *result = arrayMallocFromIntegerValue(1, v1 / v2); break;
            case binary_remainder:
                *result = arrayMallocFromIntegerValue(1, v1 % v2); break;
            case binary_plus:
                *result = arrayMallocFromIntegerValue(1, v1 + v2); break;
            case binary_minus:
                *result = arrayMallocFromIntegerValue(1, v1 - v2); break;
            default:
                errorAndExit("This should not happen!"); break;
        }
    } else if (id == element_real) {
        float v1 = *((float *)op1);
        float v2 = *((float *)op2);
        // integer and real
        switch (opcode) {
            case binary_eq:
                *result = arrayMallocFromBoolValue(1, v1 == v2); break;
            case binary_ne:
                *result = arrayMallocFromBoolValue(1, v1 != v2); break;
            case binary_lt:
                *result = arrayMallocFromBoolValue(1, v1 < v2); break;
            case binary_bt:
                *result = arrayMallocFromBoolValue(1, v1 > v2); break;
            case binary_leq:
                *result = arrayMallocFromBoolValue(1, v1 <= v2); break;
            case binary_beq:
                *result = arrayMallocFromBoolValue(1, v1 >= v2); break;
            case binary_exponent:
                *result = arrayMallocFromRealValue(1, powf(v1, v2)); break;
            case binary_multiply:
                *result = arrayMallocFromRealValue(1, v1 * v2); break;
            case binary_divide:
                *result = arrayMallocFromRealValue(1, v1 / v2); break;
            case binary_remainder:
                // TODO: what should we return on real % real?
                *result = arrayMallocFromRealValue(1, 0.0f); break;
            case binary_plus:
                *result = arrayMallocFromRealValue(1, v1 + v2); break;
            case binary_minus:
                *result = arrayMallocFromRealValue(1, v1 - v2); break;
            default:
                errorAndExit("This should not happen!"); break;
        }
    } else {
        errorAndExit("This should not happen!");
    }
}


///------------------------------DIMENSIONLESS ARRAY---------------------------------------------------------------

void *arrayMallocFromNull(ElementTypeID id, int64_t size) {
    switch (id) {
        case element_boolean:
            return arrayMallocFromBoolValue(size, false);
        case element_integer:
            return arrayMallocFromIntegerValue(size, 0);
        case element_character:
            return arrayMallocFromCharacterValue(size, 0);
        case element_real:
            return arrayMallocFromRealValue(size, 0.0f);
        default:
            return NULL;
    }
}

void *arrayMallocFromIdentity(ElementTypeID id, int64_t size) {
    switch (id) {
        case element_boolean:
            return arrayMallocFromBoolValue(size, true);
        case element_integer:
            return arrayMallocFromIntegerValue(size, 1);
        case element_character:
            return arrayMallocFromCharacterValue(size, 1);
        case element_real:
            return arrayMallocFromRealValue(size, 1.0f);
        default:
            return NULL;
    }
}

void *arrayMallocFromElementValue(ElementTypeID id, int64_t size, void *value) {
    if (elementIsBasicType(id)) {
        int64_t elementSize = elementGetSize(id);
        char *src = value;
        char *target = malloc(elementSize * size);
        for (int64_t i = 0; i < size; i++) {
            memcpy(target + i * elementSize, src, elementSize);
        }
        return (void *)target;
    }
    return NULL;
}

// typed versions of arrayMallocFromElementValue
void *arrayMallocFromBoolValue(int64_t size, bool value) {
    bool *arr = malloc(sizeof(bool) * size);
    for (int64_t i = 0; i < size; i++)
        arr[i] = value;
    return arr;
}
void *arrayMallocFromCharacterValue(int64_t size, int8_t value) {
    int8_t *arr = malloc(sizeof(int8_t) * size);
    for (int64_t i = 0; i < size; i++)
        arr[i] = value;
    return arr;
}
void *arrayMallocFromIntegerValue(int64_t size, int32_t value) {
    int32_t *arr = malloc(sizeof(int32_t) * size);
    for (int64_t i = 0; i < size; i++)
        arr[i] = value;
    return arr;
}
void *arrayMallocFromRealValue(int64_t size, float value) {
    float *arr = malloc(sizeof(float) * size);
    for (int64_t i = 0; i < size; i++)
        arr[i] = value;
    return arr;
}

// simple getter/setters
bool arrayGetBoolValue(void *arr, int64_t index) {
    return ((bool *)arr)[index];
}
int8_t arrayGetCharacterValue(void *arr, int64_t index) {
    return ((int8_t *)arr)[index];
}
int32_t arrayGetIntegerValue(void *arr, int64_t index) {
    return ((int32_t *)arr)[index];
}
float arrayGetRealValue(void *arr, int64_t index) {
    return ((float *)arr)[index];
}
void arraySetBoolValue(void *arr, int64_t index, bool value) {
    ((bool *)arr)[index] = value;
}
void arraySetCharacterValue(void *arr, int64_t index, int8_t value) {
    ((int8_t *)arr)[index] = value;
}
void arraySetIntegerValue(void *arr, int64_t index, int32_t value) {
    ((int32_t *)arr)[index] = value;
}
void arraySetRealValue(void *arr, int64_t index, float value) {
    ((float *)arr)[index] = value;
}

void arrayMallocFromUnaryOp(ElementTypeID id, UnaryOpCode opcode, void *src, int64_t length, void **result);

// make sure the binary op can be handled by dimensionless array before calling this function
bool arrayBinopResultType(ElementTypeID id, BinOpCode opcode, ElementTypeID *resultType, bool* resultCollapseToScalar);
void arrayMallocFromBinOp(ElementTypeID id, BinOpCode opcode, void *op1, int64_t op1Size, void *op2, int64_t op2Size, void **result, int64_t *resultSize);

bool arrayMixedElementCanPromoteToSameType(ElementTypeID *idArray, int64_t size, ElementTypeID *resultType);
void arrayMixedElementPromoteToTargetType(ElementTypeID *idArray, void *valueArray, int64_t size, ElementTypeID targetType, void **result);