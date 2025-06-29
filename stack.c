
#include "stack.h"

void initStack(Stack* stack, int capacity) {
    stack->data = (StackElem*)malloc(capacity * sizeof(StackElem));
    if (stack->data == NULL) {
        fprintf(stderr, "Failed to allocate memory for stack.\n");
        exit(-1);
    }
    stack->top = -1;
    stack->capacity = capacity;
}

void freeStack(Stack* stack) {
    free(stack->data);
    stack->data = NULL;
    stack->top = -1;
    stack->capacity = 0;
}

bool isStackEmpty(Stack* stack) {
    return stack->top == -1;
}

bool isStackFull(Stack* stack) {
    return stack->top == stack->capacity - 1;
}

bool push(Stack* stack, byte value, bool isRef) {
    if (isStackFull(stack)) {
        fprintf(stderr, "Stack overflow.\n");
        return false;
    }
    stack->data[++stack->top].value = value;
    stack->data[stack->top].isRef = isRef;
    return true;
}

byte pop(Stack* stack) {
    if (isStackEmpty(stack)) {
        fprintf(stderr, "Stack underflow.\n");
        exit(-1);
    }
    return stack->data[stack->top--].value;
}

byte peek(Stack* stack) {
    if (isStackEmpty(stack)) {
        fprintf(stderr, "Stack is empty.\n");
        exit(-1);
    }
    return stack->data[stack->top].value;
}

byte getAt(Stack* stack, int pos) {
    if (pos < 0 || pos >= stack->capacity) return 0;

    return stack->data[pos].value;
}

bool isRefAt(Stack* stack, int pos) {
    if (pos < 0 || pos >= stack->capacity) return false;

    return stack->data[pos].isRef;
}

void setAt(Stack* stack, byte value, bool isRef, int pos) {
    if (pos < 0 || pos >= stack->capacity) return;

    stack->data[pos].value = value;
    stack->data[pos].isRef = isRef;
}

int getNextFree(Stack* stack) {
    return stack->top + 1;
}

void* getMemRefAt(Stack* stack, int pos) {
    if (pos < 0 || pos >= stack->capacity) return NULL;

    return &stack->data[pos];
}

bool isStackRef(Stack* stack, void* ptr) {
    if (ptr < (void*)stack->data || ptr >= (void*)(stack->data) + stack->capacity * sizeof(StackElem)) {
        return false;
    }
    return true;
}

void showStack(Stack* stack) {
    for (int i = stack->top; i >= 0; i--) {
        printf("[ %x ]", stack->data[i].value);
        printf(stack->data[i].isRef ? " <- ref start\n" : "\n");
    }
}

void initCallStack(CallStack* stack, int capacity) {
    stack->frames = (CallFrame*)malloc(capacity * sizeof(CallFrame));
    if (stack->frames == NULL) {
        fprintf(stderr, "Failed to allocate memory for call stack.\n");
        exit(-1);
    }
    stack->top = -1;
    stack->capacity = capacity;
}

void freeCallStack(CallStack* stack) {
    free(stack->frames);
    stack->frames = NULL;
    stack->top = -1;
    stack->capacity = 0;
}

bool isCallStackEmpty(CallStack* stack) {
    return stack->top == -1;
}

bool pushCallFrame(CallStack* stack, long returnPC, int baseStackPos) {
    if (stack->top == stack->capacity - 1) {
        fprintf(stderr, "Call stack overflow.\n");
        return false;
    }
    stack->frames[++stack->top].returnPC = returnPC;
    stack->frames[stack->top].baseStackPos = baseStackPos;
    return true;
}

long popCallFrame(CallStack* stack) {
    if (isCallStackEmpty(stack)) {
        fprintf(stderr, "Call stack underflow.\n");
        exit(-1);
    }
    return stack->frames[stack->top--].returnPC;
}

int getBaseStackPos(CallStack* stack) {
    if (isCallStackEmpty(stack)) {
        fprintf(stderr, "Call stack is empty.\n");
        exit(-1);
    }
    return stack->frames[stack->top].baseStackPos;
}