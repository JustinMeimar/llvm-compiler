#include <math.h>
#include "VariableStdio.h"
#include "RuntimeVariables.h"
#include "RuntimeErrors.h"
#include "Literal.h"
#include "stdlib.h"
#include "ctype.h"
#include "limits.h"
#include "NDArrayVariable.h"

void typeDebugPrint(Type *this) {
    FILE *fd = stderr;
    fprintf(fd, "<");
    switch (this->m_typeId) {
        case TYPEID_NDARRAY:
            fprintf(fd, "ndarray");
            break;
        case TYPEID_INTERVAL:
            fprintf(fd, "interval");
            break;
        case TYPEID_TUPLE:
            fprintf(fd, "tuple");
            break;
        case TYPEID_STREAM_IN:
            fprintf(fd, "stream_in");
            break;
        case TYPEID_STREAM_OUT:
            fprintf(fd, "stream_out");
            break;
        case TYPEID_UNKNOWN:
            fprintf(fd, "unknown");
            break;
        default:
            fprintf(fd, "invalid");
            break;
    }
    switch (this->m_typeId) {
        case TYPEID_NDARRAY: {
            ArrayType *CTI = this->m_compoundTypeInfo;
            fprintf(fd, ",eid=%d,nDim=%d,isString=%d,isOwned=%d,isRef=%d,isSelfRef=%d", CTI->m_elementTypeID, CTI->m_nDim,
                CTI->m_isString, CTI->m_isOwned, CTI->m_isRef, CTI->m_isSelfRef);
            for (int8_t i = 0; i < CTI->m_nDim; i++) {
                fprintf(fd, ",dim[%d]=%ld", i, CTI->m_dims[i]);
            }
        } break;
        case TYPEID_TUPLE: {
            TupleType *CTI = this->m_compoundTypeInfo;
            fprintf(fd, ",nField=%ld,idToIdx=[", CTI->m_nField);
            for (int64_t i = 0; i < CTI->m_nField; i++) {
                fprintf(fd, "%ld", CTI->m_idxToStrid[i]);
                if (i != CTI->m_nField - 1)
                    fprintf(fd, ",");
            }
            fprintf(fd, "]{");
            // recursively print the types of each field
            for (int64_t i = 0; i < CTI->m_nField; i++) {
                typeDebugPrint(&(CTI->m_fieldTypeArr[i]));
            }
            fprintf(fd, "}");
        } break;
        default: break;
    }
    fprintf(fd, ">");
}

void typeInitDebugPrint(Type *this, char *msg) {
    FILE *fd = stderr;
    fprintf(fd, "(init type %p '", (void *)this);
    fprintf(fd, "%s", msg);
    fprintf(fd, "')");
    typeDebugPrint(this);
    fprintf(fd, "\n");
}


void variablePrintToStream(Variable *this, Variable *stream) {
    if (stream->m_type->m_typeId != TYPEID_STREAM_OUT) {
        singleTypeError(stream->m_type, "Attempt to print a variable to:");
    }
    variablePrintToStdout(this);
}

void elementPrintToFile(FILE *fd, ElementTypeID id, void *value) {
    switch (id) {
        case ELEMENT_INTEGER:
            fprintf(fd, "%d", *(int32_t *)value);
            break;
        case ELEMENT_REAL:
            fprintf(fd, "%g", *(float *)value);
            break;
        case ELEMENT_BOOLEAN:
            if (*(bool *)value) {
                fprintf(fd, "T");
            } else {
                fprintf(fd, "F");
            }
            break;
        case ELEMENT_CHARACTER:
            fprintf(fd, "%c", *(int8_t *)value);
            break;
        case ELEMENT_NULL:
            fprintf(fd, "%c", 0x00);
            break;
        case ELEMENT_IDENTITY:
            fprintf(fd, "%c", 0x01);
            break;
        default:
            errorAndExit("Unexpected element id when printing to stdout!"); break;
    }
}

