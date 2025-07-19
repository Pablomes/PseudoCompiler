
#include "memory.h"

void createProgramMemory(ProgramMemory* mem, int numCells) {
    mem->memSize = numCells * sizeof(MemoryCell);
    mem->inUse = 0;

    mem->memBlock = (MemoryCell*) malloc(mem->memSize);
    if (mem->memBlock == NULL) {
        fprintf(stderr, "Problem allocating program memory block. Machine will abort now.\n");
        exit(-1);
    }

    mem->free = mem->memBlock;

    for (int i = 0; i < numCells - 1; i++) {
        mem->memBlock[i].nextFree = &(mem->memBlock[i + 1]);
        mem->memBlock[i].free = true;
        mem->memBlock[i].forceFree = false;
        mem->memBlock[i].marked = false;
    }
    mem->memBlock[numCells - 1].nextFree = NULL;
    mem->memBlock[numCells - 1].free = true;
    mem->memBlock[numCells - 1].forceFree = false;
    mem->memBlock[numCells - 1].marked = false;
}

static void freeCell(MemoryCell* cell, ProgramMemory* mem) {
    cell->marked = false;
    cell->nextFree = mem->free;
    mem->free = cell;
    cell->free = true;
    cell->forceFree = false;
    mem->inUse--;

    freeObj(&cell->obj);
}

void freeProgramMemory(ProgramMemory* mem) {
    size_t numCells = mem->memSize / sizeof(MemoryCell);

    for (size_t i = 0; i < numCells; i++) {
        MemoryCell* cell = &mem->memBlock[i];
        freeCell(cell, mem);
    }

    mem->inUse = 0;

    free(mem->memBlock);
}

Obj* allocString(ProgramMemory* mem, const char* chars, int length) {
    if (mem->free == NULL) return NULL;

    MemoryCell* cell = mem->free;
    mem->free = mem->free->nextFree;

    cell->nextFree = NULL;
    cell->free = false;

    createString(&cell->obj, chars, length);

    if (cell->obj.as.StringObj.start == NULL) return NULL;

    mem->inUse++;

    return &cell->obj;
}

Obj* allocArray(ProgramMemory* mem, int length, int width, int x0, int y0, size_t elemSize) {
    if (mem->free == NULL) return NULL;

    MemoryCell* cell = mem->free;
    mem->free = mem->free->nextFree;

    cell->nextFree = NULL;
    cell->free = false;

    createArray(&cell->obj, length, width, x0, y0, elemSize);

    if (cell->obj.as.ArrayObj.start == NULL) return NULL;

    mem->inUse++;

    return &cell->obj;
}

Obj* allocFile(ProgramMemory* mem, const char* filename, FileAccessType accessType) {
    if (mem->free == NULL) return NULL;

    MemoryCell* cell = mem->free;
    mem->free = mem->free->nextFree;

    cell->nextFree = NULL;
    cell->free = false;

    createFile(&cell->obj, filename, accessType);

    if (cell->obj.as.FileObj.filePtr == NULL) return NULL;

    mem->inUse++;

    return &cell->obj;
}

bool inProgramMemory(ProgramMemory* mem, void* ptr) {
    if (ptr >= (void*)mem->memBlock && ptr < (void*)mem->memBlock + mem->memSize) {
        return true;
    }
    return false;
}

bool isValidReference(ProgramMemory* mem, void* ptr) {
    if (!inProgramMemory(mem, ptr)) return false;

    if (((MemoryCell*)ptr)->free) return false;

    return true;
}

void markCell(ProgramMemory* mem, void* ptr) {
    if (!inProgramMemory(mem, ptr)) return;

    if (((MemoryCell*)ptr)->obj.type == OBJ_ARRAY && ((MemoryCell*)ptr)->obj.as.ArrayObj.elemSize == 8) {
        for (int i = 0; i < ((MemoryCell*)ptr)->obj.as.ArrayObj.length * ((MemoryCell*)ptr)->obj.as.ArrayObj.width; i+=8) {
            byte8 strPtr = (((byte8)((MemoryCell*)ptr)->obj.as.ArrayObj.start[i]) << 56) | (((byte8)((MemoryCell*)ptr)->obj.as.ArrayObj.start[i + 1]) << 48) | (((byte8)((MemoryCell*)ptr)->obj.as.ArrayObj.start[i + 2]) << 40) | (((byte8)((MemoryCell*)ptr)->obj.as.ArrayObj.start[i + 3]) << 32) | (((byte8)((MemoryCell*)ptr)->obj.as.ArrayObj.start[i + 4]) << 24) | (((byte8)((MemoryCell*)ptr)->obj.as.ArrayObj.start[i + 5]) << 16) | (((byte8)((MemoryCell*)ptr)->obj.as.ArrayObj.start[i + 6]) << 8) | (((byte8)((MemoryCell*)ptr)->obj.as.ArrayObj.start[i + 7]));
            markCell(mem, (void*)strPtr);
        }
    }

    ((MemoryCell*)ptr)->marked = true;
}

void markForceFree(ProgramMemory* mem, void* ptr) {
    if (!inProgramMemory(mem, ptr)) return;

    ((MemoryCell*)ptr)->forceFree = true;
}

size_t collectGarbage(ProgramMemory* mem) {
    size_t collected = 0;

    size_t numCells = mem->memSize / sizeof(MemoryCell);

    for (size_t i = 0; i < numCells; i++) {
        MemoryCell* cell = &mem->memBlock[i];

        if (!cell->marked || cell->forceFree) {
            freeCell(cell, mem);
            collected += sizeof(Obj);
        }

        cell->marked = false;
        cell->forceFree = false;
    }

    return collected;
}