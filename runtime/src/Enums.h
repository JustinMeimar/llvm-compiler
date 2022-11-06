#pragma once

#include <stdint.h>

// This file define all enums and consts in the runtime library

const int64_t SIZE_UNKNOWN = -1;

///TYPES
typedef enum enum_gazprea_typeid {
    /// spec
    // basic types
    typeid_integer = 0,
    typeid_real,
    typeid_boolean,
    typeid_character,
    typeid_interval,  // not a compound type because it is always constant size
    // compound types
    typeid_vector,
    typeid_string,
    typeid_matrix,
    typeid_tuple,

    /// non-spec
    // basic types
    typeid_stream_in,
    typeid_stream_out,
    typeid_null,
    typeid_identity,
    typeid_empty_list,      // empty vector or matrix
    typeid_unknown,         // unknown type is for declaration/parameter atom with inferred type
    // compound types
    typeid_vector_literal,  // can have different types for each element, useful only in varDeclarationStatement, converted to vector right away in any other situation
    typeid_matrix_literal,  // rectangular shape, the shorter rows are filled with nulls
    typeid_vector_ref,      // vector/matrix indexed by a vector is a reference to the original vector
    typeid_matrix_ref,      // matrix indexed by two vectors

    NUM_TYPE_IDS            // number of ids in the enum
} TypeID;

typedef enum enum_type_qualifier {
    qualifier_const,
    qualifier_var,

    NUM_QUALIFIERS
} TypeQualifier;

///Unary/Binary Ops
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
} BinaryOpCode;