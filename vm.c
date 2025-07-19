
#include "vm.h"



static void runtimeError(VM* vm, const char* message) {
    vm->hadRuntimeError = true;

    fprintf(stderr, "Runtime error at PC %d: %s\n",vm->PC, message);
}

void initVM(VM* vm, int heapCapacity, int stackCapacity, int callStackCapacity, BytecodeStream* bStream) {
    vm->PC = 0;
    vm->hadRuntimeError = false;
    initStack(&vm->stack, stackCapacity);
    initCallStack(&vm->callStack, callStackCapacity);
    createProgramMemory(&vm->mem, heapCapacity);
    vm->program = bStream;
    vm->nextCallBase = 0;
}

void freeVM(VM* vm) {
    vm->PC = 0;
    vm->hadRuntimeError = false;
    freeStack(&vm->stack);
    freeCallStack(&vm->callStack);
    freeProgramMemory(&vm->mem);
    vm->program = NULL;
}

static void advance(VM* vm) {
    vm->PC++;
}

static void outputStack(VM* vm) {
    printf("\n\n\nShowing stack state\n");
    showStack(&vm->stack);
}

static double dmod(double a, double b) {
    return a - (int)(a / b) * b;
}

static Obj* concatStrings(VM* vm, Obj* fst, Obj* snd) {
    int length = fst->as.StringObj.length + snd->as.StringObj.length;
    char* buff = malloc(length * sizeof(char));
    if (buff == NULL) return NULL;

    for (int i = 0; i < fst->as.StringObj.length; i++) {
        buff[i] = fst->as.StringObj.start[i];
    }

    for (int i = 0; i < snd->as.StringObj.length; i++) {
        buff[fst->as.StringObj.length + i] = snd->as.StringObj.start[i];
    }

    Obj* res = allocString(&vm->mem, buff, length);
    if (res == NULL) {
        free(buff);
    }
    return res;
}

static int cmpStrings(Obj* str1, Obj* str2) {
    // 0 if equal, <0 if str1 before str2 and viceversa
    int length = str1->as.StringObj.length;
    int shorter = -1;
    if (str2->as.StringObj.length < length) {
        length = str2->as.StringObj.length;
        shorter = 1;
    } else if (str2->as.StringObj.length == length) {
        shorter = 0;
    }

    for (int i = 0; i < length; i++) {
        if (str1->as.StringObj.start[i] != str2->as.StringObj.start[i]) {
            return (int)str1->as.StringObj.start[i] - (int)str2->as.StringObj.start[i];
        }
    }

    return shorter;
}

static void clearInputBuffer() {
    clearerr(stdin);
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { }
}

static void jmpTo(VM* vm, int dst) {
    vm->PC = dst - 1;
}

static void pushByte(VM* vm, byte b, bool isRef) {
    bool res = push(&vm->stack, b, isRef);
    if (!res) {
        runtimeError(vm, "Stack overflow.");
    }
}

static byte popByte(VM* vm) {
    return pop(&vm->stack);
}

#define READ_BYTE(idx)  (vm->program->stream[idx])
#define READ_4BYTE(idx) (((byte4)vm->program->stream[idx] << 24) | ((byte4)vm->program->stream[idx + 1] << 16) | ((byte4)vm->program->stream[idx + 2] << 8) | ((byte4)vm->program->stream[idx + 3]))
#define READ_8BYTE(idx) (((byte8)vm->program->stream[idx] << 56) | ((byte8)vm->program->stream[idx + 1] << 48) | ((byte8)vm->program->stream[idx + 2] << 40) | ((byte8)vm->program->stream[idx + 3] << 32) | ((byte8)vm->program->stream[idx + 4] << 24) | ((byte8)vm->program->stream[idx + 5] << 16) | ((byte8)vm->program->stream[idx + 6] << 8) | ((byte8)vm->program->stream[idx + 7]))

#define READ_INT(var, idx)  {byte4 temp = READ_4BYTE(idx); var = *(int*)(&temp);}
#define READ_REAL(var, idx) {byte8 temp = READ_8BYTE(idx); var = *(double*)(&temp);}
#define READ_CHAR(var, idx) {var = *(char*)(&READ_BYTE(idx));}
#define READ_BOOL(var, idx) {var = *(bool*)(&READ_BYTE(idx));}
#define READ_REF(var, idx)  {void* temp = READ_8BYTE(idx); var = *(void**)(&temp);}

#define PUSH_BYTE(b)    { pushByte(vm, b, false); }
#define PUSH_4BYTE(b)   { pushByte(vm, (byte)(b & 0xff), false); pushByte(vm, (byte)((b>>8) & 0xff), false); pushByte(vm, (byte)((b>>16) & 0xff), false); pushByte(vm, (byte)((b>>24) & 0xff), false); }
#define PUSH_8BYTE(b)   { pushByte(vm, (byte)(b & 0xff), false); pushByte(vm, (byte)((b>>8) & 0xff), false); pushByte(vm, (byte)((b>>16) & 0xff), false); pushByte(vm, (byte)((b>>24) & 0xff), false); pushByte(vm, (byte)((b>>32) & 0xff), false); pushByte(vm, (byte)((b>>40) & 0xff), false); pushByte(vm, (byte)((b>>48) & 0xff), false); pushByte(vm, (byte)((b>>56) & 0xff), false); }
#define PUSH_8BREF(b)   { pushByte(vm, (byte)(b & 0xff), true); pushByte(vm, (byte)((b>>8) & 0xff), false); pushByte(vm, (byte)((b>>16) & 0xff), false); pushByte(vm, (byte)((b>>24) & 0xff), false); pushByte(vm, (byte)((b>>32) & 0xff), false); pushByte(vm, (byte)((b>>40) & 0xff), false); pushByte(vm, (byte)((b>>48) & 0xff), false); pushByte(vm, (byte)((b>>56) & 0xff), false); }

#define PUSH_CHAR(c)    { byte temp = *(byte*)(&c); PUSH_BYTE(temp); }
#define PUSH_BOOL(b)    { byte temp = *(byte*)(&b); PUSH_BYTE(temp); }
#define PUSH_INT(x)     { byte4 temp = *(byte4*)(&x); PUSH_4BYTE(temp); }
#define PUSH_REAL(r)    { byte8 temp = *(byte8*)(&r); PUSH_8BYTE(temp); }
#define PUSH_REF(r)     { byte8 temp = *(byte8*)(&r); PUSH_8BREF(temp); }

#define POP_BYTE(var)   { var = popByte(vm); }
#define POP_4BYTE(var)  { var = (((byte4)(popByte(vm)) << 24) | ((byte4)(popByte(vm)) << 16) | ((byte4)(popByte(vm)) << 8) | ((byte4)(popByte(vm)))); }
#define POP_8BYTE(var)  { var = (((byte8)(popByte(vm)) << 56) | ((byte8)(popByte(vm)) << 48) | ((byte8)(popByte(vm)) << 40) | ((byte8)(popByte(vm)) << 32) | ((byte8)(popByte(vm)) << 24) | ((byte8)(popByte(vm)) << 16) | ((byte8)(popByte(vm)) << 8) | ((byte8)(popByte(vm)))); }

