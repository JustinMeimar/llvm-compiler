#include <malloc.h>
#include "FreeList.h"

FreeList *freeListAppend(FreeList *prev, void *data) {
    FreeList *result = malloc(sizeof(FreeList));
    result->m_data = data;
    result->m_next = prev;
    return result;
}

void freeListFreeAll(FreeList *this, void freeFunc(void *)) {
    if (this != NULL) {
        freeListFreeAll(this->m_next, freeFunc);
#ifdef DEBUG_PRINT
        fprintf(stderr, "Freelistfreefunc\n");
#endif
        freeFunc(this->m_data);
        free(this);
    }
}