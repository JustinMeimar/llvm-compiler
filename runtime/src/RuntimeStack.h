#pragma once

#include "RuntimeVariables.h"
#include "RuntimeTypes.h"
#include "Enums.h"


typedef struct struct_runtime_stack_item {
    StackItemType m_typeid;
    void *m_item;
} StackItem;

typedef struct struct_runtime_stack {
    int64_t m_idx;
    int64_t m_size;
    StackItem *m_stack;
} RuntimeStack;

/// INTERFACE
RuntimeStack *runtimeStackMallocThenInit();
void runtimeStackDestructThenFree(RuntimeStack *stack);

Variable *variableStackAllocate(RuntimeStack *stack);
Type *typeStackAllocate(RuntimeStack *stack);
int64_t runtimeStackSave(RuntimeStack *stack);  // returns the position of the current stack pointer
void runtimeStackRestore(RuntimeStack *stack, int64_t position);


/// How to use runtime stack to free variables

// program start:
// stack = runtimeStackMallocThenInit();


// program end:
// runtimeStackDestructThenFree(stack)


// every scoped variable or type (that is, not temporary variable)'s variableMalloc() or typeMalloc() is replaced with variableStackAllocate(stack)


// block statement
// {
// blockStart = runtimeStackSave(stack);
// (block body)
// runtimeStackRestore(stack, blockStart);
// }


// if-else statement
// stacksave/restore is optional for if because if statement can't contain a declaration without a block statement


// infinite loop statement
// loopStart = runtimeStackSave(stack);
// bodyStart = runtimeStackSave(stack);  // no need to put this line inside the body
// (loop body)  // can have break and continue
// runtimeStackRestore(stack, bodyStart);
// MERGE:
// runtimeStackRestore(stack, loopStart);


// break statement
// (break)


// continue statement
// runtimeStackRestore(stack, bodyStart);
// (continue)


// loop while statement
// loopStart = runtimeStackSave(stack);
// bodyStart = runtimeStackSave(stack);
// HEADER:
// (loop header)
// BODY:
// (loop body)
// runtimeStackRestore(stack, bodyStart);
// MERGE:
// runtimeStackRestore(stack, loopStart);  // note if we break out of loop body then we will restore twice but this should be fine


// post-predicated while statement
// loopStart = runtimeStackSave(stack);
// bodyStart = runtimeStackSave(stack);
// BODY:
// (loop body)
// (loop header(footer?))
// runtimeStackRestore(stack, bodyStart);  // right before jump
// MERGE:
// runtimeStackRestore(stack, loopStart);


// iterator loop
// loopStart = runtimeStackSave(stack);
// (loop header)  // where domain variables are allocated
// bodyStart = runtimeStackSave(stack);
// (loop body)
// runtimeStackRestore(stack, bodyStart);
// MERGE:
// runtimeStackRestore(stack, loopStart);


// subroutine definition
// subroutineStart = runtimeStackSave(stack);
// (parameter list)
// (body)
// (return statement)  // assume we always have a return statement


// return statement
// runtimeStackRestore(stack, subroutineStart);
// (return)