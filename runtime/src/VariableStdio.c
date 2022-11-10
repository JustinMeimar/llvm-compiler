#include "VariableStdio.h"
#include "RuntimeVariables.h"
#include "RuntimeErrors.h"
#include "Literal.h"

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

int32_t readIntegerFromStdin();
float readRealFromStdin();
bool readBooleanFromStdin();
int8_t readCharacterFromStdin();
int32_t getStdinState();