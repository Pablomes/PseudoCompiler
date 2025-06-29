
#include "object.h"

void freeObj(Obj* obj) {
    switch (obj->type) {
        case OBJ_STRING: {
            obj->as.StringObj.length = 0;
            free(obj->as.StringObj.start);
            break;
        }
        case OBJ_ARRAY: {
            obj->as.ArrayObj.length = 0;
            obj->as.ArrayObj.width = 0;
            obj->as.ArrayObj.x0 = 0;
            obj->as.ArrayObj.y0 = 0;
            obj->as.ArrayObj.elemSize = 0;
            free(obj->as.ArrayObj.start);
            break;
        }
        default: break;
    }
    obj->type = OBJ_NONE;
}

void createString(Obj* obj, const char* chars, int length) {
    obj->type = OBJ_STRING;

    obj->as.StringObj.length = length;
    char* buff = (char*) malloc(length * sizeof(char));

    if (buff == NULL) {
        printf("Not enough memory available to allocate string");
        exit(-1);
    }

    memcpy(buff, chars, length * sizeof(char));

    obj->as.StringObj.start = buff;
}

void createArray(Obj* obj, int length, int width, int x0, int y0, size_t elemSize) {
    obj->type = OBJ_ARRAY;

    obj->as.ArrayObj.length = length;
    obj->as.ArrayObj.width = width;
    obj->as.ArrayObj.x0 = x0;
    obj->as.ArrayObj.y0 = y0;
    obj->as.ArrayObj.elemSize = elemSize;

    obj->as.ArrayObj.start = (byte*) malloc(elemSize * length * width);
}