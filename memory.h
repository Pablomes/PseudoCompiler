#ifndef PSEUDOCOMPILER_MEMORY_H
#define PSEUDOCOMPILER_MEMORY_H

#include "object.h"
#include "common.h"

#define GROW_CAPACITY(cap, min)     (((cap) < (min)) ? (min) : (cap * 2))

typedef struct MemoryCell {
    Obj obj;
    bool marked;
    bool free;
    bool forceFree;
    struct MemoryCell* nextFree;
} MemoryCell;

typedef struct {
    MemoryCell* memBlock;
    size_t  memSize;
    size_t inUse;
    MemoryCell* free;
} ProgramMemory;

void createProgramMemory(ProgramMemory* mem, int numCells);
void freeProgramMemory(ProgramMemory* mem);

Obj* allocString(ProgramMemory* mem, const char* chars, int length);
Obj* allocArray(ProgramMemory* mem, int length, int width, int x0, int y0, size_t elemSize);
Obj* allocFile(ProgramMemory* mem, const char* filename, FileAccessType accessType);

bool inProgramMemory(ProgramMemory* mem, void* ptr);
bool isValidReference(ProgramMemory* mem, void* ptr);
void markCell(ProgramMemory* mem, void* ptr);
void markForceFree(ProgramMemory* mem, void* ptr);

size_t collectGarbage(ProgramMemory* mem);

#endif //PSEUDOCOMPILER_MEMORY_H
