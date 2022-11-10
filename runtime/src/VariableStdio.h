#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct struct_gazprea_variable Variable;
typedef struct struct_gazprea_type Type;


void typeDebugPrint(Type *this);  // debug print to stdout               /// INTERFACE


void variablePrintToStream(Variable *this, Variable *stream);            /// INTERFACE
void variablePrintToStdout(Variable *this);
void variableDebugPrint(Variable *this);  // debug print to stdout       /// INTERFACE

void variableReadFromStream(Variable *this, Variable *stream);
void variableReadFromStdin(Variable *this);
int32_t readIntegerFromStdin();
float readRealFromStdin();
bool readBooleanFromStdin();
int8_t readCharacterFromStdin();
int32_t getStdinState();