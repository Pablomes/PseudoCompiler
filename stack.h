#ifndef PSEUDOCOMPILER_STACK_H
#define PSEUDOCOMPILER_STACK_H

#include "common.h"

typedef struct {
    byte value;
    bool isRef;
} StackElem;

typedef struct {
    StackElem* data;
    int top;
    int capacity;
} Stack;

void initStack(Stack* stack, int capacity);
void freeStack(Stack* stack);
bool isStackEmpty(Stack* stack);
bool isStackFull(Stack* stack);
bool push(Stack* stack, byte value, bool isRef);
byte pop(Stack* stack);
byte peek(Stack* stack);
byte getAt(Stack* stack, int pos);
bool isRefAt(Stack* stack, int pos);
void setAt(Stack* stack, byte value, bool isRef, int pos);
int getNextFree(Stack* stack);
void* getMemRefAt(Stack* stack, int pos);
bool isStackRef(Stack* stack, void* ptr);

void showStack(Stack* stack);

typedef struct {
    long returnPC;
    int baseStackPos;
} CallFrame;

typedef struct {
    CallFrame* frames;
    int top;
    int capacity;
} CallStack;

void initCallStack(CallStack* stack, int capacity);
void freeCallStack(CallStack* stack);
bool isCallStackEmpty(CallStack* stack);
bool pushCallFrame(CallStack* stack, long returnPC, int baseStackPos);
long popCallFrame(CallStack* stack);
int getBaseStackPos(CallStack* stack);


#endif //PSEUDOCOMPILER_STACK_H
