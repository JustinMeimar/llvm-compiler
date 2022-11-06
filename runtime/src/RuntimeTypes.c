#include <stddef.h>
#include "RuntimeTypes.h"
#include <stdlib.h>

///------------------------------TYPE AND VARIABLE---------------------------------------------------------------

Type *typeMalloc() {
    return malloc(sizeof(Type));
}

bool typeIsUnknown(Type *this) {
    return this->m_typeId == typeid_unknown;
}

bool typeIsBasicType(Type *this) {
    TypeID id = this->m_typeId;
    return id == typeid_integer || id == typeid_boolean || id == typeid_character || id == typeid_real;
}

bool typeIsVectorOrString(Type *this) {
    TypeID id = this->m_typeId;
    return id == typeid_vector || id == typeid_vector_literal || id == typeid_string;
}

bool typeIsMatrix(Type *this) {
    TypeID id = this->m_typeId;
    return id == typeid_matrix || id == typeid_matrix_literal;
}