#define POP_CHAR(c)     { byte temp; POP_BYTE(temp); c = *(char*)(&temp); }
#define POP_BOOL(b)     { byte temp; POP_BYTE(temp); b = *(bool*)(&temp); }
#define POP_INT(n)      { byte4 temp; POP_4BYTE(temp); n = *(int*)(&temp); }
#define POP_REAL(r)     { byte8 temp; POP_8BYTE(temp); r = *(double*)(&temp); }
#define POP_REF(r)      { byte8 temp; POP_8BYTE(temp); r = *(void**)(&temp); }

static void substring(VM* vm) {
    int length;
    POP_INT(length);
    int initPos;
    POP_INT(initPos);
    void* ref;
    POP_REF(ref);

    if (!isValidReference(&vm->mem, ref)) {
        runtimeError(vm, "Segmentation fault.");
        return;
    }

    Obj* str = (Obj*)ref;

    if (initPos + length - 1 > str->as.StringObj.length) {
        runtimeError(vm, "Substring overextends string.");
        return;
    }
    if (initPos <= 0 || initPos > str->as.StringObj.length) {
        runtimeError(vm, "Initial pos must be between 1 and length of string.");
        return;
    }
    char* sub = extractNullTerminatedString(str->as.StringObj.start + initPos - 1, length);

    Obj* strPtr = allocString(&vm->mem, sub, length);

    if (strPtr == NULL) {
        runtimeError(vm, "String allocation failed in heap.");
        printf("IN USE %zu of %zu and next free is %p\n",  vm->mem.inUse, vm->mem.memSize, vm->mem.free);
        free(sub);
        return;
    }

    PUSH_REF(strPtr);
}

static void length(VM* vm) {
    void* ref;
    POP_REF(ref);

    if (!isValidReference(&vm->mem, ref)) {
        runtimeError(vm, "Segmentation fault.");
        return;
    }

    Obj* str = (Obj*)ref;

    int len = str->as.StringObj.length;

    PUSH_INT(len);
}

static void lcase(VM* vm) {
    void* ref;
    POP_REF(ref);

    if (!isValidReference(&vm->mem, ref)) {
        runtimeError(vm, "Segmentation fault.");
        return;
    }

    Obj* str = (Obj*)ref;

    char* lchars = (char*) malloc(sizeof(char) * str->as.StringObj.length);

    if (lchars == NULL) {
        runtimeError(vm, "Memory allocation fail.");
        return;
    }

    for (int i = 0; i < str->as.StringObj.length; i++) {
        char c = str->as.StringObj.start[i];

        if (c >= 'A' && c <= 'Z') {
            c += 32;
        }

        lchars[i] = c;
    }

    Obj* newStr = allocString(&vm->mem, lchars, str->as.StringObj.length);

    if (newStr == NULL) {
        runtimeError(vm, "Memory allocation fail.");
        return;
    }

    PUSH_REF(newStr);
}

static void ucase(VM* vm) {
    void* ref;
    POP_REF(ref);

    if (!isValidReference(&vm->mem, ref)) {
        runtimeError(vm, "Segmentation fault.");
        return;
    }

    Obj* str = (Obj*)ref;

    char* uchars = (char*) malloc(sizeof(char) * str->as.StringObj.length);

    if (uchars == NULL) {
        runtimeError(vm, "Memory allocation fail.");
        return;
    }

    for (int i = 0; i < str->as.StringObj.length; i++) {
        char c = str->as.StringObj.start[i];

        if (c >= 'a' && c <= 'z') {
            c -= 32;
        }

        uchars[i] = c;
    }

    Obj* newStr = allocString(&vm->mem, uchars, str->as.StringObj.length);

    if (newStr == NULL) {
        runtimeError(vm, "Memory allocation fail.");
        return;
    }

    PUSH_REF(newStr);
}

static void randomBetween(VM* vm) {
    int min, max;
    POP_INT(max);
    POP_INT(min);

    srand((unsigned int)clock());

    int randomNum = rand() % (max - min + 1) + min;

    PUSH_INT(randomNum);
}

static void rnd(VM* vm) {
    srand((unsigned int)clock());

    double randomReal = rand() / ((double) RAND_MAX);

    PUSH_REAL(randomReal);
}

static void integer(VM* vm) {
    double real;
    POP_REAL(real);

    int num = (int)real;

    PUSH_INT(num);
}

static void eof(VM* vm) {
    void* ref;
    POP_REF(ref);

    if (!isValidReference(&vm->mem, ref)) {
        runtimeError(vm, "Segmentation fault.");
        return;
    }

    Obj* file = (Obj*) ref;

    FILE* filePtr = file->as.FileObj.filePtr;

    bool isEOF = feof(filePtr) == 0 ? false : true;

    PUSH_BOOL(isEOF);
}

static void charAt(VM* vm) {
    int pos;
    POP_INT(pos);

    void* ref;
    POP_REF(ref);

    if (!isValidReference(&vm->mem, ref)) {
        runtimeError(vm, "Segmentation fault.");
        return;
    }

    Obj* str = (Obj*) ref;

    if (pos <= 0 || pos > str->as.StringObj.length) {
        runtimeError(vm, "Position must be between 1 and length of string.");
        return;
    }

    char result = str->as.StringObj.start[pos - 1];

    PUSH_CHAR(result);
}

static void runBuiltinFunc(VM* vm, int idx) {
    switch (idx) {
        case 0: {
            substring(vm);
            break;
        }
        case 1: {
            length(vm);
            break;
        }
        case 2: {
            lcase(vm);
            break;
        }
        case 3: {
            ucase(vm);
            break;
        }
        case 4: {
            randomBetween(vm);
            break;
        }
        case 5: {
            rnd(vm);
            break;
        }
        case 6: {
            integer(vm);
            break;
        }
        case 7: {
            eof(vm);
            break;
        }
        case 8: {
            charAt(vm);
            break;
        }
        default: break;
    }
}

