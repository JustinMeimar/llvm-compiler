#pragma once

#include <stdint.h>

// This file declares all enums and consts in the runtime library

const int64_t SIZE_UNKNOWN;
const int64_t SIZE_UNSPECIFIED;  // for string only

/// TYPES
typedef enum enum_gazprea_typeid {
    /// spec
    typeid_ndarray,
    typeid_string,
    typeid_interval,  // not a compound type because it is always constant size
    typeid_tuple,

    /// non-spec
    // basic types
    typeid_stream_in,
    typeid_stream_out,
    typeid_empty_array,     // empty vector or matrix
    typeid_unknown,         // unknown type is for declaration/parameter atom with inferred type

    NUM_TYPE_IDS            // number of ids in the enum
} TypeID;

/// Element types are data types of elements for scalar, vector and matrix types
typedef enum enum_gazprea_element_typeid {
    element_integer,
    element_real,
    element_boolean,
    element_character,
    element_null,
    element_identity,
    element_mixed,

    NUM_ELEMENT_TYPES
} ElementTypeID;
// mixed element type is used for vector or matrix constructs that can have many types inside the same array temporarily

/// Base types of interval type
typedef enum enum_gazprea_interval_base_typeid {
    integer_base_interval,
    unspecified_base_interval,  // the interval alone
} IntervalTypeBaseTypeID;


/// Unary/Binary Ops
typedef enum enum_unary_op_code {
    unary_plus,
    unary_minus,
    unary_not,

    NUM_UNARY_OPS
} UnaryOpCode;

typedef enum enum_binary_op_code {
    binary_index,                   // []
    binary_range_construct,         // ..
    binary_exponent,                // ^
    binary_multiply,                // *
    binary_divide,                  // /
    binary_remainder,               // %
    binary_dot_product,             // **
    binary_plus,                    // +
    binary_minus,                   // -
    binary_by,                      // by

    // condition ops
    binary_lt,                      // <
    binary_bt,                      // >
    binary_leq,                     // <=
    binary_beq,                     // >=
    binary_eq,                      // ==
    binary_ne,                      // !=
    binary_and,                     // and
    binary_or,                      // or
    binary_xor,                     // xor

    // concat
    binary_concat,                  // ||

    NUM_BINARY_OPS
} BinOpCode;