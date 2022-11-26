#include "NDArray.h"
#include "RuntimeErrors.h"
#include "math.h"
#include "string.h"
#include "VariableStdio.h"

void mixedTypeElementInitFromValue(MixedTypeElement *this, ElementTypeID eid, void *value) {
    this->m_elementTypeID = eid;
    switch (eid) {
        case ELEMENT_INTEGER:
        case ELEMENT_REAL:
        case ELEMENT_BOOLEAN:
        case ELEMENT_CHARACTER: {
            int64_t elementSize = elementGetSize(eid);
            this->m_element = malloc(elementSize);
            elementAssign(eid, this->m_element, value);
        } break;
        case ELEMENT_NULL:
        case ELEMENT_IDENTITY:
            this->m_element = NULL; break;
        default:
            errorAndExit("Invalid eid for element of mixed array!");
    }
}

bool elementIsMixedType(ElementTypeID id) {
    return id == ELEMENT_MIXED;
}

bool elementIsNullIdentity(ElementTypeID id) {
    return id == ELEMENT_NULL || id == ELEMENT_IDENTITY;
}

bool elementIsBasicType(ElementTypeID id) {
    return id == ELEMENT_BOOLEAN || id == ELEMENT_CHARACTER || id == ELEMENT_INTEGER || id == ELEMENT_REAL;
}

int64_t elementGetSize(ElementTypeID id) {
    switch (id) {
        case ELEMENT_REAL:
            return sizeof(float);
        case ELEMENT_INTEGER:
            return sizeof(int32_t);
        case ELEMENT_CHARACTER:
            return sizeof(int8_t);
        case ELEMENT_BOOLEAN:
            return sizeof(bool);
        case ELEMENT_MIXED:
            return sizeof(MixedTypeElement);
        default:
            break;
    }
    return 0;
}

void elementAssign(ElementTypeID id, void *target, void *src) {
    switch (id) {
        case ELEMENT_INTEGER:
            *(int32_t *)target = *(int32_t *)src; break;
        case ELEMENT_REAL:
            *(float *)target = *(float *)src; break;
        case ELEMENT_BOOLEAN:
            *(bool *)target = *(bool *)src; break;
        case ELEMENT_CHARACTER:
            *(int8_t *)target = *(int8_t *)src; break;
        case ELEMENT_MIXED: {
            MixedTypeElement *targetElement = target;
            MixedTypeElement *srcElement = src;

            ElementTypeID newID = srcElement->m_elementTypeID;
            arrayFree(targetElement->m_elementTypeID, targetElement->m_element, 1);
            targetElement->m_elementTypeID = newID;
            mixedTypeElementInitFromValue(targetElement, newID, srcElement->m_element);
        } break;
        default: break;
    }
}

