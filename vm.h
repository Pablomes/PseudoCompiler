#ifndef PSEUDOCOMPILER_VM_H
#define PSEUDOCOMPILER_VM_H

#include <math.h>

#include "common.h"
#include "bytecode.h"
#include "memory.h"
#include "stack.h"
#include "object.h"

typedef struct {
    ProgramMemory mem;
    Stack stack;
    CallStack callStack;
    BytecodeStream* program;
    int PC;
    bool hadRuntimeError;
    int nextCallBase;
    long callPC;
} VM;

void initVM(VM* vm, int heapCapacity, int stackCapacity, int callStackCapacity, BytecodeStream* bStream);
void freeVM(VM* vm);

void run(VM* vm, bool debug);

#endif //PSEUDOCOMPILER_VM_H
