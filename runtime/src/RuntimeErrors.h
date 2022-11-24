#pragma once

#include <stdlib.h>
#include <stdio.h>
#include "RuntimeTypes.h"

void errorAndExit(const char *errorMsg);
void singleTypeError(Type *targetType, const char *errorMsg);  // assignment, declaration, promotion etc.
void unknownTypeVariableError();