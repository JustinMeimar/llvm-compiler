#pragma once
#include "RuntimeVariables.h"

typedef struct struct_gazprea_variable Variable;
typedef struct struct_gazprea_type Type;

///------------------------------TYPE LITERAL---------------------------------------------------------------

// ndarray
void typeInitFromBooleanScalar(Type *this);
void typeInitFromIntegerScalar(Type *this);
void typeInitFromRealScalar(Type *this);
void typeInitFromCharacterScalar(Type *this);
/**
 * Gives the result Type for baseType[size]; will perform type checking during intialization
 * @param this the Type object to be initialized
 * @param size size of vector; pass in NULL if the size is inferred ("*")
 * @param baseType base type of the vector
 */
void typeInitFromVectorSizeSpecification(Type *this, Variable *size, Type *baseType);  // for vector and string
void typeInitFromMatrixSizeSpecification(Type *this, Variable *nRow, Variable *nCol, Type *baseType);  // baseType[nRow, nCol]
// string
void typeInitFromUnspecifiedString(Type *this);
// interval
void typeInitFromUnspecifiedInterval(Type *this);
void typeInitFromIntegerInterval(Type *this);
// tuple
/**
 * Initialize a Type object to describe a tuple type
 * Note the method will not directly use but will copy the typeArray or stridArray provided,
 * so we need to take care of the deallocation after calling this method
 * @param this the allocated type object pointer
 * @param nField the number of fields the tuple has in total
 * @param typeArray an array of type pointers, one for each field in the tuple
 * @param stridArray an array of mapped string ids, stridArray[i] should be the name of field in position i+1, or -1 if the field has no name
 */
void typeInitFromTupleType(Type *this, int64_t nField, Type **typeArray, int64_t *stridArray);
// unknown type
void typeInitFromUnknownType(Type *this);

///------------------------------TUPLE---------------------------------------------------------------

void variableSetStrIdArray(Variable *this, int64_t *stridArray);  // for tuple var function parameter
Type *variableGetType(Variable *this);
Type *variableSwapType(Variable *this, Type *newType);  // alternative way

///------------------------------VARIABLE LITERAL---------------------------------------------------------------

// LLVM::Value *v = llvmfunction->call(variableMalloc());
// llvmfunction->call(variableInit...(v, ...));

// ndarray
void variableInitFromBooleanScalar(Variable *this, bool value);
void variableInitFromIntegerScalar(Variable *this, int32_t value);
void variableInitFromRealScalar(Variable *this, float value);
void variableInitFromCharacterScalar(Variable *this, int8_t value);
void variableInitFromNullScalar(Variable *this);
void variableInitFromIdentityScalar(Variable *this);
void variableInitFromVectorLiteral(Variable *this, int64_t nVars, Variable **vars);  // could be either vector or matrix literal
// string
void variableInitFromString(Variable *this, int64_t strLength, int8_t *str);
// tuple
/**
 * Initialize a tuple variable from a tuple literal construct
 * we should manually deallocate the vars after calling this function
 * @param this the allocated variable pointer
 * @param nField the number of fields the tuple has in total
 * @param vars a variable pointer array, one variable for each field
 */
void variableInitFromTupleLiteral(Variable *this, int64_t nField, Variable **vars);
// stream
void variableInitFromStdInput(Variable *this);
void variableInitFromStdOutput(Variable *this);
// empty array
void variableInitFromEmptyArray(Variable *this);

/**
 * Create a vector/matrix; creates empty matrix if 0 variables are offered
 * @param this the variable to initialize as the result vector or matrix
 * @param nVars the number of variables in the vars, for matrix this would be number of rows
 * @param vars element in the vector; for matrix this would be vector literals
 */
void variableInitFromGeneratorArray(Variable *this, int64_t nVars, Variable **vars);
/**
 * Creates a tuple with nFilter + 1 fields from a filter construct
 * @param this The variable to initialize as tuple
 * @param nFilter the number of filter expressions in the filter
 * @param domainExpr the single domain expression that this filter loops on
 * @param accept a (flattened) matrix, an element accept[i][j] gives whether the result of evaluating the jth expression on ith domain variable is true
 */
void variableInitFromFilterArray(Variable *this, int64_t nFilter, Variable *domainExpr, const bool *accept);