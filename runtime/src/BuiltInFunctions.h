#include "Enums.h"
#include "RuntimeVariables.h"

/// INTERFACE
// all the below functions will return a newly created variable, and therefore they need to be freed manually
Variable *BuiltInStreamState();
Variable *BuiltInLength(Variable *vec);
Variable *BuiltInReverse(Variable *vec);
Variable *BuiltInRows(Variable *mat);
Variable *BuiltInColumns(Variable *mat);