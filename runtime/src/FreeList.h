#pragma once

typedef struct struct_gazprea_free_list FreeList;

typedef struct struct_gazprea_free_list {
    void *m_data;
    FreeList *m_next;
} FreeList;

FreeList *freeListAppend(FreeList *prev, void *data);
void freeListFreeAll(FreeList *this, void freeFunc(void *));