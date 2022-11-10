#include <stddef.h>
#include "Literal.h"
#include "NDArray.h"

///------------------------------TYPE---------------------------------------------------------------

void typeInitFromUnspecifiedInterval(Type *this) {
    typeInitFromIntervalType(this, UNSPECIFIED_BASE_INTERVAL);
}


///------------------------------VARIABLE---------------------------------------------------------------

void variableInitFromBooleanScalar(Variable *this, bool value) {
    this->m_type = typeMalloc();
    typeInitFromArrayType(this->m_type, ELEMENT_BOOLEAN, 0, NULL);
    this->m_data = arrayMallocFromBoolValue(1, value);
    this->m_fieldPos = -1;
    this->m_parent = this->m_data;
}

void variableInitFromIntegerScalar(Variable *this, int32_t value){
    this->m_type = typeMalloc();
    typeInitFromArrayType(this->m_type, ELEMENT_INTEGER, 0, NULL);
    this->m_data = arrayMallocFromIntegerValue(1, value);
    this->m_fieldPos = -1;
    this->m_parent = this->m_data;
}

void variableInitFromRealScalar(Variable *this, float value){
    this->m_type = typeMalloc();
    typeInitFromArrayType(this->m_type, ELEMENT_REAL, 0, NULL);
    this->m_data = arrayMallocFromRealValue(1, value);
    this->m_fieldPos = -1;
    this->m_parent = this->m_data;
}

void variableInitFromCharacterScalar(Variable *this, int8_t value){
    this->m_type = typeMalloc();
    typeInitFromArrayType(this->m_type, ELEMENT_CHARACTER, 0, NULL);
    this->m_data = arrayMallocFromCharacterValue(1, value);
    this->m_fieldPos = -1;
    this->m_parent = this->m_data;
}

void variableInitFromNullScalar(Variable *this) {
    this->m_type = typeMalloc();
    typeInitFromArrayType(this->m_type, ELEMENT_NULL, 0, NULL);
    this->m_data = arrayMallocFromNull(ELEMENT_NULL, 1);
    this->m_fieldPos = -1;
    this->m_parent = this->m_data;
}

void variableInitFromIdentityScalar(Variable *this){
    this->m_type = typeMalloc();
    typeInitFromArrayType(this->m_type, ELEMENT_IDENTITY, 0, NULL);
    this->m_data = arrayMallocFromIdentity(ELEMENT_IDENTITY, 1);
    this->m_fieldPos = -1;
    this->m_parent = this->m_data;
}

void variableInitFromStdInput(Variable *this) {
    this->m_type = typeMalloc();
    this->m_type->m_compoundTypeInfo = NULL;
    this->m_type->m_typeId = TYPEID_STREAM_IN;
    this->m_data = NULL;
    this->m_fieldPos = -1;
    this->m_parent = this->m_data;
}

void variableInitFromStdOutput(Variable *this) {
    this->m_type = typeMalloc();
    this->m_type->m_compoundTypeInfo = NULL;
    this->m_type->m_typeId = TYPEID_STREAM_OUT;
    this->m_data = NULL;
    this->m_fieldPos = -1;
    this->m_parent = this->m_data;
}