bool elementCanBeCastedFrom(ElementTypeID to, ElementTypeID from) {
    if (elementIsBasicType(to) && elementIsBasicType(from)) {
        if (from == ELEMENT_REAL && (to == ELEMENT_BOOLEAN || to == ELEMENT_CHARACTER))
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
        return elementIsNullIdentity(from) || (from == ELEMENT_INTEGER && to == ELEMENT_REAL);
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
    if (!elementIsBasicType(id) || id == ELEMENT_CHARACTER)
        return false;
    if (id == ELEMENT_BOOLEAN) {
        return opcode == UNARY_NOT;
    } else {
        // integer and real
        return opcode == UNARY_PLUS || opcode == UNARY_MINUS;
    }
}

bool elementBinOpResultType(ElementTypeID id, BinOpCode opcode, ElementTypeID *resultType) {
    // first filter the ops that are not always between two elements of same type
    // or not returning an element
    switch(opcode) {
        case BINARY_INDEX:
        case BINARY_RANGE_CONSTRUCT:
        case BINARY_DOT_PRODUCT:
        case BINARY_CONCAT:
            return false;
        default:
            break;
    }
    // no character, null, identity or mixed type can be used in binop without some transformation e.g. promotion
    if (!elementIsBasicType(id) || id == ELEMENT_CHARACTER)
        return false;
    if (id == ELEMENT_BOOLEAN) {
        switch(opcode) {
            case BINARY_EQ:
            case BINARY_NE:
            case BINARY_AND:
            case BINARY_OR:
            case BINARY_XOR:
                *resultType = ELEMENT_BOOLEAN;
                return true;
            default:
                return false;
        }
    } else {
        // integer and real
        switch(opcode) {
            case BINARY_EQ:
            case BINARY_NE:
            case BINARY_LT:
            case BINARY_BT:
            case BINARY_LEQ:
            case BINARY_BEQ:
                *resultType = ELEMENT_BOOLEAN;
                return true;
            case BINARY_EXPONENT:
            case BINARY_MULTIPLY:
            case BINARY_DIVIDE:
            case BINARY_REMAINDER:
            case BINARY_PLUS:
            case BINARY_MINUS:
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
        if (srcID == ELEMENT_NULL)
            *result = arrayMallocFromNull(resultID, 1);
        else if (srcID == ELEMENT_IDENTITY)
            *result = arrayMallocFromIdentity(resultID, 1);
        else if (srcID == ELEMENT_INTEGER && resultID == ELEMENT_REAL) {  // the only remaining case is promoting from integer to real
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
    if (resultID == ELEMENT_BOOLEAN) {
        bool *resultBool = malloc(sizeof(bool));
        *result = resultBool;
        switch (srcID) {
            case ELEMENT_CHARACTER:
                *resultBool = *((int8_t *)src) != 0; break;
            case ELEMENT_INTEGER:
                *resultBool = *((int32_t *)src) != 0; break;
            case ELEMENT_REAL:
                errorAndExit("Attempt to cast real to boolean!"); break;
            default: errorAndExit("This should not happen!"); break;
        }
    } else if (resultID == ELEMENT_CHARACTER) {
        int8_t *resultChar = malloc(sizeof(int8_t));
        *result = resultChar;
        switch (srcID) {
            case ELEMENT_BOOLEAN:
                *resultChar = *((bool *)src) ? 1 : 0; break;
            case ELEMENT_INTEGER:
                *resultChar = integerToCharacter(*((int32_t *)src)); break;
            case ELEMENT_REAL:
                errorAndExit("Attempt to cast real to character!"); break;
            default: errorAndExit("This should not happen!"); break;
        }
    } else if (resultID == ELEMENT_INTEGER) {
        int32_t *resultInt = malloc(sizeof(int32_t));
        *result = resultInt;
        switch (srcID) {
            case ELEMENT_BOOLEAN:
                *resultInt = *((bool *)src) ? 1 : 0; break;
            case ELEMENT_CHARACTER:
                *resultInt = (int32_t)*((unsigned char *)src); break;
            case ELEMENT_REAL:
                // TODO: spec check
                *resultInt = (int32_t)*((float *)src); break;
            default: errorAndExit("This should not happen!"); break;
        }
    } else if (resultID == ELEMENT_REAL) {
        float *resultFloat = malloc(sizeof(float));
        *result = resultFloat;
        switch (srcID) {
            case ELEMENT_BOOLEAN:
                *resultFloat = *((bool *)src) ? 1.0f : 0.0f; break;
            case ELEMENT_CHARACTER:
                // TODO: spec check
                *resultFloat = (float)*((unsigned char *)src); break;
            case ELEMENT_INTEGER:
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
    if (id == ELEMENT_BOOLEAN) {
        *result = arrayMallocFromBoolValue(1, !*((bool *)src));
    } else if (id == ELEMENT_INTEGER) {
        int32_t value = *((int32_t *)src);
        *result = arrayMallocFromIntegerValue(1, (opcode == UNARY_MINUS) ? -value : value);
    } else if (id == ELEMENT_REAL) {
        float value = *((float *)src);
        *result = arrayMallocFromRealValue(1, (opcode == UNARY_MINUS) ? -value : value);
    } else {
        errorAndExit("This should not happen!");
    }
}

int32_t integerExponentiation(int32_t base, int32_t exp) {
    if (base == 0 && exp < 0) {
        errorAndExit("Division by zero!");
    } else if (base == -1) {
        return exp % 2 == 0 ? 1 : -1;
    } else if (exp < 0) {
        return 0;
    }

    // power by repeated squaring
    int32_t result = 1;
    while (exp != 0) {
        if (exp % 2 == 1) {
            result *= base;
        }
        exp /= 2;
        base *= base;
    }
    return result;
}

// binary operation may not return the same type e.g. 1 == 2
void elementMallocFromBinOp(ElementTypeID id, BinOpCode opcode, void *op1, void *op2, void **result) {
    if (id == ELEMENT_BOOLEAN) {
        bool v1 = *((bool *)op1);
        bool v2 = *((bool *)op2);
        bool *resultBool = malloc(sizeof(bool));
        *result = resultBool;
        switch(opcode) {
            case BINARY_EQ:
                *resultBool = v1 == v2; break;
            case BINARY_NE:
                *resultBool = v1 != v2; break;
            case BINARY_AND:
                *resultBool = v1 && v2; break;
            case BINARY_OR:
                *resultBool = v1 || v2; break;
            case BINARY_XOR:
                *resultBool = v1 ^ v2; break;  // ^ is bitwise, but doesn't matter since all bits in bool are 0 except the last bit
            default:
                errorAndExit("This should not happen!"); break;
        }
    } else if (id == ELEMENT_INTEGER) {
        int32_t v1 = *((int32_t *)op1);
        int32_t v2 = *((int32_t *)op2);
        switch (opcode) {
            case BINARY_EQ:
                *result = arrayMallocFromBoolValue(1, v1 == v2); break;
            case BINARY_NE:
                *result = arrayMallocFromBoolValue(1, v1 != v2); break;
            case BINARY_LT:
                *result = arrayMallocFromBoolValue(1, v1 < v2); break;
            case BINARY_BT:
                *result = arrayMallocFromBoolValue(1, v1 > v2); break;
            case BINARY_LEQ:
                *result = arrayMallocFromBoolValue(1, v1 <= v2); break;
            case BINARY_BEQ:
                *result = arrayMallocFromBoolValue(1, v1 >= v2); break;
            case BINARY_EXPONENT:
                *result = arrayMallocFromIntegerValue(1, integerExponentiation(v1, v2)); break;
            case BINARY_MULTIPLY:
                *result = arrayMallocFromIntegerValue(1, v1 * v2); break;
            case BINARY_DIVIDE:
                *result = arrayMallocFromIntegerValue(1, v1 / v2); break;
            case BINARY_REMAINDER:
                *result = arrayMallocFromIntegerValue(1, v1 % v2); break;
            case BINARY_PLUS:
                *result = arrayMallocFromIntegerValue(1, v1 + v2); break;
            case BINARY_MINUS:
                *result = arrayMallocFromIntegerValue(1, v1 - v2); break;
            default:
                errorAndExit("This should not happen!"); break;
        }
    } else if (id == ELEMENT_REAL) {
        float v1 = *((float *)op1);
        float v2 = *((float *)op2);
        // integer and real
        switch (opcode) {
            case BINARY_EQ:
                *result = arrayMallocFromBoolValue(1, v1 == v2); break;
            case BINARY_NE:
                *result = arrayMallocFromBoolValue(1, v1 != v2); break;
            case BINARY_LT:
                *result = arrayMallocFromBoolValue(1, v1 < v2); break;
            case BINARY_BT:
                *result = arrayMallocFromBoolValue(1, v1 > v2); break;
            case BINARY_LEQ:
                *result = arrayMallocFromBoolValue(1, v1 <= v2); break;
            case BINARY_BEQ:
                *result = arrayMallocFromBoolValue(1, v1 >= v2); break;
            case BINARY_EXPONENT:
                *result = arrayMallocFromRealValue(1, powf(v1, v2)); break;
            case BINARY_MULTIPLY:
                *result = arrayMallocFromRealValue(1, v1 * v2); break;
            case BINARY_DIVIDE:
                *result = arrayMallocFromRealValue(1, v1 / v2); break;
            case BINARY_REMAINDER:
                *result = arrayMallocFromRealValue(1, fmodf(v1, v2)); break;
            case BINARY_PLUS:
                *result = arrayMallocFromRealValue(1, v1 + v2); break;
            case BINARY_MINUS:
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
        case ELEMENT_BOOLEAN:
            return arrayMallocFromBoolValue(size, false);
        case ELEMENT_INTEGER:
            return arrayMallocFromIntegerValue(size, 0);
        case ELEMENT_CHARACTER:
            return arrayMallocFromCharacterValue(size, 0);
        case ELEMENT_REAL:
            return arrayMallocFromRealValue(size, 0.0f);
        case ELEMENT_MIXED:
            errorAndExit("Attempt to malloc a mixed type array from null!");
        default: break;
    }
    return NULL;
}

void *arrayMallocFromIdentity(ElementTypeID id, int64_t size) {
    switch (id) {
        case ELEMENT_BOOLEAN:
            return arrayMallocFromBoolValue(size, true);
        case ELEMENT_INTEGER:
            return arrayMallocFromIntegerValue(size, 1);
        case ELEMENT_CHARACTER:
            return arrayMallocFromCharacterValue(size, 1);
        case ELEMENT_REAL:
            return arrayMallocFromRealValue(size, 1.0f);
        default:  break;
    }
    return NULL;
}

void *arrayMallocFromElementValue(ElementTypeID id, int64_t size, void *value) {
    if (elementIsBasicType(id) || id == ELEMENT_MIXED) {
        int64_t elementSize = elementGetSize(id);
        char *target = malloc(elementSize * size);

        if (id == ELEMENT_MIXED) {
            MixedTypeElement *element = value;
            for (int64_t i = 0; i < size; i++) {
                MixedTypeElement *curTarget = (void *)(target + i * elementSize);
                mixedTypeElementInitFromValue(curTarget, element->m_elementTypeID, element->m_element);
            }
        } else {
            for (int64_t i = 0; i < size; i++) {
                void *curTarget = target + i * elementSize;
                memcpy(curTarget, value, elementSize);
            }
        }
        return (void *)target;
    }
    return NULL;
}

void *arrayMallocFromMemcpy(ElementTypeID id, int64_t size, void *value) {
    if (elementIsBasicType(id)) {
        int64_t elementSize = elementGetSize(id);
        void *target = malloc(elementSize * size);
        memcpy(target, value, elementSize * size);
        return target;
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

void arrayFree(ElementTypeID id, void *arr, int64_t size) {
    if (elementIsBasicType(id)) {
        free(arr);
    } else if (elementIsMixedType(id)) {
        MixedTypeElement *ptr = arr;
        for (int64_t i = 0; i < size; i++) {
            if (elementIsBasicType(ptr[i].m_elementTypeID))
                free(ptr[i].m_element);
        }
        // then free the pointer array itself
        free(arr);
    }
}

void *arrayGetElementPtrAtIndex(ElementTypeID eid, void *arr, int64_t index) {
    if (eid == ELEMENT_NULL || eid == ELEMENT_IDENTITY)
        return arr;
    int64_t elementSize = elementGetSize(eid);
    return ((char *)arr) + elementSize * index;
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

void arrayMallocFromUnaryOp(ElementTypeID id, UnaryOpCode opcode, void *src, int64_t length, void **result) {
    if(!elementCanUseUnaryOp(id, opcode)) {
        errorAndExit("Invalid unary operand type!");
    }
    if (id == ELEMENT_BOOLEAN) {
        bool *resultArray = arrayMallocFromNull(id, length);
        for (int64_t i = 0; i < length; i++)
            resultArray[i] = !arrayGetBoolValue(src, i);
        *result = resultArray;
    } else if (id == ELEMENT_INTEGER) {
        int32_t *resultArray = arrayMallocFromNull(id, length);
        for (int64_t i = 0; i < length; i++) {
            int32_t value = arrayGetIntegerValue(src, i);
            resultArray[i] = (opcode == UNARY_MINUS) ? -value : value;
        }
        *result = resultArray;
    } else if (id == ELEMENT_REAL) {
        float *resultArray = arrayMallocFromNull(id, length);
        for (int64_t i = 0; i < length; i++) {
            float value = arrayGetRealValue(src, i);
            resultArray[i] = (opcode == UNARY_MINUS) ? -value : value;
        }
        *result = resultArray;
    } else if (id == ELEMENT_MIXED) {
        ElementTypeID eid;
        if (!arrayMixedElementCanBePromotedToSameType(src, length, &eid)) {
            errorAndExit("mixed type can not be promoted to the same type!");
        }
        void *temp;
        arrayMallocFromPromote(eid, id, length, src, &temp);
        arrayMallocFromUnaryOp(eid, opcode, temp, length, result);
        free(temp);
    } else {
        errorAndExit("This should not happen!");
    }
}

// make sure the binary op can be handled by dimensionless array before calling this function
bool arrayBinopResultType(ElementTypeID id, BinOpCode opcode, ElementTypeID *resultType, bool* resultCollapseToScalar) {
    // first filter the ops that can not be done between two arrays
    // or not returning an array
    if (opcode == BINARY_RANGE_CONSTRUCT || opcode == BINARY_INDEX)
        return false;
    // no null, identity or mixed type can be used in binop without some transformation e.g. promotion
    if (!elementIsBasicType(id))
        return false;
    if (id == ELEMENT_BOOLEAN) {
        switch(opcode) {
            case BINARY_EQ:
            case BINARY_NE:
                *resultType = ELEMENT_BOOLEAN;
                *resultCollapseToScalar = true;
                return true;
            case BINARY_AND:
            case BINARY_OR:
            case BINARY_XOR:
            case BINARY_CONCAT:
                *resultType = ELEMENT_BOOLEAN;
                *resultCollapseToScalar = false;
                return true;
            default: break;
        }
    } else if (id == ELEMENT_CHARACTER) {
        if (opcode == BINARY_CONCAT) {
            *resultType = id;
            *resultCollapseToScalar = false;
            return true;
        }
    } else {
        // integer and real
        switch(opcode) {
            case BINARY_EQ:
            case BINARY_NE:
                *resultType = ELEMENT_BOOLEAN;
                *resultCollapseToScalar = true;
                return true;
            case BINARY_LT:
            case BINARY_BT:
            case BINARY_LEQ:
            case BINARY_BEQ:
                *resultType = ELEMENT_BOOLEAN;
                *resultCollapseToScalar = false;
                return true;
            case BINARY_EXPONENT:
            case BINARY_MULTIPLY:
            case BINARY_DIVIDE:
            case BINARY_REMAINDER:
            case BINARY_PLUS:
            case BINARY_MINUS:
            case BINARY_CONCAT:
                *resultType = id;  // same as the original element type
                *resultCollapseToScalar = false;
                return true;
            case BINARY_DOT_PRODUCT:
                *resultType = id;
                *resultCollapseToScalar = true;
            default: break;
        }
    }
    return false;
}

void arrayMallocFromBinOp(ElementTypeID id, BinOpCode opcode, void *op1, int64_t op1Size, void *op2, int64_t op2Size, void **result, int64_t *resultSize) {
    ElementTypeID resultEID;
    bool resultCollapseToScalar;
    bool success = arrayBinopResultType(id, opcode, &resultEID, &resultCollapseToScalar);
    if (!success) {
        fprintf(stderr, "NDArray can't perform binop between element id:%d and opcode:%d", id, opcode);
        errorAndExit("Invalid type for binary operator!");
    }

    char *op1Pos = op1;
    char *op2Pos = op2;
    char *resultPos = NULL;
    int64_t elementSize = elementGetSize(id);
    int64_t resultElementSize = elementGetSize(resultEID);
    int64_t resultArraySize;
    // only differences from scalar binop is that now we have dot-product, concat and boolean; as well as != and ==
    // and the only way we have binop between two boolean arrays is when we concat them
    // op1Size should be the same as op2Size except for concatenation '||'
    if (opcode == BINARY_CONCAT) {
        resultArraySize = op1Size + op2Size;
        resultPos = malloc(resultArraySize * resultElementSize);
        memcpy(resultPos, op1, op1Size * elementSize);
        memcpy(resultPos + op1Size * elementSize, op2, op2Size * elementSize);
    } else if (opcode == BINARY_DOT_PRODUCT) {
        resultArraySize = 1;

        void *sum = arrayMallocFromNull(resultEID, 1);
        for (int64_t i = 0; i < resultArraySize; i++) {
            // sum += op1[i] * op2[i]
            void *temp;
            void *tempSum;
            elementMallocFromBinOp(id, BINARY_MULTIPLY, op1Pos + i * elementSize, op2Pos + i * elementSize, &temp);
            elementMallocFromBinOp(resultEID, BINARY_PLUS, sum, temp, &tempSum);
            free(temp);
            free(sum);
            sum = tempSum;
        }
        resultPos = sum;
    } else if (opcode == BINARY_EQ || opcode == BINARY_NE) {
        resultArraySize = 1;

        bool *aggregate = arrayMallocFromIdentity(resultEID, resultArraySize);
        *aggregate = memcmp(op1Pos, op2Pos, op1Size * elementSize) == 0;
        if (opcode == BINARY_NE)
            *aggregate = !*aggregate;
        resultPos = (void *)aggregate;
    } else {  // for other operators, this is same as scalar case i.e. the binop is done element-wise
        resultArraySize = op1Size;
        resultPos = malloc(resultArraySize * resultElementSize);
        for (int64_t i = 0; i < resultArraySize; i++) {
            void *temp;
            elementMallocFromBinOp(id, opcode, op1Pos + i * elementSize, op2Pos + i * elementSize, &temp);
            memcpy(resultPos + i * resultElementSize, temp, resultElementSize);
            free(temp);
        }
    }
    *result = resultPos;
    if (resultSize != NULL)
        *resultSize = resultArraySize;
}

void arrayMallocFromCastPromote(ElementTypeID resultID, ElementTypeID srcID, int64_t size, void *src, void **result,
    void conversion(ElementTypeID, ElementTypeID, void*, void**)) {
    int64_t resultElementSize = elementGetSize(resultID);
    char *resultPos = malloc(resultElementSize * size);

    if (srcID == ELEMENT_MIXED) {
        MixedTypeElement *mixed = src;
        for (int64_t i = 0; i < size; i++) {
            ElementTypeID eid = mixed[i].m_elementTypeID;
            void *temp;
            conversion(resultID, eid, mixed[i].m_element, &temp);
            memcpy(resultPos + resultElementSize * i, temp, resultElementSize);
            free(temp);
        }
    } else {
        int64_t srcElementSize = elementGetSize(srcID);
        char *srcPos = src;
        for (int64_t i = 0; i < size; i++) {
            void *temp;
            conversion(resultID, srcID, srcPos + i * srcElementSize, &temp);
            memcpy(resultPos + resultElementSize * i, temp, resultElementSize);
            free(temp);
        }
    }
    *result = resultPos;
}

void arrayMallocFromCast(ElementTypeID resultID, ElementTypeID srcID, int64_t size, void *src, void **result) {
    arrayMallocFromCastPromote(resultID, srcID, size, src, result, elementMallocFromCast);
}
void arrayMallocFromPromote(ElementTypeID resultID, ElementTypeID srcID, int64_t size, void *src, void **result) {
    arrayMallocFromCastPromote(resultID, srcID, size, src, result, elementMallocFromPromotion);
}

bool arrayMixedElementCanBePromotedToSameType(MixedTypeElement *arr, int64_t size, ElementTypeID *resultType) {
    // not a general algorithm, but should work on gazprea since the only nontrivial scalar promotion is integer->real
    *resultType = arr[0].m_elementTypeID;  // we assume array literal is at least size one
    int64_t numNull = *resultType == ELEMENT_NULL;
    int64_t numIdentity = *resultType == ELEMENT_IDENTITY;
    for (int64_t i = 1; i < size; i++) {
        ElementTypeID eid = arr[i].m_elementTypeID;
        if (eid == ELEMENT_NULL || eid == ELEMENT_IDENTITY) {
            numNull += eid == ELEMENT_NULL;
            numIdentity += eid == ELEMENT_IDENTITY;
        } else {
            bool success = elementCanBePromotedBetween(*resultType, eid, resultType);
            if (!success)
                return false;
        }
    }
    return numNull == 0 || numIdentity == 0 || numNull + numIdentity < size;  // all identity, all null or exist some other type
}

// n * m matrix multiply by m * k matrix to produce a n * k matrix
void arrayMallocFromMatrixMultiplication(ElementTypeID id, void *op1, void *op2, int64_t n, int64_t m, int64_t k, void **result) {
    if (id == ELEMENT_INTEGER) {
        int32_t *mat1 = op1;
        int32_t *mat2 = op2;
        int32_t *mat3 = arrayMallocFromNull(id, n * k);
        for (int64_t i = 0; i < n; i++) {
            for (int64_t j = 0; j < k; j++) {
                int32_t sum = 0;
                for (int64_t l = 0; l < m; l++) {
                    sum += mat1[i * m + l] * mat2[l * k + j];
                }
                mat3[i * k + j] = sum;
            }
        }
        *result = mat3;
        return;
    } else if (id == ELEMENT_REAL) {
        float *mat1 = op1;
        float *mat2 = op2;
        float *mat3 = arrayMallocFromNull(id, n * k);
        for (int64_t i = 0; i < n; i++) {
            for (int64_t j = 0; j < k; j++) {
                float sum = 0;
                for (int64_t l = 0; l < m; l++) {
                    sum += mat1[i * m + l] * mat2[l * k + j];
                }
                mat3[i * k + j] = sum;
            }
        }
        *result = mat3;
        return;
    }
    errorAndExit("Invalid matrix multiplication base type!");
}

void arrayMallocFromVectorResize(ElementTypeID id, void *old, int64_t oldSize, int64_t newSize, void **result) {
    if (!elementIsBasicType(id)) {
        errorAndExit("Vector to be resized is not of any of the basic types!");
    }
    int64_t elementSize = elementGetSize(id);
    char *oldArr = old;
    char *resultArr = arrayMallocFromNull(id, newSize);
    for (int64_t i = 0; i < oldSize && i < newSize; i++) {
        memcpy(resultArr + i * elementSize, oldArr + i * elementSize, elementSize);
    }
    *result = resultArr;
}

void arrayMallocFromMatrixResize(ElementTypeID id, void *old, int64_t oldNRow, int64_t oldNCol, int64_t newNRow, int64_t newNCol, void **result) {
    if (!elementIsBasicType(id)) {
        errorAndExit("Vector to be resized is not of any of the basic types!");
    }
    int64_t elementSize = elementGetSize(id);
    char *oldArr = old;
    char *resultArr = arrayMallocFromNull(id, newNRow * newNCol);
    for (int64_t i = 0; i < oldNRow && i < newNRow; i++) {
        for (int64_t j = 0; j < oldNCol && j < newNCol; j++) {
            int64_t offset = i * newNCol + j;
            memcpy(resultArr + offset * elementSize, oldArr + offset * elementSize, elementSize);
        }
    }
    *result = resultArr;
}