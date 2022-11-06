#pragma once

///------------------------------TYPE AND VARIABLE---------------------------------------------------------------

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

Type *typeMalloc();

void typeInitFromCopy(Type *this, Type *other);
void typeInitFromAssign(Type *this, Type *lhs, Variable *rhs);
void typeInitFromDeclaration(Type *this, Type *lhs, Variable *rhs);
void typeInitFromCast(Type *this, Type *target, Variable *source);
void typeInitFromPromotion(Type *this, Type *target, Variable *source);
void typeInitFromUnaryOp(Type *this, Variable *operand, UnaryOpCode opcode);
void typeInitFromBinaryOp(Type *this, Variable *op1, Variable *op2, BinaryOpCode opcode);
void typeInitFromTwoSingleTerms(Type *this, Type *first, Type *second);
void typeInitFromParameter(Type *this, Type *parameterType, Variable *invokeValue);  // same as fromDeclaration but allow vec/matrix references

void typeDeconstructor(Type *this);

bool typeIsUnknown(Type *this);
bool typeIsVectorSizeUnknown(Type *this);  // applies to only vector and #rows in vector
bool typeIsSize2Unknown(Type *this);
bool typeIsBasicType(Type *this);
bool typeIsVectorOrString(Type *this);  // includes literal but not empty list
bool typeIsMatrix(Type *this);
bool typeIsIdentical(Type *this, Type *other);  // checks if the two types are describing the same types

int64_t typeGetMemorySize(Type *this);

// debug print to stdout
void typeDebugPrint(Type *this);

///------------------------------Variable---------------------------------------------------------------

Variable *variableMalloc();

void variableInitFromCopy(Variable *this, Variable *other);
void variableInitFromAssign(Variable *this, Type *lhs, Variable *rhs);
void variableInitFromDeclaration(Variable *this, Type *lhs, Variable *rhs);
void variableInitFromCast(Variable *this, Type *target, Variable *source);
void variableInitFromPromotion(Variable *this, Type *target, Variable *source);
void variableInitFromUnaryOp(Variable *this, Variable *operand, UnaryOpCode opcode);
void variableInitFromBinaryOp(Variable *this, Variable *op1, Variable *op2, BinaryOpCode opcode);
void variableInitFromParameter(Variable *this, Type *parameterType, Variable *invokeValue);

void variableDeconstructor(Type *this);

bool variableAliasWith(Variable *this, Variable *other);  // return ture if the two variable alias
void variableVectorIndexAssignment(Variable *vector, Variable *index, Variable *rhs);
void variableMatrixIndexAssignment(Variable *vector, Variable *rowIndex, Variable *colIndex, Variable *rhs);

// debug print to stdout
void variableDebugPrint(Type *this);

///------------------------------COMPOUND TYPE INFO---------------------------------------------------------------
// VectorType---------------------------------------------------------------------------------------------
typedef struct struct_gazprea_vector_or_string_type {
    TypeID  m_elementTypeID;
    int64_t m_length;  // # of elements in the vector
} VectorType;

VectorType *vectorTypeMalloc();

void vectorTypeInitFromLength(VectorType *this, TypeID elementType, int64_t size);  // SIZE_UNKNOWN for unknown size
void vectorTypeInitFromCopy(VectorType *this, VectorType *other);

// note given a VectorType object, we can't distinguish between
// 1. vector
// 2. vector literal
// 3. vector reference (vec[vec], mat[vec, scalar] or mat[scalar, vec])
// we need to look at Type.m_typeid to determine which one is the actual type of this variable

// there is no need for a deconstructor since we don't need arrays for type information
// void vectorTypeDeconstructor(VectorType *this);


// VectorLiteralType---------------------------------------------------------------------------------------------
typedef struct struct_gazprea_vector_literal_type {
    TypeID *m_elementTypeIDArray;  // we only need TypeID * instead of Type * because no compound type can be an element of a vector
    int64_t m_length;
} VectorLiteralType;

VectorLiteralType *vectorLiteralTypeMalloc(int64_t length);

void vectorLiteralTypeInitFromVariables(VectorLiteralType *this, int64_t nVars, Variable *varArray);

void vectorLiteralTypeDeconstructor(VectorLiteralType *this);


// MatrixType---------------------------------------------------------------------------------------------
typedef struct struct_gazprea_matrix_type {
    TypeID m_elementTypeID;
    int64_t m_nRow;
    int64_t m_nCol;
} MatrixType;

MatrixType *matrixTypeMalloc();

void matrixTypeInitFromSizes(MatrixType *this, TypeID elementType, int64_t nRow, int64_t nCol);  // SIZE_UNKNOWN for unknown size
void matrixTypeInitFromCopy(MatrixType *this, MatrixType *other);

// void matrixTypeDeconstructor(MatrixType *this);


// MatrixLiteralType---------------------------------------------------------------------------------------------
typedef struct struct_gazprea_matrix_literal_type {
    TypeID *m_elementTypeIDArray;  // nRow * nCol, rectangular, pad shorter rows with typeid_null
    int64_t m_nRow;
    int64_t m_nCol;
} MatrixLiteralType;

void matrixLiteralTypeInitFromVariables(MatrixLiteralType *this, int64_t nRow, int64_t nCol, Variable *varArray);

void matrixLiteralTypeDeconstructor(MatrixLiteralType *this);


// TupleType---------------------------------------------------------------------------------------------
/// TODO