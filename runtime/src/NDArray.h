#pragma once

/**
 * This file handles interfaces for element type math in array type
 * no type checking is enforced in most interfaces
 */

#include <stdint.h>
#include <stdbool.h>
#include "Enums.h"

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
void elementMallocFromBinOp(ElementTypeID id, BinOpCode opcode, void *op1, void *op2, void **result);


///------------------------------DIMENSIONLESS ARRAY---------------------------------------------------------------

void *arrayMallocFromNull(ElementTypeID id, int64_t size);
void *arrayMallocFromIdentity(ElementTypeID id, int64_t size);
void *arrayMallocFromElementValue(ElementTypeID id, int64_t size, void *value);

// typed versions of arrayMallocFromElementValue
void *arrayMallocFromBoolValue(int64_t size, bool value);
void *arrayMallocFromCharacterValue(int64_t size, int8_t value);
void *arrayMallocFromIntegerValue(int64_t size, int32_t value);
void *arrayMallocFromRealValue(int64_t size, float value);

// simple getter/setters
bool arrayGetBoolValue(void *arr, int64_t index);
int8_t arrayGetCharacterValue(void *arr, int64_t index);
int32_t arrayGetIntegerValue(void *arr, int64_t index);
float arrayGetRealValue(void *arr, int64_t index);
void arraySetBoolValue(void *arr, int64_t index, bool value);
void arraySetCharacterValue(void *arr, int64_t index, int8_t value);
void arraySetIntegerValue(void *arr, int64_t index, int32_t value);
void arraySetRealValue(void *arr, int64_t index, float value);

void arrayMallocFromUnaryOp(ElementTypeID id, UnaryOpCode opcode, void *src, int64_t length, void **result);

// make sure the binary op can be handled by dimensionless array before calling
// this function does not handle matrix multiplication
bool arrayBinopResultType(ElementTypeID id, BinOpCode opcode, ElementTypeID *resultType, bool* resultCollapseToScalar);
void arrayMallocFromBinOp(ElementTypeID id, BinOpCode opcode, void *op1, int64_t op1Size, void *op2, int64_t op2Size, void **result, int64_t *resultSize);

bool arrayMixedElementCanBePromotedToSameType(ElementTypeID *idArray, int64_t size, ElementTypeID *resultType);
void arrayMixedElementPromoteToTargetType(ElementTypeID *idArray, void **valueArray, int64_t size, ElementTypeID targetType, void **result);
