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

void unknownTypeVariableError() {
    fprintf(stderr, "Found a variable of unknown type!");
    exit(1);
}