void variablePrintToFile(FILE *fd, Variable *this) {
    // only arrays and string can be printed
    Variable *temp = variableConvertLiteralAndRefToConcreteArray(this);
    if (temp) {
        variablePrintToFile(fd, temp);
        variableDestructThenFree(temp);
        return;
    }

    if (typeIsEmptyArray(this->m_type)) {
        fprintf(fd, "[]");
        return;
    }

    Type *type = this->m_type;
    if (type->m_typeId == TYPEID_NDARRAY) {
        ArrayType *CTI = type->m_compoundTypeInfo;

        if (CTI->m_isString) {
            int64_t size = arrayTypeGetTotalLength(CTI);
            for (int64_t i = 0; i < size; i++) {
                int8_t *ch = variableNDArrayGet(this, i);
                fprintf(fd, "%c", *ch);
            }
        } else {
            ElementTypeID eid = CTI->m_elementTypeID;
            if (CTI->m_nDim == 0)  // scalar
                elementPrintToFile(fd, eid, variableNDArrayGet(this, 0));
            else {
                int64_t elementSize = arrayTypeElementSize(CTI);
                int64_t *dims = CTI->m_dims;
                if (CTI->m_nDim == 1) {  // vector
                    fprintf(fd, "[");
                    for (int64_t i = 0; i < dims[0]; i++) {
                        elementPrintToFile(fd, eid, variableNDArrayGet(this, i));
                        if (i != dims[0] - 1) {
                            fprintf(fd, " ");
                        }
                    }
                    fprintf(fd, "]");
                } else {  // matrix
                    fprintf(fd, "[");
                    for (int64_t i = 0; i < dims[0]; i++) {
                        fprintf(fd, "[");
                        for (int64_t j = 0; j < dims[1]; j++) {
                            elementPrintToFile(fd, eid, variableNDArrayGet(this, i * dims[1] + j));
                            if (j != dims[1] - 1) {
                                fprintf(fd, " ");
                            }
                        }
                        fprintf(fd, "]");
                        if (i != dims[0] - 1) {
                            fprintf(fd, " ");
                        }
                    }
                    fprintf(fd, "]");
                }
            }
        }
    }
}

void variablePrintToStdout(Variable *this) {
    variablePrintToFile(stdout, this);
}

