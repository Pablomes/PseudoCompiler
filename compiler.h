#ifndef PSEUDOCOMPILER_COMPILER_H
#define PSEUDOCOMPILER_COMPILER_H

#include "common.h"
#include "parser.h"
#include "symbol.h"
#include "bytecode.h"


typedef struct {
    SymbolTable* globalTable;
    SymbolTable* symbolTable;
    int depth;
    BytecodeStream* bStream;
    int stackPos;
    int lastCaseJumpPos;
} Compiler;

void initCompiler(Compiler* compiler, BytecodeStream* bStream);
void freeCompiler(Compiler* compiler);
bool compile(Compiler* compiler, AST* ast);
void printCompileResult(Compiler* compiler);

#endif //PSEUDOCOMPILER_COMPILER_H
