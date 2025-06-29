
#include "common.h"

char* extractNullTerminatedString(const char* start, int length) {
    // Allocate memory for the new string (+1 for the null terminator)
    char* result = (char*)malloc(length + 1);
    if (!result) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    // Copy the substring into the new buffer
    strncpy(result, start, length);

    // Null-terminate the string
    result[length] = '\0';

    return result;
}
