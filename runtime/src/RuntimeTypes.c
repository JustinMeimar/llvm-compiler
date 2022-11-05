#include <stddef.h>
#include "RuntimeTypes.h"

///------------------------------INTEGER---------------------------------------------------------------

void integerTypeInit(Type *this) {
    this->m_typeId = typeid_integer;
    this->m_compoundTypeInfo = NULL;
}