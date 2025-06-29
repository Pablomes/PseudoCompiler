#ifndef PSEUDOCOMPILER_COMMON_H
#define PSEUDOCOMPILER_COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t byte;
typedef uint16_t byte2;
typedef uint32_t byte4;
typedef uint64_t byte8;

char* extractNullTerminatedString(const char* start, int length);

#endif //PSEUDOCOMPILER_COMMON_H
