#ifndef PSEUDOCOMPILER_OBJECT_H
#define PSEUDOCOMPILER_OBJECT_H

#include "common.h"
#include "semantic.h"

typedef enum {
    OBJ_NONE, OBJ_STRING, OBJ_ARRAY, OBJ_FILE
} ObjType;

typedef struct {
    ObjType type;

    union {
        struct {
            int length;
            char* start;
        } StringObj;

        struct {
            int length;
            int width;
            int x0;
            int y0;
            size_t elemSize;
            byte* start;
        } ArrayObj;

        struct {
            FILE* filePtr;
            FileAccessType accessType;
        } FileObj;
    } as;
} Obj;

void freeObj(Obj* obj);
void createString(Obj* obj, const char* chars, int length);
void createArray(Obj* obj, int length, int width, int x0, int y0, size_t elemSize);
void createFile(Obj* obj, const char* filename, FileAccessType accessType);


#endif //PSEUDOCOMPILER_OBJECT_H
