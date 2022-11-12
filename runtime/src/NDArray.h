#pragma once

/**
 * This file handles interfaces for element type math in array type
 * no type checking is enforced in most interfaces
 * The idea of ndarray comes from numpy's handling of different dimension arrays
 */

#include <stdint.h>
#include <stdbool.h>
#include "Enums.h"

typedef struct struct_gazprea_mixed_type_element {
    ElementTypeID m_elementTypeID;
    void *m_element;
} MixedTypeElement;

bool elementIsMixedType(ElementTypeID id);
bool elementIsNullIdentity(ElementTypeID id);
bool elementIsBasicType(ElementTypeID id);
int64_t elementGetSize(ElementTypeID id);

bool elementCanBeCastedFrom(ElementTypeID to, ElementTypeID from);  // does not handle mixed type
bool elementCanBePromotedFrom(ElementTypeID to, ElementTypeID from);
bool elementCanBePromotedBetween(ElementTypeID op1, ElementTypeID op2, ElementTypeID *resultType);  // allow op1 promote to op2 or vice versa
bool elementCanUseUnaryOp(ElementTypeID id, UnaryOpCode opcode);
bool elementBinOpResultType(ElementTypeID id, BinOpCode opcode, ElementTypeID *resultType);

void elementMallocFromPromotion(ElementTypeID resultID, ElementTypeID srcID, void *src, void **result);
void elementMallocFromCast(ElementTypeID resultID, ElementTypeID srcID, void *src, void **result);

// operations that can only be done on the same element type
void elementMallocFromAssignment(ElementTypeID id, void *src, void **result);
void elementMallocFromUnaryOp(ElementTypeID id, UnaryOpCode opcode, void *src, void **result);
// binary operation may not return the same type e.g. 1 == 2
void elementMallocFromBinOp(ElementTypeID operandID, BinOpCode opcode, void *op1, void *op2, void **result);


///------------------------------DIMENSIONLESS ARRAY---------------------------------------------------------------

void *arrayMallocFromNull(ElementTypeID id, int64_t size);
void *arrayMallocFromIdentity(ElementTypeID id, int64_t size);
void *arrayMallocFromElementValue(ElementTypeID id, int64_t size, void *value);
void *arrayMallocFromMemcpy(ElementTypeID id, int64_t size, void *value);

// typed versions of arrayMallocFromElementValue
void *arrayMallocFromBoolValue(int64_t size, bool value);
void *arrayMallocFromCharacterValue(int64_t size, int8_t value);
void *arrayMallocFromIntegerValue(int64_t size, int32_t value);
void *arrayMallocFromRealValue(int64_t size, float value);

void arrayFree(ElementTypeID id, void *arr, int64_t size);

// simple getter/setters
bool arrayGetBoolValue(void *arr, int64_t index);
int8_t arrayGetCharacterValue(void *arr, int64_t index);
int32_t arrayGetIntegerValue(void *arr, int64_t index);
float arrayGetRealValue(void *arr, int64_t index);
void arraySetBoolValue(void *arr, int64_t index, bool value);
void arraySetCharacterValue(void *arr, int64_t index, int8_t value);
void arraySetIntegerValue(void *arr, int64_t index, int32_t value);
void arraySetRealValue(void *arr, int64_t index, float value);


/// unary op
void arrayMallocFromUnaryOp(ElementTypeID id, UnaryOpCode opcode, void *src, int64_t length, void **result);

// make sure the binary op can be handled by dimensionless array before calling
// this function does not handle matrix multiplication
/// binary op
bool arrayBinopResultType(ElementTypeID id, BinOpCode opcode, ElementTypeID *resultType, bool* resultCollapseToScalar);
void arrayMallocFromBinOp(ElementTypeID id, BinOpCode opcode, void *op1, int64_t op1Size, void *op2, int64_t op2Size, void **result, int64_t *resultSize);

/// casting and promotion
void arrayMallocFromCast(ElementTypeID resultID, ElementTypeID srcID, int64_t size, void *src, void **result);
void arrayMallocFromPromote(ElementTypeID resultID, ElementTypeID srcID, int64_t size, void *src, void **result);
bool arrayMixedElementCanBePromotedToSameType(MixedTypeElement *mixedArray, int64_t size, ElementTypeID *resultType);

/// binary op: matrix multiplication
// n * m matrix multiply by m * k matrix to produce a n * k matrix
void arrayMallocFromMatrixMultiplication(ElementTypeID id, void *op1, void *op2, int64_t n, int64_t m, int64_t k, void **result);
void arrayMallocFromVectorResize(ElementTypeID id, void *old, int64_t oldSize, int64_t newSize, void **result);
void arrayMallocFromMatrixResize(ElementTypeID id, void *old, int64_t oldNRow, int64_t oldNCol, int64_t newNRow, int64_t newNCol, void **result);