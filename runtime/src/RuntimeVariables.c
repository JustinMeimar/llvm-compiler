#include "RuntimeVariables.h"
#include <stdlib.h>
#include "RuntimeErrors.h"
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
// tuple(character c, integer i) - void * - a pointer to the start of memory which contains a int8_t * followed by a int32_t *
// null & identity & [] & stream_in & stream_out - these types do not need to store associated data, m_data is ignored
// unknown - unknown type should not be associated with a variable
// TODO: vector_ref/matrix_ref

///------------------------------HELPER FUNCTIONS---------------------------------------------------------------

//void variableInitFromIntegerLiteral(Variable *this, int32_t value) {
//    this->m_type.m_typeId = typeid_integer;
//    this->m_type.m_compoundTypeInfo = NULL;
//    this->m_data = malloc(sizeof(int32_t));
//    *((int32_t *)this->m_data) = value;
//    this->m_fieldPos = -1;
//    this->m_parent = this->m_data;
//}
//
//void variableInitFromRealLiteral(Variable *this, float value) {
//    this->m_type.m_typeId = typeid_real;
//    this->m_type.m_compoundTypeInfo = NULL;
//    this->m_data = malloc(sizeof(float));
//    *((float *)this->m_data) = value;
//    this->m_fieldPos = -1;
//    this->m_parent = this->m_data;
//}
//
//void variableInitFromCharacterLiteral(Variable *this, int8_t value) {
//    this->m_type.m_typeId = typeid_character;
//    this->m_type.m_compoundTypeInfo = NULL;
//    this->m_data = malloc(sizeof(int8_t));
//    *((int8_t *)this->m_data) = value;
//    this->m_fieldPos = -1;
//    this->m_parent = this->m_data;
//}
//
//void variableInitFromBooleanLiteral(Variable *this, bool value) {
//    this->m_type.m_typeId = typeid_boolean;
//    this->m_type.m_compoundTypeInfo = NULL;
//    this->m_data = malloc(sizeof(bool));
//    *((bool *)this->m_data) = value;
//    this->m_fieldPos = -1;
//    this->m_parent = this->m_data;
//}
//
//void variableInitFromIntervalLiteral(Variable *this, int32_t head, int32_t tail) {
//    this->m_type.m_typeId = typeid_interval;
//    this->m_type.m_compoundTypeInfo = NULL;
//    int *data = malloc(sizeof(int32_t) * 2);
//    this->m_data = data;
//    data[0] = head;
//    data[1] = tail;
//    this->m_fieldPos = -1;
//    this->m_parent = this->m_data;
//}

// <type> var = null;
void variableInitFromNull(Variable *this, Type *type) {
    TypeID typeid = type->m_typeId;
    switch(typeid) {
        // TODO
//        case typeid_integer:
//            variableInitFromIntegerLiteral(this, 0); break;
//        case typeid_character:
//            variableInitFromCharacterLiteral(this, 0); break;
//        case typeid_real:
//            variableInitFromRealLiteral(this, 0.0f); break;
//        case typeid_boolean:
//            variableInitFromBooleanLiteral(this, false); break;
//        case typeid_interval:
//            variableInitFromIntervalLiteral(this, 0, 0); break;
//        case typeid_vector:
//        case typeid_string:
//        case typeid_matrix:
//        case typeid_tuple:
        default:
            targetTypeError(type, "Attempt to promote null into type: ");
            break;
    }
}

///------------------------------INTERFACES---------------------------------------------------------------

Variable *variableMalloc() {
    return malloc(sizeof(Variable));
}

void variableInitFromMemcpy(Variable *this, Variable *other) {
    // copy the type
    typeInitFromCopy(&this->m_type, &other->m_type);
    int64_t dataSize = typeGetMemorySize(&other->m_type);
    this->m_data = malloc(dataSize);
    memcpy(this->m_data, other->m_data, dataSize);
    this->m_fieldPos = other->m_fieldPos;
    this->m_parent = this->m_data;
}

void variableInitFromAssign(Variable *this, Type *lhsType, Variable *rhs) {
    // The left-hand side type can only be lvalue type or reference types
    // TODO: add considerations for reference types
    if (!typeIsLValueType(lhsType)) {
        targetTypeError(lhsType, "Attempt to assign to a value of type:");
    }
    // case 1: assign to scalar
    if (typeIsBasicType(lhsType)) {
        // use the same method as promotion
        variableInitFromPromotion(this, lhsType, rhs);
        return;
    }
    // TODO
}

void variableInitFromDeclaration(Variable *this, Type *lhsType, Variable *rhs) {
    // The left-hand side type can only be lvalue type or of unknown type
    if (!typeIsLValueType(lhsType) && !typeIsUnknown(lhsType)) {
        targetTypeError(lhsType, "Attempt to declare a value of type:");
    }
    // case 1: declaring a scalar
    if (typeIsBasicType(lhsType)) {
        // use the same method as promotion
        variableInitFromPromotion(this, lhsType, rhs);
        return;
    }

    Type *rhsType = &rhs->m_type;
    TypeID lhsTypeID = lhsType->m_typeId;
    TypeID rhsTypeID = rhsType->m_typeId;

    // case 2: declaring a vector
    // TODO
}

void variableInitFromPromotion(Variable *this, Type *targetType, Variable *source) {
    // The target of promotion must be a lvalue type
    if (!typeIsLValueType(targetType)) {
        targetTypeError(targetType, "Attempt to promote to a value of type:");
    }
    Type *srcType = &source->m_type;
    TypeID targetTypeID = targetType->m_typeId;
    TypeID srcTypeID = srcType->m_typeId;

    // case 1: promote to a scalar
//    if (typeIsBasicType(targetType)) {
//        if (targetTypeID == srcTypeID) {
//            // if same type, copy the content
//            variableInitFromMemcpy(this, source);
//        } else if (targetTypeID == typeid_real && srcTypeID == typeid_integer) {  // integer one-way promotion to real
//            // TODO: make sure this promotion follows spec
//            float value = (float) *((int32_t *)source->m_data);
//            variableInitFromRealLiteral(this, value);
//        }
//        return;
//    }
}

void variableDestructor(Variable *this) {
    TypeID id = this->m_type.m_typeId;

    if (id == typeid_ndarray || id == typeid_string) {
        // destructor does different things depending on the element type
        ArrayType *CTI = this->m_type.m_compoundTypeInfo;
        ElementTypeID elementID = CTI->m_elementTypeID;
        if (elementID == element_mixed) {
            // we need to free each pointer in pointer array in m_data
            int64_t size = arrayTypeGetTotalLength(CTI);
            char *ptr = this->m_data;
            for (int64_t i = 0; i < size; i ++) {
                void *curPos = ptr + i * sizeof(void *);
                free(curPos);
            }
            // then free the pointer array itself
            free(this->m_data);
        } else if (elementID == element_null || elementID == element_identity) { // no array is associated with these types
        } else {  // basic type arrays
            free(this->m_data);
        }
    }

    switch (id) {
        case typeid_ndarray:
        case typeid_string:
        case typeid_stream_in:
        case typeid_stream_out:
        case typeid_empty_array:
            break;  // m_data is ignored for these types
        case typeid_interval:
        case typeid_tuple:
            free(this->m_data);
            break;
        case typeid_unknown:
        case NUM_TYPE_IDS:
        default:
            unknownTypeVariableError();
            break;
    }

    typeDestructor(&this->m_type);
}