#include "VariableStdio.h"
#include "RuntimeVariables.h"
#include "RuntimeErrors.h"
#include "Literal.h"
#include "stdlib.h"
#include "ctype.h"
#include "limits.h"

void typeDebugPrint(Type *this) {
    printf("<");
    switch (this->m_typeId) {
        case TYPEID_NDARRAY:
            printf("ndarray");
            break;
        case TYPEID_STRING:
            printf("string");
            break;
        case TYPEID_INTERVAL:
            printf("interval");
            break;
        case TYPEID_TUPLE:
            printf("tuple");
            break;
        case TYPEID_STREAM_IN:
            printf("stream_in");
            break;
        case TYPEID_STREAM_OUT:
            printf("stream_out");
            break;
        case TYPEID_EMPTY_ARRAY:
            printf("empty_arr");
            break;
        case TYPEID_UNKNOWN:
            printf("unknown");
            break;
        default:
            printf("invalid");
            break;
    }
    switch (this->m_typeId) {
        case TYPEID_NDARRAY:
        case TYPEID_STRING: {
            ArrayType *CTI = this->m_compoundTypeInfo;
            printf(",eid=%d,nDim=%d", CTI->m_elementTypeID, CTI->m_nDim);
            for (int8_t i = 0; i < CTI->m_nDim; i++) {
                printf(",dim[%d]=%ld", i, CTI->m_dims[i]);
            }
        } break;
        case TYPEID_TUPLE: {
            TupleType *CTI = this->m_compoundTypeInfo;
            printf(",nField=%ld,idToIdx=[", CTI->m_nField);
            for (int64_t i = 0; i < CTI->m_nField; i++) {
                printf("%ld", CTI->m_idxToStrid[i]);
                if (i != CTI->m_nField - 1)
                    printf(",");
            }
            printf("]{");
            // recursively print the types of each field
            for (int64_t i = 0; i < CTI->m_nField; i++) {
                typeDebugPrint(&(CTI->m_fieldTypeArr[i]));
            }
            printf("}");
        } break;
        default: break;
    }
    printf(">");
}



void variablePrintToStream(Variable *this, Variable *stream) {
    if (stream->m_type->m_typeId != TYPEID_STREAM_OUT) {
        targetTypeError(stream->m_type, "Attempt to print a variable to:");
    }
    variablePrintToStdout(this);
}

void elementPrintToStdout(ElementTypeID id, void *value) {
    switch (id) {
        case ELEMENT_INTEGER:
            printf("%d", *(int32_t *)value);
            break;
        case ELEMENT_REAL:
            printf("%g", *(float *)value);
            break;
        case ELEMENT_BOOLEAN:
            if (*(bool *)value) {
                printf("T");
            } else {
                printf("F");
            }
            break;
        case ELEMENT_CHARACTER:
            printf("%c", *(int8_t *)value);
            break;
        case ELEMENT_NULL:
            printf("%c", 0x00);
            break;
        case ELEMENT_IDENTITY:
            printf("%c", 0x01);
            break;
        default:
            errorAndExit("Unexpected element id when printing to stdout!"); break;
    }
}

void variablePrintToStdout(Variable *this) {
    Type *type = this->m_type;
    if (type->m_typeId == TYPEID_STRING) {
        ArrayType *CTI = type->m_compoundTypeInfo;
        int64_t size = arrayTypeGetTotalLength(CTI);
        int8_t *str = this->m_data;
        for (int64_t i = 0; i < size; i++) {
            printf("%c", str[i]);
        }
    } else if (type->m_typeId == TYPEID_NDARRAY) {
        ArrayType *CTI = type->m_compoundTypeInfo;
        ElementTypeID eid = CTI->m_elementTypeID;
        char *dataPos = this->m_data;
        if (CTI->m_nDim == 0)  // scalar
            elementPrintToStdout(eid, dataPos);
        else {
            int64_t elementSize = arrayTypeElementSize(CTI);
            int64_t *dims = CTI->m_dims;
            if (CTI->m_nDim == 1) {  // vector
                printf("[");
                for (int64_t i = 0; i < dims[0]; i++) {
                    elementPrintToStdout(eid, dataPos + i * elementSize);
                    if (i != dims[0] - 1) {
                        printf(" ");
                    }
                }
                printf("]");
            } else {  // matrix
                printf("[");
                for (int64_t i = 0; i < dims[0]; i++) {
                    printf("[");
                    for (int64_t j = 0; j < dims[1]; j++) {
                        if (j != dims[1] - 1) {
                            printf(" ");
                            elementPrintToStdout(eid, dataPos + (i * dims[1] + j) * elementSize);
                        }
                    }
                    printf("]");
                    if (i != dims[0] - 1) {
                        printf(" ");
                    }
                }
                printf("]");
            }
        }
    }
}

void variableDebugPrint(Variable *this) {
    printf("(Var");
    typeDebugPrint(this->m_type);
    switch(this->m_type->m_typeId) {
        case TYPEID_NDARRAY:
        case TYPEID_STRING: {
            // use the spec print method
            variablePrintToStdout(this);
        } break;
        case TYPEID_INTERVAL: {
            int32_t *interval = this->m_data;
            printf("(%d..%d)", interval[0], interval[1]);
        } break;
        case TYPEID_TUPLE:{
            // print recursively
            Variable **vars = this->m_data;
            TupleType *CTI = this->m_type->m_compoundTypeInfo;
            for (int64_t i = 0; i < CTI->m_nField; i++) {
                variableDebugPrint(vars[i]);
            }
        } break;
        default: break;
    }
    printf(")");
}

