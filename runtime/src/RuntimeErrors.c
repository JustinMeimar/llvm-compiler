#include "RuntimeErrors.h"
#include "VariableStdio.h"

void errorAndExit(const char *errorMsg) {
    fprintf(stderr, "%s", errorMsg);
    exit(1);
}

void singleTypeError(Type *targetType, const char *errorMsg) {
    fprintf(stderr, "%s", errorMsg);
    typeDebugPrint(targetType);
    exit(1);
}

void doubleTypeError(Type *type1, Type *type2, const char *errorMsg) {
    fprintf(stderr, "%s", errorMsg);
    typeDebugPrint(type1);
    fprintf(stderr, " and ");
    typeDebugPrint(type2);
    exit(1);
}

void unknownTypeVariableError() {
    fprintf(stderr, "Found a variable of unknown type!");
    exit(1);
}