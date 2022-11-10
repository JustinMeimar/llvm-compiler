#pragma once
#include "RuntimeVariables.h"

typedef struct struct_gazprea_variable Variable;
typedef struct struct_gazprea_type Type;

void variableInitFromBooleanScalar(Variable *this, bool value);
void variableInitFromIntegerScalar(Variable *this, int32_t value);
void variableInitFromRealScalar(Variable *this, float value);
void variableInitFromCharacterScalar(Variable *this, int8_t value);
void variableInitFromNullScalar(Variable *this);
void variableInitFromIdentityScalar(Variable *this);