///------------------------------STREAM_STD_INPUT---------------------------------------------------------------

void variableReadFromStdin(Variable *this) {
    TypeID tid = this->m_type->m_typeId;
    if (tid != TYPEID_NDARRAY) {
        targetTypeError(this->m_type, "Attempt to read from stdin into a variable of type:");
    }
    ArrayType *CTI = this->m_type->m_compoundTypeInfo;
    Variable *rhs = variableMalloc();
    switch (CTI->m_elementTypeID) {
        case ELEMENT_INTEGER:
            variableInitFromIntegerScalar(rhs, readIntegerFromStdin()); break;
        case ELEMENT_REAL:
            variableInitFromRealScalar(rhs, readRealFromStdin()); break;
        case ELEMENT_BOOLEAN:
            variableInitFromBooleanScalar(rhs, readBooleanFromStdin()); break;
        case ELEMENT_CHARACTER:
            variableInitFromCharacterScalar(rhs, readCharacterFromStdin()); break;
        default:
            targetTypeError(this->m_type, "Attempt to read from stdin into a variable of type:"); break;
    }
    variableAssignment(this, rhs);
    variableDestructThenFree(rhs);
}

int32_t stream_state = 0;  // 0, 1, 2 = success, error, eof
#define BUFFER_SIZE 1024

/**
 * only a section of the buffer will be used at any given time
 * the section is every character starting from cur_pos up to but not include the valid_until pos
 */
char circular_buffer[BUFFER_SIZE];  // only a section of the buffer will be used at any given time
int last_successful_read = 0;  // the position right after the last successful read
int cur_pos = 0;  // the next character will start reading from here
int valid_until = 0;

// https://en.cppreference.com/w/c/io/feof
// return EOF on unsuccessful read, otherwise the result can be converted to char
int readNextChar() {
    if (cur_pos == valid_until) {
        int ch = fgetc(stdin);
        if (ch != EOF) {
            cur_pos = (cur_pos + 1) % BUFFER_SIZE;
            valid_until = (valid_until + 1) % BUFFER_SIZE;
            if (cur_pos == last_successful_read) {
                // emit an error
                errorAndExit("Error: Attempt to read an input longer than the input buffer length 1024!");
            }
        }
        return ch;
    } else {
        // just read from the saved buffer
        char ch = circular_buffer[cur_pos];
        cur_pos = (cur_pos + 1) % BUFFER_SIZE;
        return ch;
    }
}

void rewindInputBuffer() {
    cur_pos = last_successful_read;
}

// on a successful read, we want to register the new rewind point
void updateRewindPoint(int newRewindPoint) {
    last_successful_read = newRewindPoint;
}

// Each of the interface below should always
// 1. update rewind point when success; rewind when fail
// 2. set stream state regardless of success or failure
// 3. return correct result when success; return null when fail

int32_t readIntegerFromStdin() {
    int ch = ' ';
    while (isspace(ch)) {
        ch = readNextChar();
        if (ch == EOF) {
            rewindInputBuffer();
            stream_state = 2;
            return 0;
        }
    }
    long sign;
    long integer;
    if (ch == '+') {
        sign = 1;
        integer = 0;
    } else if (ch == '-') {
        sign = -1;
        integer = 0;
    } else if (!isdigit(ch)) {
        rewindInputBuffer();
        stream_state = 1;
        return 0;
    } else {
        sign = 1;
        integer = ch - '0';
    }

    // now read the rest of the integer literal until we see a non-digit character
    while (true) {
        int temp = cur_pos;
        ch = readNextChar();
        if (isdigit(ch)) {
            integer = integer * 10 + ch - '0';
            if (sign * integer < INT32_MIN || sign * integer > INT32_MAX) {  // literal is too large
                rewindInputBuffer();
                stream_state = 1;
                return 0;
            }
        } else if (ch == EOF) {
            rewindInputBuffer();
            stream_state = 2;
            return 0;
        } else if (isspace(ch)) {  // successful read
            updateRewindPoint(temp);
            rewindInputBuffer();
            stream_state = 0;
            return (int32_t)(sign * integer);
        } else {  // is failure
            rewindInputBuffer();
            stream_state = 1;
            return 0;
        }
    }
}

float readRealFromStdin() {
    // TODO: read float from stdin
    return 0.0f;
}

bool readBooleanFromStdin() {
    // read until a non-whitespace character is encountered, then read one more character to see if the boolean is valid
    while (true) {
        int ch = readNextChar();
        if (ch == EOF) {
            rewindInputBuffer();
            stream_state = 2;
            return false;
        } else if (isspace(ch)) {
            // do nothing
        } else if (ch == 'T' || ch == 'F') {
            bool result = ch == 'T';
            int temp = cur_pos;

            ch = readNextChar();
            if (isspace(ch)) {  // success
                updateRewindPoint(temp);  // don't actually eat the space
                rewindInputBuffer();
                stream_state = 0;
                return result;
            } else {
                rewindInputBuffer();
                stream_state = (ch == EOF) ? 2 : 1;
                return false;
            }
        } else {
            rewindInputBuffer();
            stream_state = 1;
            return false;
        }
    }
}

int8_t readCharacterFromStdin() {
    int ch = readNextChar();
    updateRewindPoint(cur_pos);  // read character should always success
    stream_state = 0;

    if (ch == EOF) {
        return -1;
    } else {
        return (int8_t) ch;
    }
}

int32_t getStdinState() {
    return stream_state;
}