#include "Literal.h"
#include "NDArray.h"

void variableInitFromBooleanScalar(Variable *this, bool value) {
    this->m_type = typeMalloc();
    this->m_data = arrayMallocFromBoolValue(1, value);
    this->m_fieldPos = -1;
    this->m_parent = this->m_data;
}