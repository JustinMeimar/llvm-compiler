#pragma once

/**
 * This file handles interfaces for element type math in array type
 * no type checking is enforced in most interfaces
 */

#include <stdint.h>
#include <stdbool.h>
#include "Enums.h"

bool elementIsMixedType(ElementTypeID id);
bool elementIsBasicType(ElementTypeID id);
bool elementSize(ElementTypeID id);

bool elementCanBeCastedFrom(ElementTypeID to, ElementTypeID from);
bool elementCanBePromotedFrom(ElementTypeID to, ElementTypeID from);
bool elementCanBePromotedBetween(ElementTypeID op1, ElementTypeID op2, ElementTypeID *resultType);  // allow op1 promote to op2 or vice versa
bool elementCanUseUnaryOp(ElementTypeID id, UnaryOpCode opcode);
bool elementBinOpResultType(ElementTypeID id, BinOpCode opcode, ElementTypeID *resultType, bool* resultMustBeScalar);

void elementPromotion(ElementTypeID resultID, ElementTypeID srcID, void *src, void *result);
void elementCast(ElementTypeID resultID, ElementTypeID srcID, void *src, void *result);

// operations that can only be done on the same element type
void elementAssignment(ElementTypeID id, void *src, void *result);
void elementUnaryOp(ElementTypeID id, UnaryOpCode opcode, void *src, void *result);
// binary operation may not return the same type e.g. 1 == 2
void elementBinOp(ElementTypeID id, BinOpCode opcode, void *op1, void *op2, void *result);


///------------------------------DIMENSIONLESS ARRAY---------------------------------------------------------------

void *arrayMallocFromNull(ElementTypeID id, int64_t size);
void *arrayMallocFromIdentity(ElementTypeID id, int64_t size);
void *arrayMallocFromElementValue(ElementTypeID id, int64_t size, void *value);
void arrayMallocFromUnaryOp(ElementTypeID id, UnaryOpCode opcode, void *src, int64_t length, void *result);
// make sure the binary op can be handled by dimensionless array before calling this function
void arrayMallocFromBinOp(ElementTypeID id, BinOpCode opcode, void *op1, int64_t op1Size, void *op2, int64_t op2Size, void *result, int64_t *resultSize);

bool arrayMixedElementCanPromoteToSameType(ElementTypeID *idArray, int64_t size, ElementTypeID *resultType);
void arrayMixedElementPromoteToTargetType(ElementTypeID *idArray, void *valueArray, int64_t size, ElementTypeID targetType, void *result);