void variableDebugPrint(Variable *this) {
    fprintf(stderr, "(Var(blockScoped=%d)", this->m_isBlockScoped);
    typeDebugPrint(this->m_type);
    switch(this->m_type->m_typeId) {
        case TYPEID_NDARRAY: {
            // use the spec print method
            variablePrintToFile(stderr, this);
        } break;
        case TYPEID_INTERVAL: {
            int32_t *interval = this->m_data;
            fprintf(stderr, "(%d..%d)", interval[0], interval[1]);
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
    fprintf(stderr, ")");
}

void variableInitDebugPrint(Variable *this, char *msg) {
    FILE *fd = stderr;
    fprintf(fd, "(init var %p '", (void *)this);
    fprintf(fd, "%s", msg);
    fprintf(fd, "')");
    variableDebugPrint(this);
    fprintf(fd, "\n");
}

///------------------------------STREAM_STD_INPUT---------------------------------------------------------------

void variableReadFromStdin(Variable *this) {
    TypeID tid = this->m_type->m_typeId;
    if (tid != TYPEID_NDARRAY) {
        singleTypeError(this->m_type, "Attempt to read from stdin into a variable of type:");
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
            singleTypeError(this->m_type, "Attempt to read from stdin into a variable of type:"); break;
    }
    variableAssignment(this, rhs);
    variableDestructThenFree(rhs);
}

///------------------------------HELPERS---------------------------------------------------------------

int32_t global_stream_state = 0;  // 0, 1, 2 = success, error, eof
#define BUFFER_SIZE 1024

/**
 * only a section of the buffer will be used at any given time
 * the section is every character starting from cur_pos up to but not include the valid_until pos
 */
char circular_buffer[BUFFER_SIZE];  // only a section of the buffer will be used at any given time
char token_buffer[BUFFER_SIZE];  // holds the result of readNextToken()
int result_token_length = 0;
int last_successful_read = 0;  // the position right after the last successful read
int cur_pos = 0;  // the next character will start reading from here
int valid_until = 0;


int32_t getStdinState() { return global_stream_state; }
int getNextPos(int pos) { return (pos + 1) % BUFFER_SIZE; }
int getPrevPos(int pos) { return (pos + BUFFER_SIZE - 1) % BUFFER_SIZE; }
void rewindInputBuffer() { cur_pos = last_successful_read; }
// on a successful read, we want to register the new rewind point
void updateRewindPoint(int newRewindPoint) { last_successful_read = newRewindPoint; }

// https://en.cppreference.com/w/c/io/feof
// return EOF on unsuccessful read, otherwise the result can be converted to char
int readNextChar() {
    if (cur_pos == valid_until) {
        int ch = fgetc(stdin);
        if (ch != EOF) {
            circular_buffer[cur_pos] = (char)ch;
            cur_pos = getNextPos(cur_pos);
            valid_until = cur_pos;
            if (cur_pos == last_successful_read) {
                // emit an error
                errorAndExit("Error: Attempt to read an input longer than the input buffer length 1024!");
            }
        }
        return ch;
    } else {
        // just read from the saved buffer
        char ch = circular_buffer[cur_pos];
        cur_pos = getNextPos(cur_pos);
        return ch;
    }
}

// will malloc for the token and return one character after the token's last character
// e.g. this will return a space on success read, 2 on eof and 1 on buffer size limit met
int readNextToken() {
    int ch = ' ';
    while (isspace(ch)) {
        if (getNextPos(cur_pos) == last_successful_read) {
            // too many whitespaces
            result_token_length = 0;
            return 1;
        }
        ch = readNextChar();
        if (ch == EOF) {  // encounters EOF without seeing a non-space character
            // no token read
            result_token_length = 0;
            return 2;
        }
    }
    // read the entire token, and stop when the next space is encountered
    int n = 0;
    while (true) {
        token_buffer[n] = (char)ch;
        n += 1;
        if (getNextPos(cur_pos) == last_successful_read) {
            // token too long
            result_token_length = n;
            return 1;
        }
        ch = readNextChar();
        if (ch == EOF || isspace(ch)) {
            // success, return the token
            result_token_length = n;
            return ch == EOF ? 2 : 0;
        }
        // otherwise keep going
    }
}

///------------------------------STDIN FOR BASIC TYPES---------------------------------------------------------------

// Each of the interface below should always
// 1. update rewind point when success; rewind when fail
// 2. set stream state regardless of success or failure
// 3. return correct result when success; return null when fail

int32_t readIntegerFromStdin() {
    int result = readNextToken();
    if (result == 2 && result_token_length == 0) {  // eof
        rewindInputBuffer();
        global_stream_state = 2;
        return false;
    } else if (result == 1) {  // buffer overflow
        errorAndExit("Error: Attempt to read an input longer than the input buffer length 1024!");
    } else {
        char ch = token_buffer[0];
        long sign;
        long integer;
        if (ch == '+' || ch == '-') {
            sign = ch == '+' ? 1 : -1;
            integer = 0;
            if (result_token_length == 1) {  // fail, a single sign is not an integer
                rewindInputBuffer();
                global_stream_state = 1;
                return 0;
            }
        } else if (!isdigit(ch)) {
            rewindInputBuffer();
            global_stream_state = 1;
            return 0;
        } else {
            sign = 1;
            integer = ch - '0';
        }

        int buffer_pos = 1;
        // now read the rest of the integer literal until we see a non-digit character
        while (buffer_pos < result_token_length) {
            ch = token_buffer[buffer_pos];
            buffer_pos += 1;
            if (isdigit(ch)) {
                integer = integer * 10 + ch - '0';
            }
            if (!isdigit(ch) || sign * integer < INT32_MIN || sign * integer > INT32_MAX) {  // is failure
                rewindInputBuffer();
                global_stream_state = 1;
                return 0;
            }
        }
        // success
        updateRewindPoint(result == 2 ? cur_pos : getPrevPos(cur_pos));
        rewindInputBuffer();
        global_stream_state = 0;
        return (int32_t)(sign * integer);
    }
    errorAndExit("This should not happen!");
}

float readRealFromStdin() {
    int result = readNextToken();
    if (result == 2 && result_token_length == 0) {  // eof
        rewindInputBuffer();
        global_stream_state = 2;
        return false;
    } else if (result == 1) {  // buffer overflow
        errorAndExit("Error: Attempt to read an input longer than the input buffer length 1024!");
    } else {
        // represents the current progress in reading the real number
        // 0 = expecting the integer part, 'e' or '.'
        // 1 = just see the '.' but did not see 'e'
        // 2 = after seeing 'e' but have not seen '+' '-' or first digit following 'e'
        // 3 = after first char after 'e'
        int state = 0;

        // read the first character
        char ch = token_buffer[0];
        float sign;
        float value;
        int mantissa_length = 0;
        int exp = 0;
        int expsign = 1;
        bool see_digit_before_exp = false;  // a valid float should have at least one digit before the exponent part
        bool see_digit_after_exp = true;  // if 'e' is seen then there must be a digit after that
        bool see_e_or_dot = false;  // a float must have 'e' or '.' to be valid float
        if (ch == '+' || ch == '-') {
            sign = ch == '+' ? 1.0f : -1.0f;
            value = 0.0f;
        } else if (isdigit(ch)) {
            sign = 1.0f;
            value = (float) (ch - '0');
            see_digit_before_exp = true;
        } else if (ch == '.') {
            sign = 1.0f;
            value = 0.0f;
            state = 1;
            see_e_or_dot = true;
        } else {
            rewindInputBuffer();
            global_stream_state = 1;
            return 0.0f;
        }

        int buffer_pos = 1;
        while (true) {
            if (buffer_pos >= result_token_length) {
                break;
            }
            ch = token_buffer[buffer_pos];
            buffer_pos += 1;

            switch (state) {
                case 0: {  // expecting the integer part or '.'
                    if (ch == '.') {
                        state = 1;
                        see_e_or_dot = true;
                    } else if (isdigit(ch)) {
                        value = value * 10.0f + (float)(ch - '0');
                        see_digit_before_exp = true;
                    } else if (ch == 'e') {
                        state = 2;
                        see_digit_after_exp = false;
                        see_e_or_dot = true;
                    } else {
                        rewindInputBuffer();
                        global_stream_state = 1;
                        return 0.0f;
                    }
                } break;
                case 1: {  // just see the '.' but did not see 'e'
                    if (isdigit(ch)) {
                        mantissa_length += 1;
                        value += (float)(ch - '0') * powf(10, (float)-mantissa_length);
                        see_digit_before_exp = true;
                    } else if (ch == 'e') {
                        state = 2;
                        see_digit_after_exp = false;
                        see_e_or_dot = true;
                    } else {
                        rewindInputBuffer();
                        global_stream_state = 1;
                        return 0.0f;
                    }
                } break;
                case 2: {  // after seeing 'e' but have not seen '+' '-' or first digit following 'e'
                    if (ch == '+' || ch == '-') {
                        expsign = ch == '+' ? 1 : -1;
                    } else if (isdigit(ch)) {
                        exp = exp * 10 + (ch - '0');
                        see_digit_after_exp = true;
                    } else {
                        rewindInputBuffer();
                        global_stream_state = 1;
                        return 0.0f;
                    }
                    state = 3;
                } break;
                case 3: { // after first char after 'e'
                    if (isdigit(ch)) {
                        exp = exp * 10 + (ch - '0');
                        see_digit_after_exp = true;
                    } else {
                        rewindInputBuffer();
                        global_stream_state = 1;
                        return 0.0f;
                    }
                } break;
                default:
                    errorAndExit("This should not happen!");
            }
        }
        if (!see_digit_before_exp || !see_digit_after_exp || !see_e_or_dot) {
            rewindInputBuffer();
            global_stream_state = 1;
            return 0.0f;
        } else {  // success
            updateRewindPoint(result == 2 ? cur_pos : getPrevPos(cur_pos));
            rewindInputBuffer();
            global_stream_state = 0;
            return sign * value * powf(10.0f, (float)(exp * expsign));
        }
    }
}

bool readBooleanFromStdin() {
    // read until a non-whitespace character is encountered, then read one more character to see if the boolean is valid
    int result = readNextToken();
    if (result == 2 && result_token_length == 0) {  // eof
        rewindInputBuffer();
        global_stream_state = 2;
        return false;
    } else if (result == 1) {  // buffer overflow
        errorAndExit("Error: Attempt to read an input longer than the input buffer length 1024!");
    } else {
        char ch = token_buffer[0];
        if (result_token_length == 1 && (ch == 'T' || ch == 'F')) {  // success
            updateRewindPoint(result == 2 ? cur_pos : getPrevPos(cur_pos));  // not EOF then there is a whitespace
            rewindInputBuffer();
            global_stream_state = 0;
            return ch == 'T';
        } else {
            rewindInputBuffer();
            global_stream_state = 1;
            return false;
        }
    }
}

int8_t readCharacterFromStdin() {
    int ch = readNextChar();
    updateRewindPoint(cur_pos);  // read character should always success
    global_stream_state = 0;

    if (ch == EOF) {
        return -1;
    } else {
        return (int8_t) ch;
    }
}