static void runInstruction(VM* vm) {
    Instruction op = vm->program->stream[vm->PC];

    switch (op) {
        case LOAD_INT: {
            int n;
            READ_INT(n, vm->PC + 1);
            vm->PC += 4;
            PUSH_INT(n);
            break;
        }
        case LOAD_REAL: {
            double d;
            READ_REAL(d, vm->PC + 1);
            vm->PC += 8;
            PUSH_REAL(d);
            break;
        }
        case LOAD_CHAR: {
            char c;
            READ_CHAR(c, vm->PC + 1);
            vm->PC++;
            PUSH_CHAR(c);
            break;
        }
        case LOAD_BOOL: {
            bool b;
            READ_BOOL(b, vm->PC + 1);
            vm->PC++;
            PUSH_BOOL(b);
            break;
        }
        case LOAD_STRING: {
            int length;
            READ_INT(length, vm->PC + 1);
            vm->PC += 4;
            char* buff = malloc(length * sizeof(char));
            if (buff == NULL) {
                runtimeError(vm, "String allocation failed.");
                break;
            }
            for (int i = 0; i < length; i++) {
                char c;
                READ_CHAR(c, vm->PC + 1);
                vm->PC++;
                buff[i] = c;
            }

            Obj* strPtr = allocString(&vm->mem, buff, length);
            if (strPtr == NULL) {
                runtimeError(vm, "String allocation failed in heap.");
                printf("IN USE %zu of %zu and next free is %p\n",  vm->mem.inUse, vm->mem.memSize, vm->mem.free);
                free(buff);
                break;
            }

            PUSH_REF(strPtr);
            break;
        }
        case CREATE_ARRAY: {
            int x0, x1, y0, y1, elemSize;
            POP_INT(elemSize);
            POP_INT(y1);
            POP_INT(y0);
            POP_INT(x1);
            POP_INT(x0);

            Obj* arrPtr = allocArray(&vm->mem, x1 - x0 + 1, y1 - y0 + 1, x0, y0, elemSize);
            if (arrPtr == NULL) {
                runtimeError(vm, "Array allocation failed.");
                break;
            }

            PUSH_REF(arrPtr);
            break;
        }
        case STORE_INT: {
            int pos; POP_INT(pos);
            int num; POP_INT(num);
            setAt(&vm->stack, (byte)(num) & 0xff, false, pos);
            setAt(&vm->stack, (byte)(num >> 8) & 0xff, false, pos + 1);
            setAt(&vm->stack, (byte)(num >> 16) & 0xff, false, pos + 2);
            setAt(&vm->stack, (byte)(num >> 24) & 0xff, false, pos + 3);
            PUSH_INT(num);
            break;
        }
        case STORE_REAL: {
            int pos; POP_INT(pos);
            byte8 num; POP_8BYTE(num);
            setAt(&vm->stack, (byte)(num) & 0xff, false, pos);
            setAt(&vm->stack, (byte)(num >> 8) & 0xff, false, pos + 1);
            setAt(&vm->stack, (byte)(num >> 16) & 0xff, false, pos + 2);
            setAt(&vm->stack, (byte)(num >> 24) & 0xff, false, pos + 3);
            setAt(&vm->stack, (byte)(num >> 32) & 0xff, false, pos + 4);
            setAt(&vm->stack, (byte)(num >> 40) & 0xff, false, pos + 5);
            setAt(&vm->stack, (byte)(num >> 48) & 0xff, false, pos + 6);
            setAt(&vm->stack, (byte)(num >> 56) & 0xff, false, pos + 7);

            PUSH_8BYTE(num);
            break;
        }
        case STORE_CHAR: {
            int pos;
            POP_INT(pos);
            byte c; POP_BYTE(c);
            setAt(&vm->stack, c, false, pos);

            PUSH_BYTE(c);
            break;
        }
        case STORE_BOOL: {
            int pos;
            POP_INT(pos);
            byte b; POP_BYTE(b);
            setAt(&vm->stack, b, false, pos);

            PUSH_BYTE(b);
            break;
        }
        case STORE_REF: {
            int pos; POP_INT(pos);
            byte8 num; POP_8BYTE(num);
            setAt(&vm->stack, (byte)(num) & 0xff, true, pos);
            setAt(&vm->stack, (byte)(num >> 8) & 0xff, true, pos + 1);
            setAt(&vm->stack, (byte)(num >> 16) & 0xff, true, pos + 2);
            setAt(&vm->stack, (byte)(num >> 24) & 0xff, true, pos + 3);
            setAt(&vm->stack, (byte)(num >> 32) & 0xff, true, pos + 4);
            setAt(&vm->stack, (byte)(num >> 40) & 0xff, true, pos + 5);
            setAt(&vm->stack, (byte)(num >> 48) & 0xff, true, pos + 6);
            setAt(&vm->stack, (byte)(num >> 56) & 0xff, true, pos + 7);

            PUSH_8BYTE(num);
            break;
        }
        case FETCH_INT: {
            int pos; POP_INT(pos);

            byte4 res = (((byte4)(getAt(&vm->stack, pos + 3)) << 24) | ((byte4)(getAt(&vm->stack, pos + 2)) << 16) | ((byte4)(getAt(&vm->stack, pos + 1)) << 8) | ((byte4)(getAt(&vm->stack, pos))));
            PUSH_4BYTE(res);
            break;
        }
        case FETCH_REAL: {
            int pos; POP_INT(pos);

            byte8 res = (((byte8)(getAt(&vm->stack, pos + 7)) << 56) | ((byte8)(getAt(&vm->stack, pos + 6)) << 48) | ((byte8)(getAt(&vm->stack, pos + 5)) << 40) | ((byte8)(getAt(&vm->stack, pos + 4)) << 32) | ((byte8)(getAt(&vm->stack, pos + 3)) << 24) | ((byte8)(getAt(&vm->stack, pos + 2)) << 16) | ((byte8)(getAt(&vm->stack, pos + 1)) << 8) | ((byte8)(getAt(&vm->stack, pos))));
            PUSH_8BYTE(res);
            break;
        }
        case FETCH_CHAR: {
            int pos; POP_INT(pos);
            byte res = getAt(&vm->stack, pos);
            PUSH_BYTE(res);
            break;
        }
        case FETCH_BOOL: {
            int pos; POP_INT(pos);
            byte res = getAt(&vm->stack, pos);
            PUSH_BYTE(res);
            break;
        }
        case FETCH_REF: {
            int pos; POP_INT(pos);

            byte8 res = (((byte8)(getAt(&vm->stack, pos + 7)) << 56) | ((byte8)(getAt(&vm->stack, pos + 6)) << 48) | ((byte8)(getAt(&vm->stack, pos + 5)) << 40) | ((byte8)(getAt(&vm->stack, pos + 4)) << 32) | ((byte8)(getAt(&vm->stack, pos + 3)) << 24) | ((byte8)(getAt(&vm->stack, pos + 2)) << 16) | ((byte8)(getAt(&vm->stack, pos + 1)) << 8) | ((byte8)(getAt(&vm->stack, pos))));
            PUSH_8BREF(res);
            break;
        }
        case CALL_SUB: {
            vm->nextCallBase = getNextFree(&vm->stack);
            break;
        }
        case DO_CALL: {
            int newPC;
            READ_INT(newPC, vm->PC + 1);
            vm->PC += 4;
            int returnPC = vm->PC + 1;
            pushCallFrame(&vm->callStack, returnPC, vm->nextCallBase);
            jmpTo(vm, newPC);
            break;
        }
        case RETURN: {
            byte size = READ_BYTE(vm->PC + 1);
            vm->PC++;
            switch (size) {
                case 1: {
                    byte res; POP_BYTE(res);
                    int base = getBaseStackPos(&vm->callStack);
                    int returnPC = popCallFrame(&vm->callStack);
                    jmpTo(vm, returnPC);
                    vm->stack.top = base - 1;
                    PUSH_BYTE(res);
                    break;
                }
                case 4: {
                    byte4 res; POP_4BYTE(res);
                    int base = getBaseStackPos(&vm->callStack);
                    int returnPC = popCallFrame(&vm->callStack);
                    jmpTo(vm, returnPC);
                    vm->stack.top = base - 1;
                    PUSH_4BYTE(res);
                    break;
                }
                case 8: {
                    byte8 res; POP_8BYTE(res);
                    int base = getBaseStackPos(&vm->callStack);
                    int returnPC = popCallFrame(&vm->callStack);
                    jmpTo(vm, returnPC);
                    vm->stack.top = base - 1;
                    PUSH_8BYTE(res);
                    break;
                }
                default: break;
            }
            break;
        }
        case RETURN_NIL: {
            int base = getBaseStackPos(&vm->callStack);
            int returnPC = popCallFrame(&vm->callStack);
            jmpTo(vm, returnPC);
            vm->stack.top = base - 1;
            break;
        }
        case CALL_BUILTIN: {
            int builtinIdx;
            READ_INT(builtinIdx, vm->PC + 1);
            vm->PC += 4;
            runBuiltinFunc(vm, builtinIdx);
            break;
        }
        case RSTORE_INT: {
            int base = getBaseStackPos(&vm->callStack);
            int pos; POP_INT(pos); pos += base;
            int num; POP_INT(num);
            setAt(&vm->stack, (byte)(num) & 0xff, false, pos);
            setAt(&vm->stack, (byte)(num >> 8) & 0xff, false, pos + 1);
            setAt(&vm->stack, (byte)(num >> 16) & 0xff, false, pos + 2);
            setAt(&vm->stack, (byte)(num >> 24) & 0xff, false, pos + 3);
            PUSH_INT(num);
            break;
        }
        case RSTORE_REAL: {
            int base = getBaseStackPos(&vm->callStack);
            int pos; POP_INT(pos); pos += base;
            byte8 num; POP_8BYTE(num);
            setAt(&vm->stack, (byte)(num) & 0xff, false, pos);
            setAt(&vm->stack, (byte)(num >> 8) & 0xff, false, pos + 1);
            setAt(&vm->stack, (byte)(num >> 16) & 0xff, false, pos + 2);
            setAt(&vm->stack, (byte)(num >> 24) & 0xff, false, pos + 3);
            setAt(&vm->stack, (byte)(num >> 32) & 0xff, false, pos + 4);
            setAt(&vm->stack, (byte)(num >> 40) & 0xff, false, pos + 5);
            setAt(&vm->stack, (byte)(num >> 48) & 0xff, false, pos + 6);
            setAt(&vm->stack, (byte)(num >> 56) & 0xff, false, pos + 7);

            PUSH_8BYTE(num);
            break;
        }
        case RSTORE_CHAR: {
            int base = getBaseStackPos(&vm->callStack);
            int pos;
            POP_INT(pos); pos += base;
            byte c; POP_BYTE(c);
            setAt(&vm->stack, c, false, pos);

            PUSH_BYTE(c);
            break;
        }
        case RSTORE_BOOL: {
            int base = getBaseStackPos(&vm->callStack);
            int pos;
            POP_INT(pos); pos += base;
            byte c; POP_BYTE(c);
            setAt(&vm->stack, c, false, pos);

            PUSH_BYTE(c);
            break;
        }
        case RSTORE_REF: {
            int base = getBaseStackPos(&vm->callStack);
            int pos; POP_INT(pos); pos += base;
            byte8 num; POP_8BYTE(num);
            setAt(&vm->stack, (byte)(num) & 0xff, true, pos);
            setAt(&vm->stack, (byte)(num >> 8) & 0xff, true, pos + 1);
            setAt(&vm->stack, (byte)(num >> 16) & 0xff, true, pos + 2);
            setAt(&vm->stack, (byte)(num >> 24) & 0xff, true, pos + 3);
            setAt(&vm->stack, (byte)(num >> 32) & 0xff, true, pos + 4);
            setAt(&vm->stack, (byte)(num >> 40) & 0xff, true, pos + 5);
            setAt(&vm->stack, (byte)(num >> 48) & 0xff, true, pos + 6);
            setAt(&vm->stack, (byte)(num >> 56) & 0xff, true, pos + 7);

            PUSH_8BYTE(num);
            break;
        }
        case RFETCH_INT: {
            int base = getBaseStackPos(&vm->callStack);
            int pos; POP_INT(pos); pos += base;

            byte4 res = (((byte4)(getAt(&vm->stack, pos + 3)) << 24) | ((byte4)(getAt(&vm->stack, pos + 2)) << 16) | ((byte4)(getAt(&vm->stack, pos + 1)) << 8) | ((byte4)(getAt(&vm->stack, pos))));
            PUSH_4BYTE(res);
            break;
        }
        case RFETCH_REAL: {
            int base = getBaseStackPos(&vm->callStack);
            int pos; POP_INT(pos); pos += base;

            byte8 res = (((byte8)(getAt(&vm->stack, pos + 7)) << 56) | ((byte8)(getAt(&vm->stack, pos + 6)) << 48) | ((byte8)(getAt(&vm->stack, pos + 5)) << 40) | ((byte8)(getAt(&vm->stack, pos + 4)) << 32) | ((byte8)(getAt(&vm->stack, pos + 3)) << 24) | ((byte8)(getAt(&vm->stack, pos + 2)) << 16) | ((byte8)(getAt(&vm->stack, pos + 1)) << 8) | ((byte8)(getAt(&vm->stack, pos))));
            PUSH_8BYTE(res);
            break;
        }
        case RFETCH_CHAR: {
            int base = getBaseStackPos(&vm->callStack);
            int pos; POP_INT(pos); pos += base;
            byte res = getAt(&vm->stack, pos);
            PUSH_BYTE(res);
            break;
        }
        case RFETCH_BOOL: {
            int base = getBaseStackPos(&vm->callStack);
            int pos; POP_INT(pos); pos += base;
            byte res = getAt(&vm->stack, pos);
            PUSH_BYTE(res);
            break;
        }
        case RFETCH_REF: {
            int base = getBaseStackPos(&vm->callStack);
            int pos; POP_INT(pos); pos += base;

            byte8 res = (((byte8)(getAt(&vm->stack, pos + 7)) << 56) | ((byte8)(getAt(&vm->stack, pos + 6)) << 48) | ((byte8)(getAt(&vm->stack, pos + 5)) << 40) | ((byte8)(getAt(&vm->stack, pos + 4)) << 32) | ((byte8)(getAt(&vm->stack, pos + 3)) << 24) | ((byte8)(getAt(&vm->stack, pos + 2)) << 16) | ((byte8)(getAt(&vm->stack, pos + 1)) << 8) | ((byte8)(getAt(&vm->stack, pos))));
            PUSH_8BREF(res);
            break;
        }
        case FETCH_ARRAY_ELEM: {
            int y; POP_INT(y);
            int x; POP_INT(x);
            void* ref; POP_REF(ref);

            if (!isValidReference(&vm->mem, ref)) {
                runtimeError(vm, "Segmentation fault.");
                break;
            }

            Obj* arr = (Obj*)ref;
#define ARR arr->as.ArrayObj
            if (x < ARR.x0 || x >= ARR.x0 + ARR.length || y < ARR.y0 || y >= ARR.y0 + ARR.width) {
                runtimeError(vm, "Array out of bounds access.");
                break;
            }

            for (int i = ARR.elemSize - 1; i >= 0; i--) {
                byte temp = ARR.start[(y - ARR.y0) * ARR.length * ARR.elemSize + (x - ARR.x0) * ARR.elemSize + i];
                PUSH_BYTE(temp);
            }

#undef ARR
            break;
        }
        case STORE_ARRAY_ELEM: {
            int y; POP_INT(y);
            int x; POP_INT(x);
            void* ref; POP_REF(ref);

            if (!isValidReference(&vm->mem, ref)) {
                runtimeError(vm, "Segmentation fault.");
                break;
            }

            Obj* arr = (Obj*)ref;
#define ARR arr->as.ArrayObj
            if (x < ARR.x0 || x >= ARR.x0 + ARR.length || y < ARR.y0 || y >= ARR.y0 + ARR.width) {
                runtimeError(vm, "Array out of bounds access.");
                break;
            }

            for (int i = 0; i < ARR.elemSize; i++) {
                byte temp; POP_BYTE(temp);
                ARR.start[(y - ARR.y0) * ARR.length * ARR.elemSize + (x - ARR.x0) * ARR.elemSize + i] = temp;
            }

            for (int i = ARR.elemSize - 1; i >= 0; i--) {
                byte temp = ARR.start[(y - ARR.y0) * ARR.length * ARR.elemSize + (x - ARR.x0) * ARR.elemSize + i];
                PUSH_BYTE(temp);
            }

#undef ARR
            break;
        }
        case STORE_REF_INT: {
            void* ref;
            POP_REF(ref);
            byte4 num; POP_4BYTE(num);

            if (!isStackRef(&vm->stack, ref)) {
                runtimeError(vm, "Segmentation fault.");
                break;
            }

            StackElem* ptr = (StackElem*)ref;

            for (int i = 0; i < 4; i++) {
                (ptr[i]).value = (byte)((num >> (8 * i)) & 0xff);
                (ptr[i]).isRef = false;
            }

            PUSH_4BYTE(num);
            break;
        }
        case STORE_REF_REAL: {
            void* ref;
            POP_REF(ref);
            byte8 num; POP_8BYTE(num);

            if (!isStackRef(&vm->stack, ref)) {
                runtimeError(vm, "Segmentation fault.");
                break;
            }

            StackElem* ptr = (StackElem*)ref;

            for (int i = 0; i < 8; i++) {
                (ptr[i]).value = (byte)((num >> (8 * i)) & 0xff);
                (ptr[i]).isRef = false;
            }

            PUSH_8BYTE(num);
            break;
        }
        case STORE_REF_CHAR: {
            void* ref;
            POP_REF(ref);
            byte c; POP_BYTE(c);

            if (!isStackRef(&vm->stack, ref)) {
                runtimeError(vm, "Segmentation fault.");
                break;
            }

            StackElem* ptr = (StackElem*)ref;

            ptr->value = c;
            ptr->isRef = false;

            PUSH_BYTE(c);
            break;
        }
        case STORE_REF_BOOL: {
            void* ref;
            POP_REF(ref);
            byte c; POP_BYTE(c);

            if (!isStackRef(&vm->stack, ref)) {
                runtimeError(vm, "Segmentation fault.");
                break;
            }

            StackElem* ptr = (StackElem*)ref;

            ptr->value = c;
            ptr->isRef = false;

            PUSH_BYTE(c);
            break;
        }
        case FETCH_REF_INT: {
            void* ref;
            POP_REF(ref);

            if (!isStackRef(&vm->stack, ref)) {
                runtimeError(vm, "Segmentation fault.");
                break;
            }

            StackElem* ptr = (StackElem*)ref;
            byte4 num = 0;

            for (int i = 0; i < 4; i++) {
                num |= (byte4)((ptr[i].value)) << (8 * i);
            }

            PUSH_4BYTE(num);
            break;
        }
        case FETCH_REF_REAL: {
            void* ref;
            POP_REF(ref);

            if (!isStackRef(&vm->stack, ref)) {
                runtimeError(vm, "Segmentation fault.");
                break;
            }

            StackElem* ptr = (StackElem*)ref;
            byte8 num = 0;

            for (int i = 0; i < 4; i++) {
                num |= (byte8)((ptr[i].value)) << (8*i);
            }

            PUSH_8BYTE(num);
            break;
        }
        case FETCH_REF_CHAR: {
            void* ref;
            POP_REF(ref);

            if (!isStackRef(&vm->stack, ref)) {
                runtimeError(vm, "Segmentation fault.");
                break;
            }

            StackElem* ptr = (StackElem*)ref;
            byte c;

            c = ptr->value;

            PUSH_BYTE(c);
            break;
        }
        case FETCH_REF_BOOL: {
            void* ref;
            POP_REF(ref);

            if (!isStackRef(&vm->stack, ref)) {
                runtimeError(vm, "Segmentation fault.");
                break;
            }

            StackElem* ptr = (StackElem*)ref;
            byte c;

            c = ptr->value;

            PUSH_BYTE(c);
            break;
        }
        case CAST_INT_REAL: {
            int num; POP_INT(num);

            double real = (double)num;

            PUSH_REAL(real);
            break;
        }
        case CAST_INT_CHAR: {
            int num; POP_INT(num);
            if (num >= 256) num = 256;
            else if (num < 0) num = 0;

            char c = (char)num;
            PUSH_CHAR(c);
            break;
        }
        case CAST_CHAR_INT: {
            char c;
            POP_CHAR(c);
            int num = (int)c;
            PUSH_INT(num);
            break;
        }
        case ADD_INT: {
            int a, b;
            POP_INT(a); POP_INT(b);
            int res = a + b;
            PUSH_INT(res);
            break;
        }
        case ADD_REAL: {
            double a, b;
            POP_REAL(a); POP_REAL(b);
            double res = a + b;
            PUSH_REAL(res);
            break;
        }
        case MINUS_INT: {
            int a, b;
            POP_INT(a); POP_INT(b);
            int res = b - a;
            PUSH_INT(res);
            break;
        }
        case MINUS_REAL: {
            double a, b;
            POP_REAL(a); POP_REAL(b);
            double res = b - a;
            PUSH_REAL(res);
            break;
        }
        case MULT_INT: {
            int a, b;
            POP_INT(a); POP_INT(b);
            int res = a * b;
            PUSH_INT(res);
            break;
        }
        case MULT_REAL: {
            double a, b;
            POP_REAL(a); POP_REAL(b);
            double res = a * b;
            PUSH_REAL(res);
            break;
        }
        case DIV_INT: {
            int a, b;
            POP_INT(a); POP_INT(b);
            double res = (double)((double)b / (double)a);
            PUSH_REAL(res);
            break;
        }
        case DIV_REAL: {
            double a, b;
            POP_REAL(a); POP_REAL(b);
            double res = b / a;
            PUSH_REAL(res);
            break;
        }
        case MOD_INT: {
            int a, b;
            POP_INT(a); POP_INT(b);
            int res = b % a;
            PUSH_INT(res);
            break;
        }
        case MOD_REAL: {
            double a, b;
            POP_REAL(a); POP_REAL(b);
            double res = dmod(b, a);
            PUSH_REAL(res);
            break;
        }
        case FDIV_INT: {
            int a, b;
            POP_INT(a); POP_INT(b);
            int res = (int)(b / a);
            PUSH_INT(res);
            break;
        }
        case FDIV_REAL: {
            double a, b;
            POP_REAL(a); POP_REAL(b);
            int res = (int)(b / a);
            PUSH_INT(res);
            break;
        }
        case POW_INT: {
            int a, b;
            POP_INT(a); POP_INT(b);
            double res = pow(b, a);
            PUSH_REAL(res);
            break;
        }
        case POW_REAL: {
            double a, b;
            POP_REAL(a); POP_REAL(b);
            double res = pow(b, a);
            PUSH_REAL(res);
            break;
        }
        case CONCAT: {
            void* ref1, *ref2;
            POP_REF(ref1); POP_REF(ref2);

            if (!isValidReference(&vm->mem, ref1) || !isValidReference(&vm->mem, ref2)) {
                runtimeError(vm, "Segmentation fault.");
                break;
            }

            Obj* str2 = (Obj*)ref1;
            Obj* str1 = (Obj*)ref2;

            Obj* res = concatStrings(vm, str1, str2);
            if (res == NULL) {
                runtimeError(vm, "Not enough memory available for string allocation.");
                break;
            }

            //void* resRef = (void*)res;

            PUSH_REF(res);
            break;
        }
        case EQ_INT: {
            int a, b;
            POP_INT(a); POP_INT(b);
            bool res = a==b;

            PUSH_BOOL(res);
            break;
        }
        case EQ_REAL: {
            double a, b;
            POP_REAL(a); POP_REAL(b);
            bool res = a==b;

            PUSH_BOOL(res);
            break;
        }
        case EQ_BOOL: {
            bool a, b;
            POP_BOOL(a); POP_BOOL(b);
            bool res = a==b;

            PUSH_BOOL(res);
            break;
        }
        case EQ_REF: {
            void* a, *b;
            POP_REF(a); POP_REF(b);
            bool res = a==b;

            PUSH_BOOL(res);
            break;
        }
        case EQ_STRING: {
            void* a, *b;
            POP_REF(a); POP_REF(b);

            if (!isValidReference(&vm->mem, a) || !isValidReference(&vm->mem, b)) {
                runtimeError(vm, "Segmentation fault.");
                break;
            }

            Obj* str1 = (Obj*)a;
            Obj* str2 = (Obj*)b;

            bool res = cmpStrings(str1, str2) == 0;
            PUSH_BOOL(res);
            break;
        }
        case LESS_INT: {
            int a, b;
            POP_INT(a); POP_INT(b);
            bool res = b < a;
            PUSH_BOOL(res);
            break;
        }
        case LESS_REAL: {
            double a, b;
            POP_REAL(a); POP_REAL(b);
            bool res = b < a;
            PUSH_BOOL(res);
            break;
        }
        case LESS_BOOL: {
            bool a, b;
            POP_BOOL(a); POP_BOOL(b);
            bool res = b < a;
            PUSH_BOOL(res);
            break;
        }
        case LESS_REF: {
            void* a, *b;
            POP_REF(a); POP_REF(b);
            bool res = b < a;
            PUSH_BOOL(res);
            break;
        }
        case LESS_STRING: {
            void* a, *b;
            POP_REF(a); POP_REF(b);

            if (!isValidReference(&vm->mem, a) || !isValidReference(&vm->mem, b)) {
                runtimeError(vm, "Segmentation fault.");
                break;
            }

            Obj* str1 = (Obj*)b;
            Obj* str2 = (Obj*)a;

            bool res = cmpStrings(str1, str2) < 0;
            PUSH_BOOL(res);
            break;
        }
        case LESS_EQ_INT: {
            int a, b;
            POP_INT(a); POP_INT(b);
            bool res = b <= a;
            PUSH_BOOL(res);
            break;
        }
        case LESS_EQ_REAL: {
            double a, b;
            POP_REAL(a); POP_REAL(b);
            bool res = b <= a;
            PUSH_BOOL(res);
            break;
        }
        case LESS_EQ_BOOL: {
            bool a, b;
            POP_BOOL(a); POP_BOOL(b);
            bool res = b <= a;
            PUSH_BOOL(res);
            break;
        }
        case LESS_EQ_REF: {
            void* a, *b;
            POP_REF(a); POP_REF(b);
            bool res = b <= a;
            PUSH_BOOL(res);
            break;
        }
        case LESS_EQ_STRING: {
            void* a, *b;
            POP_REF(a); POP_REF(b);

            if (!isValidReference(&vm->mem, a) || !isValidReference(&vm->mem, b)) {
                runtimeError(vm, "Segmentation fault.");
                break;
            }

            Obj* str1 = (Obj*)b;
            Obj* str2 = (Obj*)a;

            bool res = cmpStrings(str1, str2) <= 0;
            PUSH_BOOL(res);
            break;
        }
        case NEQ_INT: {
            int a, b;
            POP_INT(a); POP_INT(b);
            bool res = a!=b;

            PUSH_BOOL(res);
            break;
        }
        case NEQ_REAL: {
            double a, b;
            POP_REAL(a); POP_REAL(b);
            bool res = a!=b;

            PUSH_BOOL(res);
            break;
        }
        case NEQ_BOOL: {
            bool a, b;
            POP_BOOL(a); POP_BOOL(b);
            bool res = a!=b;

            PUSH_BOOL(res);
            break;
        }
        case NEQ_REF: {
            void* a, *b;
            POP_REF(a); POP_REF(b);
            bool res = a!=b;

            PUSH_BOOL(res);
            break;
        }
        case NEQ_STRING: {
            void* a, *b;
            POP_REF(a); POP_REF(b);

            if (!isValidReference(&vm->mem, a) || !isValidReference(&vm->mem, b)) {
                runtimeError(vm, "Segmentation fault.");
                break;
            }

            Obj* str1 = (Obj*)a;
            Obj* str2 = (Obj*)b;

            bool res = cmpStrings(str1, str2) != 0;
            PUSH_BOOL(res);
            break;
        }
        case GREATER_INT: {
            int a, b;
            POP_INT(a); POP_INT(b);
            bool res = b > a;

            PUSH_BOOL(res);
            break;
        }
        case GREATER_REAL: {
            double a, b;
            POP_REAL(a); POP_REAL(b);
            bool res = b > a;

            PUSH_BOOL(res);
            break;
        }
        case GREATER_BOOL: {
            bool a, b;
            POP_BOOL(a); POP_BOOL(b);
            bool res = b > a;

            PUSH_BOOL(res);
            break;
        }
        case GREATER_REF: {
            void* a, *b;
            POP_REF(a); POP_REF(b);
            bool res = b > a;

            PUSH_BOOL(res);
            break;
        }
        case GREATER_STRING: {
            void* a, *b;
            POP_REF(a); POP_REF(b);

            if (!isValidReference(&vm->mem, a) || !isValidReference(&vm->mem, b)) {
                runtimeError(vm, "Segmentation fault.");
                break;
            }

            Obj* str1 = (Obj*)b;
            Obj* str2 = (Obj*)a;

            bool res = cmpStrings(str1, str2) > 0;
            PUSH_BOOL(res);
            break;
        }
        case GREATER_EQ_INT: {
            int a, b;
            POP_INT(a); POP_INT(b);
            bool res = b >= a;

            PUSH_BOOL(res);
            break;
        }
        case GREATER_EQ_REAL: {
            double a, b;
            POP_REAL(a); POP_REAL(b);
            bool res = b >= a;

            PUSH_BOOL(res);
            break;
        }
        case GREATER_EQ_BOOL: {
            bool a, b;
            POP_BOOL(a); POP_BOOL(b);
            bool res = b >= a;

            PUSH_BOOL(res);
            break;
        }
        case GREATER_EQ_REF: {
            void* a, *b;
            POP_REF(a); POP_REF(b);
            bool res = b >= a;

            PUSH_BOOL(res);
            break;
        }
        case GREATER_EQ_STRING: {
            void* a, *b;
            POP_REF(a); POP_REF(b);

            if (!isValidReference(&vm->mem, a) || !isValidReference(&vm->mem, b)) {
                runtimeError(vm, "Segmentation fault.");
                break;
            }

            Obj* str1 = (Obj*)b;
            Obj* str2 = (Obj*)a;

            bool res = cmpStrings(str1, str2) >= 0;
            PUSH_BOOL(res);
            break;
        }
        case AND: {
            bool a, b;
            POP_BOOL(a); POP_BOOL(b);

            bool res = a && b;
            PUSH_BOOL(res);
            break;
        }
        case OR: {
            bool a, b;
            POP_BOOL(a); POP_BOOL(b);

            bool res = a || b;
            PUSH_BOOL(res);
            break;
        }
        case NEG_INT: {
            int a;
            POP_INT(a);
            a *= -1;
            PUSH_INT(a);
            break;
        }
        case NEG_REAL: {
            double a;
            POP_REAL(a);
            a *= -1;
            PUSH_REAL(a);
            break;
        }
        case NOT: {
            bool a;
            POP_BOOL(a);
            a = !a;
            PUSH_BOOL(a);
            break;
        }
        case POP_1B: {
            byte temp; POP_BYTE(temp);
            break;
        }
        case POP_4B: {
            byte4 temp;
            POP_4BYTE(temp);
            break;
        }
        case POP_8B: {
            byte8 temp;
            POP_8BYTE(temp);
            break;
        }
        case COPY_INT: {
            int a;
            POP_INT(a);
            PUSH_INT(a);
            PUSH_INT(a);
            break;
        }
        case INPUT_INT: {
            //clearInputBuffer();
            int num;
            int res = scanf("%d", &num);
            clearInputBuffer();

            if (res <= 0) {
                runtimeError(vm, "I/O error.");
                break;
            }

            PUSH_INT(num);
            break;
        }
        case INPUT_REAL: {
            //clearInputBuffer();
            double num;
            int res = scanf("%lf", &num);
            clearInputBuffer();

            if (res <= 0) {
                runtimeError(vm, "I/O error.");
                break;
            }

            PUSH_REAL(num);
            break;
        }
        case INPUT_CHAR: {
            //clearInputBuffer();
            char c;
            int res = scanf("%c", &c);
            clearInputBuffer();

            if (res <= 0) {
                runtimeError(vm, "I/O error.");
                break;
            }

            PUSH_CHAR(c);
            break;
        }
        case INPUT_BOOL: {
            //clearInputBuffer();
            char boolean[10];

            int res = scanf("%s", boolean);
            clearInputBuffer();

            if (res <= 0) {
                runtimeError(vm, "I/O error.");
                break;
            }

            bool b = memcmp(boolean, "TRUE", 4) == 0 || memcmp(boolean, "true", 4) == 0 || memcmp(boolean, "True", 4) == 0;

            PUSH_BOOL(b);
            break;
        }
        case INPUT_STRING: {
            //clearInputBuffer();
            int currSize = 128;
            char* buff = malloc(sizeof(char) * currSize);
            if (buff == NULL) {
                runtimeError(vm, "I/O error.");
                break;
            }

            int length = 0;

            while (true) {
                char c;
                int res = scanf("%c", &c);
                if (res <= 0) {
                    runtimeError(vm, "I/O error.");
                    free(buff);
                    length = -1;
                    break;
                }

                if (c == '\n') break;

                if (length >= currSize) {
                    currSize *= 2;
                    char* orig = buff;
                    buff = realloc(buff, sizeof(char) * currSize);

                    if (buff == NULL) {
                        runtimeError(vm , "I/O error.");
                        length = -1;
                        free(orig);
                        break;
                    }
                }

                buff[length] = c;
                length++;
            }

            if (length < 0) {
                break;
            }

            char* strBuff = malloc(sizeof(char) * length);

            if (strBuff == NULL) {
                runtimeError(vm, "I/O error.");
                free(buff);
                break;
            }

            for (int i = 0; i < length; i++) {
                strBuff[i] = buff[i];
            }

            free(buff);

            Obj* strPtr = allocString(&vm->mem, strBuff, length);

            if (strPtr == NULL) {
                free(strBuff);
                runtimeError(vm, "I/O error.");
                break;
            }

            PUSH_REF(strPtr);
            break;
        }
        case OUTPUT_INT: {
            int a;
            POP_INT(a);
            printf("%d", a);
            break;
        }
        case OUTPUT_REAL: {
            double a;
            POP_REAL(a);
            printf("%f", a);
            break;
        }
        case OUTPUT_CHAR: {
            char c;
            POP_CHAR(c);
            printf("%c", c);
            break;
        }
        case OUTPUT_BOOL: {
            bool a;
            POP_BOOL(a);
            printf(a ? "TRUE" : "FALSE");
            break;
        }
        case OUTPUT_REF: {
            void* ref;
            POP_REF(ref);
            printf("[%p]", ref);
            break;
        }
        case OUTPUT_STRING: {
            void* a;
            POP_REF(a);

            if (!isValidReference(&vm->mem, a)) {
                runtimeError(vm, "Segmentation fault.");
                break;
            }

            Obj* str = (Obj*)a;

            for (int i = 0; i < str->as.StringObj.length; i++) {
                printf("%c", str->as.StringObj.start[i]);
            }

            break;
        }
        case OUTPUT_NL: {
            printf("\n");
            break;
        }
        case READ_LINE: {
            void* ref;
            POP_REF(ref);

            if (!isValidReference(&vm->mem, ref)) {
                runtimeError(vm, "Segmentation fault.");
                break;
            }

            Obj* file = (Obj*)ref;

            int currSize = 128;
            char* buff = malloc(sizeof(char) * currSize);
            if (buff == NULL) {
                runtimeError(vm, "I/O error.");
                break;
            }

            int length = 0;

            while (true) {
                int c = fgetc(file->as.FileObj.filePtr);
                if (c == EOF) {
                    if (ferror(file->as.FileObj.filePtr)) {
                        runtimeError(vm, "Error reading file.");
                        free(buff);
                        length = -1;
                        break;
                    }
                }

                if (c == '\n' || c == EOF) break;

                if (length >= currSize) {
                    currSize *= 2;
                    char* orig = buff;
                    buff = realloc(buff, sizeof(char) * currSize);

                    if (buff == NULL) {
                        runtimeError(vm , "I/O error.");
                        length = -1;
                        free(orig);
                        break;
                    }
                }

                buff[length] = (char)c;
                length++;
            }

            if (length < 0) {
                break;
            }

            char* strBuff = malloc(sizeof(char) * length);

            if (strBuff == NULL) {
                runtimeError(vm, "I/O error.");
                free(buff);
                break;
            }

            for (int i = 0; i < length; i++) {
                strBuff[i] = buff[i];
            }

            free(buff);

            Obj* strPtr = allocString(&vm->mem, strBuff, length);

            if (strPtr == NULL) {
                free(strBuff);
                runtimeError(vm, "I/O error.");
                break;
            }

            PUSH_REF(strPtr);
            break;
        }
        case WRITE_INT: {
            void* ref;
            POP_REF(ref);

            if (!isValidReference(&vm->mem, ref)) {
                runtimeError(vm, "Segmentation fault.");
                break;
            }

            Obj* file = (Obj*)ref;

            int a;
            POP_INT(a);

            fprintf(file->as.FileObj.filePtr, "%d", a);

            break;
        }
        case WRITE_REAL: {
            void* ref;
            POP_REF(ref);

            if (!isValidReference(&vm->mem, ref)) {
                runtimeError(vm, "Segmentation fault.");
                break;
            }

            Obj* file = (Obj*)ref;

            double a;
            POP_REAL(a);

            fprintf(file->as.FileObj.filePtr, "%f", a);

            break;
        }
        case WRITE_CHAR: {
            void* ref;
            POP_REF(ref);

            if (!isValidReference(&vm->mem, ref)) {
                runtimeError(vm, "Segmentation fault.");
                break;
            }

            Obj* file = (Obj*)ref;

            char a;
            POP_CHAR(a);

            fprintf(file->as.FileObj.filePtr, "%c", a);

            break;
        }
        case WRITE_BOOL: {
            void* ref;
            POP_REF(ref);

            if (!isValidReference(&vm->mem, ref)) {
                runtimeError(vm, "Segmentation fault.");
                break;
            }

            Obj* file = (Obj*)ref;

            bool a;
            POP_BOOL(a);

            fprintf(file->as.FileObj.filePtr, a ? "TRUE" : "FALSE");

            break;
        }
        case WRITE_REF: {
            void* ref;
            POP_REF(ref);

            if (!isValidReference(&vm->mem, ref)) {
                runtimeError(vm, "Segmentation fault.");
                break;
            }

            Obj* file = (Obj*)ref;

            void* a;
            POP_REF(a);

            fprintf(file->as.FileObj.filePtr, "[%p]", a);

            break;
        }
        case WRITE_STRING: {
            void* ref;
            POP_REF(ref);

            if (!isValidReference(&vm->mem, ref)) {
                runtimeError(vm, "Segmentation fault.");
                break;
            }

            Obj* file = (Obj*)ref;

            void* a;
            POP_REF(a);

            if (!isValidReference(&vm->mem, a)) {
                runtimeError(vm, "Segmentation fault.");
                break;
            }

            Obj* str = (Obj*)a;

            for (int i = 0; i < str->as.StringObj.length; i++) {
                fprintf(file->as.FileObj.filePtr, "%c", str->as.StringObj.start[i]);
            }

            break;
        }
        case WRITE_NL: {
            void* ref;
            POP_REF(ref);

            if (!isValidReference(&vm->mem, ref)) {
                runtimeError(vm, "Segmentation fault.");
                break;
            }

            Obj* file = (Obj*)ref;

            fprintf(file->as.FileObj.filePtr, "\n");
            break;
        }
        case OPENFILE: {
            int accessType;
            POP_INT(accessType);

            void* a;
            POP_REF(a);

            if (!isValidReference(&vm->mem, a)) {
                runtimeError(vm, "Segmentation fault.");
                break;
            }

            Obj* str = (Obj*)a;

            char* name = extractNullTerminatedString(str->as.StringObj.start, str->as.StringObj.length);

            Obj* file = allocFile(&vm->mem, name, accessType);

            free(name);

            if (file == NULL) {
                runtimeError(vm, "Error opening file.");
                break;
            }

            PUSH_REF(file);
            break;
        }
        case CLOSEFILE: {
            void* ref;
            POP_REF(ref);

            if (!isValidReference(&vm->mem, ref)) {
                runtimeError(vm, "Segmentation fault.");
                break;
            }

            markForceFree(&vm->mem, ref);

            Obj* file = (Obj*)ref;

            fclose(file->as.FileObj.filePtr);
            break;
        }
        /*case RINPUT_INT: {
        }
        case RINPUT_REAL: {
        }
        case RINPUT_CHAR: {
        }
        case RINPUT_BOOL: {
        }
        case RINPUT_STRING: {
        }*/
        case B_FALSE: {
            int newPC; READ_INT(newPC, vm->PC + 1);
            vm->PC += 4;

            bool cond;
            POP_BOOL(cond);
            if (!cond) {
                jmpTo(vm, newPC);
            }
            break;
        }
        case BRANCH: {
            int newPC; READ_INT(newPC, vm->PC + 1);
            vm->PC += 4;
            jmpTo(vm, newPC);

            break;
        }
        case GET_REF: {
            int pos;
            POP_INT(pos);

            void* ref = (void*)(&vm->stack.data[pos]);
            PUSH_REF(ref);
            break;
        }
        case RGET_REF: {
            int base = getBaseStackPos(&vm->callStack);
            int pos; POP_INT(pos);

            void* ref = (void*)(&vm->stack.data[pos]);
            PUSH_REF(ref);
            break;
        }
        case EXIT: {
            vm->PC = -10;
            return;
        }
        default: break;
    }
}

