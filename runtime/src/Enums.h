#pragma once

#include <stdint.h>

// This file declares all enums and consts in the runtime library

// #define DEBUG_PRINT

extern const int64_t SIZE_UNKNOWN;
extern const int64_t SIZE_UNSPECIFIED;  // for string only
extern const int8_t DIM_INVALID;  // non-ndarray have no dimension
extern const int8_t DIM_UNSPECIFIED;  // an empty array have unspecified dimension

/// TYPES
typedef enum enum_gazprea_typeid {
    /// spec
    TYPEID_NDARRAY,   // scalar, vector, empty vector, string and matrix
    TYPEID_INTERVAL,  // not a compound type because it is always constant size
    TYPEID_TUPLE,

    /// non-spec
    // basic types
    TYPEID_STREAM_IN,
    TYPEID_STREAM_OUT,
    TYPEID_UNKNOWN,         // unknown type is for declaration/parameter atom with inferred type

    NUM_TYPE_IDS            // number of ids in the enum
} TypeID;

/// Element types are data types of elements for scalar, vector and matrix types
typedef enum enum_gazprea_element_typeid {
    ELEMENT_INTEGER,
    ELEMENT_REAL,
    ELEMENT_BOOLEAN,
    ELEMENT_CHARACTER,
    ELEMENT_NULL,
    ELEMENT_IDENTITY,
    ELEMENT_MIXED,

    NUM_ELEMENT_TYPES
} ElementTypeID;
// mixed element type is used for vector or matrix constructs that can have many types inside the same array temporarily

/// Base types of interval type
typedef enum enum_gazprea_interval_base_typeid {
    INTEGER_BASE_INTERVAL,
    UNSPECIFIED_BASE_INTERVAL,  // the interval alone
} IntervalTypeBaseTypeID;


/// Classifies ndarray
typedef enum enum_gazprea_ndarray_typeid {
    NDARRAY_MIXED,
    NDARRAY_CONCRETE,
    NDARRAY_REFERENCE,
    NDARRAY_NOT_AN_NDARRAY,
    NDARRAY_INVALID,
} NDArrayTypeID;

/// Classifies ndarray index ref
typedef enum enum_gazprea_ndarray_index_ref_typeid {
    NDARRAY_INDEX_REF_SELF,
    NDARRAY_INDEX_REF_1D,
    NDARRAY_INDEX_REF_2D,
    NDARRAY_INDEX_REF_NOT_A_REF,
} NDArrayIndexRefTypeID;


/// Unary/Binary Ops
typedef enum enum_unary_op_code {
    UNARY_PLUS,
    UNARY_MINUS,
    UNARY_NOT,

    NUM_UNARY_OPS
} UnaryOpCode;                          /// INTERFACE

typedef enum enum_binary_op_code {
    BINARY_INDEX,                   // []
    BINARY_RANGE_CONSTRUCT,         // ..
    BINARY_EXPONENT,                // ^
    BINARY_MULTIPLY,                // *
    BINARY_DIVIDE,                  // /
    BINARY_REMAINDER,               // %
    BINARY_DOT_PRODUCT,             // **
    BINARY_PLUS,                    // +
    BINARY_MINUS,                   // -
    BINARY_BY,                      // by

    // condition ops
    BINARY_LT,                      // <
    BINARY_BT,                      // >
    BINARY_LEQ,                     // <=
    BINARY_BEQ,                     // >=
    BINARY_EQ,                      // ==
    BINARY_NE,                      // !=
    BINARY_AND,                     // and
    BINARY_OR,                      // or
    BINARY_XOR,                     // xor

    // concat
    BINARY_CONCAT,                  // ||

    NUM_BINARY_OPS
} BinOpCode;                            /// INTERFACE

typedef enum enum_gazprea_stack_item_type {
    STACK_ITEM_VARIABLE,
    STACK_ITEM_TYPE,
} StackItemType;