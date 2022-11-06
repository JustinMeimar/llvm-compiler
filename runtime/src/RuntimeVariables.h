#pragma once

#include <stdint.h>
#include "RuntimeTypes.h"

///------------------------------VARIABLE---------------------------------------------------------------
typedef struct struct_gazprea_variable {
    Type m_type;
    void *m_parent;  // used to avoid memory location aliasing in case of vector, matrix and tuple
    int64_t m_fieldPos;  // used to avoid tuple field aliasing
    void *m_data;  // stores the value of the variable
} Variable;

Variable *variableMalloc();

void variableInitFromAssign(Variable *this, Type *lhs, Variable *rhs);
void variableInitFromDeclaration(Variable *this, Type *lhs, Variable *rhs);
void variableInitFromCast(Variable *this, Type *target, Variable *source);
void variableInitFromPromotion(Variable *this, Type *target, Variable *source);
void variableInitFromUnaryOp(Variable *this, Variable *operand, UnaryOpCode opcode);
void variableInitFromBinaryOp(Variable *this, Variable *op1, Variable *op2, BinOpCode opcode);
void variableInitFromParameter(Variable *this, Type *parameterType, Variable *invokeValue);

void variableDestructor(Variable *this);

bool variableAliasWith(Variable *this, Variable *other);  // return ture if the two variable alias
void variableVectorIndexAssignment(Variable *vector, Variable *index, Variable *rhs);
void variableMatrixIndexAssignment(Variable *vector, Variable *rowIndex, Variable *colIndex, Variable *rhs);

// debug print to stdout
void variableDebugPrint(Type *this);