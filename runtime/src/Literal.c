#include <stddef.h>
#include "Literal.h"
#include "NDArray.h"

///------------------------------TYPE---------------------------------------------------------------

void typeInitFromUnspecifiedInterval(Type *this) {
    typeInitFromIntervalType(this, unspecified_base_interval);
}


///------------------------------VARIABLE---------------------------------------------------------------

void variableInitFromBooleanScalar(Variable *this, bool value) {
    this->m_type = typeMalloc();
    typeInitFromArrayType(this->m_type, element_boolean, 0, NULL);
    this->m_data = arrayMallocFromBoolValue(1, value);
    this->m_fieldPos = -1;
    this->m_parent = this->m_data;
}

void variableInitFromIntegerScalar(Variable *this, int32_t value){
    this->m_type = typeMalloc();
    typeInitFromArrayType(this->m_type, element_integer, 0, NULL);
    this->m_data = arrayMallocFromIntegerValue(1, value);
    this->m_fieldPos = -1;
    this->m_parent = this->m_data;
}

void variableInitFromRealScalar(Variable *this, float value){
    this->m_type = typeMalloc();
    typeInitFromArrayType(this->m_type, element_real, 0, NULL);
    this->m_data = arrayMallocFromRealValue(1, value);
    this->m_fieldPos = -1;
    this->m_parent = this->m_data;
}

void variableInitFromCharacterScalar(Variable *this, int8_t value){
    this->m_type = typeMalloc();
    typeInitFromArrayType(this->m_type, element_character, 0, NULL);
    this->m_data = arrayMallocFromCharacterValue(1, value);
    this->m_fieldPos = -1;
    this->m_parent = this->m_data;
}

void variableInitFromNullScalar(Variable *this) {
    this->m_type = typeMalloc();
    typeInitFromArrayType(this->m_type, element_null, 0, NULL);
    this->m_data = arrayMallocFromNull(element_null, 1);
    this->m_fieldPos = -1;
    this->m_parent = this->m_data;
}

void variableInitFromIdentityScalar(Variable *this){
    this->m_type = typeMalloc();
    typeInitFromArrayType(this->m_type, element_identity, 0, NULL);
    this->m_data = arrayMallocFromIdentity(element_identity, 1);
    this->m_fieldPos = -1;
    this->m_parent = this->m_data;
}

void variableInitFromStdInput(Variable *this) {
    this->m_type = typeMalloc();
    this->m_type->m_compoundTypeInfo = NULL;
    this->m_type->m_typeId = typeid_stream_in;
    this->m_data = NULL;
    this->m_fieldPos = -1;
    this->m_parent = this->m_data;
}

void variableInitFromStdOutput(Variable *this) {
    this->m_type = typeMalloc();
    this->m_type->m_compoundTypeInfo = NULL;
    this->m_type->m_typeId = typeid_stream_out;
    this->m_data = NULL;
    this->m_fieldPos = -1;
    this->m_parent = this->m_data;
}