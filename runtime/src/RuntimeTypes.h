#pragma once

///------------------------------BASIC TYPES---------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include "Enums.h"

typedef struct struct_gazprea_type {
    TypeID m_typeId;
    void *m_compoundTypeInfo;  // for compound type only
} Type;

typedef struct struct_gazprea_variable {
    TypeQualifier m_qualifier;
    Type m_type;
    int64_t m_dataSize;  // length of data in memory, used to avoid memory location aliasing
    void *m_data;  // stores the value of the variable
} Variable;

///------------------------------TYPE---------------------------------------------------------------

void typeInitFromCopy(Type *this, Type *other);
void typeInitFromAssign(Type *this, Type *lhs, Variable *rhs);
void typeInitFromDeclaration(Type *this, Type *lhs, Variable *rhs);
void typeInitFromCast(Type *this, Type *target, Variable *source);
void typeInitFromPromotion(Type *this, Type *target, Variable *source);
void typeInitFromUnaryOp(Type *this, Variable *operand, UnaryOpCode opcode);
void typeInitFromBinaryOp(Type *this, Variable *op1, Variable *op2, BinaryOpCode opcode);
void typeInitFromTwoSingleTerms(Type *this, Type *first, Type *second);

bool typeIsUnknown(Type *this);
bool typeIsBasicType(Type *this);
bool typeIsVectorOrString(Type *this);  // includes literal
bool typeIsMatrix(Type *this);
bool typeIsIdentical(Type *this, Type *other);

int64_t typeGetMemorySize(Type *this);

// debug print to stdout
void typeDebugPrint(Type *this);
void typeDebugPrintDetailed(Type *this);

///------------------------------INTEGER---------------------------------------------------------------

void integerTypeInit(Type *this);