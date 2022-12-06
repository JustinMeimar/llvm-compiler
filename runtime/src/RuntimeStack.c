#include <malloc.h>
#include "RuntimeStack.h"
#include "RuntimeErrors.h"
#include "VariableStdio.h"

/// helpers
void runtimeStackResize(RuntimeStack *stack, int64_t newSize) {
    StackItem *arr = malloc(newSize * sizeof(StackItem));
    for (int64_t i = 0; i < stack->m_idx; i++) {
        arr[i] = stack->m_stack[i];
    }
    stack->m_size = newSize;
    free(stack->m_stack);
    stack->m_stack = arr;
}
void runtimeStackPush(RuntimeStack *stack, StackItem item) {
    if (stack->m_idx >= stack->m_size) {
        runtimeStackResize(stack, stack->m_size * 2);
    }
    stack->m_stack[stack->m_idx] = item;
    stack->m_idx += 1;
}

/// interfaces
RuntimeStack *runtimeStackMallocThenInit() {
    RuntimeStack *stack = malloc(sizeof(RuntimeStack));
    stack->m_size = 1;
    stack->m_stack = malloc(sizeof(StackItem));
    stack->m_idx = 0;
    return stack;
}
void runtimeStackDestructThenFree(RuntimeStack *stack) {
    runtimeStackRestore(stack, 0);
    free(stack->m_stack);
    free(stack);
}

Variable *variableStackAllocate(RuntimeStack *stack) {
    Variable *var = variableMalloc();
    StackItem item = {STACK_ITEM_VARIABLE, var};
    runtimeStackPush(stack, item);
    return var;
}
Type *typeStackAllocate(RuntimeStack *stack) {
    Type *type = typeMalloc();
    StackItem item = {STACK_ITEM_TYPE, type};
    runtimeStackPush(stack, item);
    return type;
}
int64_t runtimeStackSave(RuntimeStack *stack) {
    return stack->m_idx;
}
void runtimeStackRestore(RuntimeStack *stack, int64_t position) {
    if (position >= 0 && position <= stack->m_idx) {
        for (int64_t i = stack->m_idx - 1; i >= position; i--) {
            StackItem *item = &stack->m_stack[i];
            switch (item->m_typeid) {
                case STACK_ITEM_VARIABLE: {
#ifdef DEBUG_PRINT
                    fprintf(stderr, "daf#27(stack free)\n");
#endif
                    variableDestructThenFreeImpl(item->m_item);
                } break;
                case STACK_ITEM_TYPE: {
                    typeDestructThenFree(item->m_item);
                } break;
                default:
                    errorAndExit("This should not happen!");
            }
        }
        stack->m_idx = position;
    } else {
        errorAndExit("Attempt to restore stack to a invalid position!");
    }
}