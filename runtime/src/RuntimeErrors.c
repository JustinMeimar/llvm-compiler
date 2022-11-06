#include "RuntimeErrors.h"

void errorAndExit(const char *errorMsg) {
    fprintf(stderr, "%s", errorMsg);
    exit(1);
}

void targetTypeError(Type *targetType, const char *errorMsg) {
    fprintf(stderr, "%s", errorMsg);
    typeDebugPrint(targetType);
    exit(1);
}

void unknownTypeVariableError() {
    fprintf(stderr, "Found a variable of unknown type!");
    exit(1);
}