static void markReferences(VM* vm) {

#define READSTACK_8BYTE(idx) (((byte8)vm->stack->data[idx + 7] << 56) | ((byte8)vm->stack->data[idx + 6] << 48) | ((byte8)vm->stack->data[idx + 5] << 40) | ((byte8)vm->stack->data[idx + 4] << 32) | ((byte8)vm->stack->data[idx + 3] << 24) | ((byte8)vm->stack->data[idx + 2] << 16) | ((byte8)vm->stack->data[idx + 1] << 8) | ((byte8)vm->stack->data[idx]))


    for (int i = 0; i <= vm->stack.top; i++) {
        if (vm->stack.data[i].isRef) {
            byte8 b = READ_8BYTE(i);
            void* ref = *(void**)(&b);

            if (isValidReference(&vm->mem, ref)) {
                markCell(&vm->mem, ref);
                i += 7;
            }
        }
    }

#undef READSTACK_8BYTE
}

void run(VM* vm) {
    while (vm->PC >= 0 && !vm->hadRuntimeError) {
        //printf("RUNNING instruction %d\n", vm->PC);
        runInstruction(vm);
        //showStack(&vm->stack);
        advance(vm);

        if (vm->mem.inUse >= vm->mem.memSize * 0.75) {
            markReferences(vm);
            size_t memCollected = collectGarbage(&vm->mem);
            printf("GARBAGE COLLECTOR COLLECTED %zu bytes.", memCollected);
        }
    }

    printf("Program executed correctly.\n");
}