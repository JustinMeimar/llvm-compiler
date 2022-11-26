#pragma once

#include <stdint.h>
#include <bits/types/FILE.h>
#include "Bool.h"
#include "NDArray.h"

#ifdef DEBUG_PRINT
extern bool reentry;
#endif

typedef struct struct_gazprea_variable Variable;
typedef struct struct_gazprea_type Type;

extern int32_t global_stream_state;  // 0, 1, 2 = success, error, eof
void typeDebugPrint(Type *this);  // debug print to stdout               /// INTERFACE
void typeInitDebugPrint(Type *this, char *msg);

void elementPrintToFile(FILE *fd, ElementTypeID id, void *value);
void variablePrintToFile(FILE *fd, Variable *this);
void variablePrintToStream(Variable *this, Variable *stream);            /// INTERFACE
void variablePrintToStdout(Variable *this);
void variableDebugPrint(Variable *this);  // debug print to stdout       /// INTERFACE
void variableInitDebugPrint(Variable *this, char *msg);

void variableReadFromStream(Variable *this, Variable *stream);
void variableReadFromStdin(Variable *this);
int32_t readIntegerFromStdin();
float readRealFromStdin();
bool readBooleanFromStdin();
int8_t readCharacterFromStdin();
int32_t getStdinState();