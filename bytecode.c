
#include "bytecode.h"

#define READ_BYTE(idx)  (bs->stream[idx])
#define READ_4BYTE(idx) (((byte4)bs->stream[idx] << 24) | ((byte4)bs->stream[idx + 1] << 16) | ((byte4)bs->stream[idx + 2] << 8) | ((byte4)bs->stream[idx + 3]))
#define READ_8BYTE(idx) (((byte8)bs->stream[idx] << 56) | ((byte8)bs->stream[idx + 1] << 48) | ((byte8)bs->stream[idx + 2] << 40) | ((byte8)bs->stream[idx + 3] << 32) | ((byte8)bs->stream[idx + 4] << 24) | ((byte8)bs->stream[idx + 5] << 16) | ((byte8)bs->stream[idx + 6] << 8) | ((byte8)bs->stream[idx + 7]))

#define READ_INT(var, idx)  {byte4 temp = READ_4BYTE(idx); var = *(int*)(&temp);}
#define READ_REAL(var, idx) {byte8 temp = READ_8BYTE(idx); var = *(double*)(&temp);}
#define READ_CHAR(var, idx) {var = *(char*)(&READ_BYTE(idx));}
#define READ_BOOL(var, idx) {var = *(bool*)(&READ_BYTE(idx));}
#define READ_REF(var, idx)  {byte8 temp = READ_8BYTE(idx); var = *(void**)(&temp);}

void initBytecodeStream(BytecodeStream* bs) {
    bs->stream = NULL;
    bs->capacity = 0;
    bs->count = 0;
}

void freeBytecodeStream(BytecodeStream* bs) {
    free(bs->stream);
    initBytecodeStream(bs);
}

void addBytecode(BytecodeStream* bs, byte b) {
    if (bs->count + 1 >= bs->capacity) {
        if (bs->capacity < 32) {
            bs->capacity = 32;
        } else {
            bs->capacity *= 2;
        }

        byte* buff = (byte*) realloc(bs->stream, bs->capacity);
        if (buff == NULL) {
            printf("Problem allocating memory for bytecode stream.\n");
            return;
        }
        bs->stream = buff;
    }

    bs->stream[bs->count++] = b;
}

void addInstruction(BytecodeStream* bs, Instruction op) {
    addBytecode(bs, (byte)op);
}

void insertAtPos(BytecodeStream* bs, byte b, int pos) {
    if (pos >= bs->capacity || pos < 0) return;

    bs->stream[pos] = b;
}

int getNextPos(BytecodeStream* bs) {
    return bs->count;
}

static int printInstruction(BytecodeStream* bs, int idx) {
    Instruction op = bs->stream[idx];

    switch (op) {
        case NOP: {
            printf("NOP");
            return 1;
        }
        case LOAD_INT: {
            printf("LOAD_INT -> ");
            int num;
            READ_INT(num, idx + 1);
            printf("%d", num);
            return 5;
        }
        case LOAD_REAL: {
            printf("LOAD_REAL -> ");
            double num;
            READ_REAL(num, idx + 1);
            printf("%f", num);
            return 9;
        }
        case LOAD_CHAR: {
            printf("LOAD_CHAR -> ");
            char c;
            READ_CHAR(c, idx + 1);
            printf("'%c'", c);
            return 2;
        }
        case LOAD_BOOL: {
            printf("LOAD_BOOL -> ");
            bool b;
            READ_BOOL(b, idx + 1);
            printf(b ? "TRUE" : "FALSE");
            return 2;
        }
        case LOAD_STRING: {
            printf("LOAD_STRING -> ");
            int length;
            READ_INT(length, idx + 1);
            printf("\"");
            for (int i = 0; i < length; i++) {
                char c;
                READ_CHAR(c, idx + 5 + i);
                printf("%c", c);
            }
            printf("\"");
            return 5 + length;
        }
        case CREATE_ARRAY: {
            printf("CREATE_ARRAY");
            return 1;
        }
        case STORE_INT: {
            printf("STORE_INT");
            return 1;
        }
        case STORE_REAL: {
            printf("STORE_REAL");
            return 1;
        }
        case STORE_CHAR: {
            printf("STORE_CHAR");
            return 1;
        }
        case STORE_BOOL: {
            printf("STORE_BOOL");
            return 1;
        }
        case STORE_REF: {
            printf("STORE_REF");
            return 1;
        }
        case FETCH_INT: {
            printf("FETCH_INT");
            return 1;
        }
        case FETCH_REAL: {
            printf("FETCH_REAL");
            return 1;
        }
        case FETCH_CHAR: {
            printf("FETCH_CHAR");
            return 1;
        }
        case FETCH_BOOL: {
            printf("FETCH_BOOL");
            return 1;
        }
        case FETCH_REF: {
            printf("FETCH_REF");
            return 1;
        }
        case CALL_SUB: {
            printf("CALL_SUB");
            return 1;
        }
        case DO_CALL: {
            printf("DO_CALL -> ");
            int pos;
            READ_INT(pos, idx + 1);
            printf("%d", pos);
            return 5;
        }
        case RETURN: {
            printf("RETURN -> ");
            int size;
            size = (int)READ_BYTE(idx + 1);
            printf("%d Bytes", size);
            return 2;
        }
        case RETURN_NIL: {
            printf("RETURN_NIL");
            return 1;
        }
        case RSTORE_INT: {
            printf("RSTORE_INT");
            return 1;
        }
        case RSTORE_REAL: {
            printf("RSTORE_REAL");
            return 1;
        }
        case RSTORE_CHAR: {
            printf("RSTORE_CHAR");
            return 1;
        }
        case RSTORE_BOOL: {
            printf("RSTORE_BOOL");
            return 1;
        }
        case RSTORE_REF: {
            printf("RSTORE_REF");
            return 1;
        }
        case RFETCH_INT: {
            printf("RFETCH_INT");
            return 1;
        }
        case RFETCH_REAL: {
            printf("RFETCH_REAL");
            return 1;
        }
        case RFETCH_CHAR: {
            printf("RFETCH_CHAR");
            return 1;
        }
        case RFETCH_BOOL: {
            printf("RFETCH_BOOL");
            return 1;
        }
        case RFETCH_REF: {
            printf("RFETCH_REF");
            return 1;
        }
        case FETCH_ARRAY_ELEM: {
            printf("FETCH_ARRAY_ELEM -> ");
            return 1;
        }
        case STORE_ARRAY_ELEM: {
            printf("STORE_ARRAY_ELEM -> ");
            return 1;
        }
        case STORE_REF_INT: {
            printf("STORE_REF_INT");
            return 1;
        }
        case STORE_REF_REAL: {
            printf("STORE_REF_REAL");
            return 1;
        }
        case STORE_REF_CHAR: {
            printf("STORE_REF_CHAR");
            return 1;
        }
        case STORE_REF_BOOL: {
            printf("STORE_REF_BOOL");
            return 1;
        }
        case FETCH_REF_INT: {
            printf("FETCH_REF_INT");
            return 1;
        }
        case FETCH_REF_REAL: {
            printf("FETCH_REF_REAL");
            return 1;
        }
        case FETCH_REF_CHAR: {
            printf("FETCH_REF_CHAR");
            return 1;
        }
        case FETCH_REF_BOOL: {
            printf("FETCH_REF_BOOL");
            return 1;
        }
        case CAST_INT_REAL: {
            printf("CAST_INT_REAL");
            return 1;
        }
        case CAST_INT_CHAR: {
            printf("CAST_INT_CHAR");
            return 1;
        }
        case CAST_CHAR_INT: {
            printf("CAST_CHAR_INT");
            return 1;
        }
        case ADD_INT: {
            printf("ADD_INT");
            return 1;
        }
        case ADD_REAL: {
            printf("ADD_REAL");
            return 1;
        }
        case MINUS_INT: {
            printf("MINUS_INT");
            return 1;
        }
        case MINUS_REAL: {
            printf("MINUS_REAL");
            return 1;
        }
        case MULT_INT: {
            printf("MULT_INT");
            return 1;
        }
        case MULT_REAL: {
            printf("MULT_REAL");
            return 1;
        }
        case DIV_INT: {
            printf("DIV_INT");
            return 1;
        }
        case DIV_REAL: {
            printf("DIV_REAL");
            return 1;
        }
        case MOD_INT: {
            printf("MOD_INT");
            return 1;
        }
        case MOD_REAL: {
            printf("MOD_REAL");
            return 1;
        }
        case FDIV_INT: {
            printf("FDIV_INT");
            return 1;
        }
        case FDIV_REAL: {
            printf("FDIV_REAL");
            return 1;
        }
        case POW_INT: {
            printf("POW_INT");
            return 1;
        }
        case POW_REAL: {
            printf("POW_REAL");
            return 1;
        }

        case CONCAT: {
            printf("CONCAT");
            return 1;
        }

        case EQ_INT: {
            printf("EQ_INT");
            return 1;
        }
        case EQ_REAL: {
            printf("EQ_REAL");
            return 1;
        }
        case EQ_BOOL: {
            printf("EQ_BOOL");
            return 1;
        }
        case EQ_REF: {
            printf("EQ_REF");
            return 1;
        }
        case EQ_STRING: {
            printf("EQ_STRING");
            return 1;
        }
        case LESS_INT: {
            printf("LESS_INT");
            return 1;
        }
        case LESS_REAL: {
            printf("LESS_REAL");
            return 1;
        }
        case LESS_BOOL: {
            printf("LESS_BOOL");
            return 1;
        }
        case LESS_REF: {
            printf("LESS_REF");
            return 1;
        }
        case LESS_STRING: {
            printf("LESS_STRING");
            return 1;
        }
        case LESS_EQ_INT: {
            printf("LESS_EQ_INT");
            return 1;
        }
        case LESS_EQ_REAL: {
            printf("LESS_EQ_REAL");
            return 1;
        }
        case LESS_EQ_BOOL: {
            printf("LESS_EQ_BOOL");
            return 1;
        }
        case LESS_EQ_REF: {
            printf("LESS_EQ_REF");
            return 1;
        }
        case LESS_EQ_STRING: {
            printf("LESS_EQ_STRING");
            return 1;
        }
        case NEQ_INT: {
            printf("NEQ_INT");
            return 1;
        }
        case NEQ_REAL: {
            printf("NEQ_REAL");
            return 1;
        }
        case NEQ_BOOL: {
            printf("NEQ_BOOL");
            return 1;
        }
        case NEQ_REF: {
            printf("NEQ_REF");
            return 1;
        }
        case NEQ_STRING: {
            printf("NEQ_STRING");
            return 1;
        }
        case GREATER_INT: {
            printf("GREATER_INT");
            return 1;
        }
        case GREATER_REAL: {
            printf("GREATER_REAL");
            return 1;
        }
        case GREATER_BOOL: {
            printf("GREATER_BOOL");
            return 1;
        }
        case GREATER_REF: {
            printf("GREATER_REF");
            return 1;
        }
        case GREATER_STRING: {
            printf("GREATER_STRING");
            return 1;
        }
        case GREATER_EQ_INT: {
            printf("GREATER_EQ_INT");
            return 1;
        }
        case GREATER_EQ_REAL: {
            printf("GREATER_EQ_REAL");
            return 1;
        }
        case GREATER_EQ_BOOL: {
            printf("GREATER_EQ_BOOL");
            return 1;
        }
        case GREATER_EQ_REF: {
            printf("GREATER_EQ_REF");
            return 1;
        }
        case GREATER_EQ_STRING: {
            printf("GREATER_EQ_STRING");
            return 1;
        }

        case AND: {
            printf("AND");
            return 1;
        }
        case OR: {
            printf("OR");
            return 1;
        }

        case NEG_INT: {
            printf("NEG_INT");
            return 1;
        }
        case NEG_REAL: {
            printf("NEG_REAL");
            return 1;
        }
        case NOT: {
            printf("NOT");
            return 1;
        }

        case POP_1B: {
            printf("POP_1B");
            return 1;
        }
        case POP_4B: {
            printf("POP_4B");
            return 1;
        }
        case POP_8B: {
            printf("POP_8B");
            return 1;
        }

        case COPY_INT: {
            printf("COPY_INT");
            return 1;
        }

        case INPUT_INT: {
            printf("INPUT_INT");
            return 1;
        }
        case INPUT_REAL: {
            printf("INPUT_REAL");
            return 1;
        }
        case INPUT_CHAR: {
            printf("INPUT_CHAR");
            return 1;
        }
        case INPUT_BOOL: {
            printf("INPUT_BOOL");
            return 1;
        }
        case INPUT_STRING: {
            printf("INPUT_STRING");
            return 1;
        }
        case OUTPUT_INT: {
            printf("OUTPUT_INT");
            return 1;
        }
        case OUTPUT_REAL: {
            printf("OUTPUT_REAL");
            return 1;
        }
        case OUTPUT_CHAR: {
            printf("OUTPUT_CHAR");
            return 1;
        }
        case OUTPUT_BOOL: {
            printf("OUTPUT_BOOL");
            return 1;
        }
        case OUTPUT_REF: {
            printf("OUTPUT_REF");
            return 1;
        }
        case OUTPUT_STRING: {
            printf("OUTPUT_STRING");
            return 1;
        }
        case OUTPUT_NL: {
            printf("OUTPUT_NL");
            return 1;
        }
        case EXIT: {
            printf("EXIT");
            return 1;
        }

        /*case RINPUT_INT: {
            break;
        }
        case RINPUT_REAL: {
            break;
        }
        case RINPUT_CHAR: {
            break;
        }
        case RINPUT_BOOL: {
            break;
        }
        case RINPUT_STRING: {
            break;
        }*/

        case B_FALSE: {
            printf("B_FALSE -> ");
            int pos;
            READ_INT(pos, idx + 1);
            printf("%d", pos);
            return 5;
        }
        case BRANCH: {
            printf("BRANCH -> ");
            int pos;
            READ_INT(pos, idx + 1);
            printf("%d", pos);
            return 5;
        }

        case GET_REF: {
            printf("GET_REF -> ");
            return 1;
        }
        case RGET_REF: {
            printf("RGET_REF -> ");
            return 1;
        }
    }
}

void printBytestream(BytecodeStream* bs) {
    int count = 0;

    while (count < bs->count) {
        printf("%d |  ", count);
        count += printInstruction(bs, count);
        printf("\n");
    }
}

bool genBinFile(BytecodeStream* bs, const char* fileName) {
    FILE* filePtr;

    const char* extension = ".pcbc";
    char* name = malloc(strlen(fileName) + strlen(extension) + 1);
    if (name == NULL) {
        printf("Problem allocating memory for filename.\n");
        return false;
    }

    name[0] = '\0';
    strcat(name, fileName);
    strcat(name, extension);

    filePtr = fopen(name, "wb");

    if (filePtr == NULL) {
        printf("Problem opening file.\n");
        free(name);
        return false;
    }

    fclose(filePtr);

    filePtr = fopen(name, "ab");

    free(name);

    if (filePtr == NULL) {
        printf("Problem opening file.\n");
        return false;
    }

    fwrite(&bs->count, sizeof(bs->count), 1, filePtr);

    fwrite(bs->stream, sizeof(byte), bs->count, filePtr);

    fclose(filePtr);

    return true;
}

bool readBinFile(BytecodeStream* bs, const char* fileName) {
    FILE* filePtr;

    const char* extension = ".pcbc";
    char* name = malloc(strlen(fileName) + strlen(extension) + 1);
    if (name == NULL) {
        printf("Problem allocating memory for filename.\n");
        return false;
    }

    name[0] = '\0';
    strcat(name, fileName);
    strcat(name, extension);

    filePtr = fopen(name, "rb");

    free(name);

    if (filePtr == NULL) {
        printf("Problem opening file.\n");
        return false;
    }

    fread(&bs->count, sizeof(bs->count), 1, filePtr);

    bs->capacity = bs->count;
    bs->stream = malloc(bs->capacity * sizeof(byte));

    if (bs->stream == NULL) {
        printf("Error allocating memory for bytecode stream.\n");
        return false;
    }

    fread(bs->stream, sizeof(byte), bs->count, filePtr);

    fclose(filePtr);

    return true;
}