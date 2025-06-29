#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"
#include "semantic.h"
#include "bytecode.h"
#include "compiler.h"
#include "vm.h"

static char* readFile(const char* path) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }

    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(fileSize + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(74);
    }

    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize) {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(74);
    }

    buffer[bytesRead] = '\0';

    fclose(file);
    return buffer;
}

/*static void runFile(const char* path) {
    char* source = readFile(path);
    InterpretResult result = interpret(source);
    free(source);

    if (result == INTERPRET_COMPILE_ERROR) exit(65);
    if (result == INTERPRET_RUNTIME_ERROR) exit(70);
}

int main(int argc, char* argv[]) {
    initVM();

    if (argc == 1) {
        repl();
    } else if (argc == 2) {
        runFile(argv[1]);
    } else {
        fprintf(stderr, "usage: clox [path]\n");
        exit(64);
    }

    freeVM();
    return 0;
}*/

static void runFile(const char* path) {
    char* source = readFile(path);

    Lexer lexer;
    initLexer(&lexer, source);

    scanSource(&lexer);

    //printTokens(&lexer);

    Parser parser;
    initParser(&parser, lexer.array);

    bool res = genAST(&parser);

    if (res) {
        freeLexer(&lexer);
        freeParser(&parser);
    }

    Analyser analyser;
    initAnalyser(&analyser);

    bool semRes = semanticAnalysis(&analyser, &parser.ast);

    if (semRes) {
        freeLexer(&lexer);
        freeParser(&parser);
        freeAnalyser(&analyser);
        return;
    }

    printf("ANALYSED CORRECTLY\n");

    //printAST(&parser);

    BytecodeStream stream;
    initBytecodeStream(&stream);

    Compiler compiler;
    initCompiler(&compiler, &stream);

    compile(&compiler, &parser.ast);

    printf("COMPILED\n");

    printCompileResult(&compiler);

    VM vm;
    initVM(&vm, 1024, 1024, 256, compiler.bStream);

    printf("_______________________________________________\n");
    printf("RUN RESULT\n");
    printf("_______________________________________________\n\n");

    run(&vm);

    /*for (int i = 0; i < compiler.bStream->count; i++) {
        printf("%x\n", compiler.bStream->stream[i]);
    }*/

    /*if (res) {
        printAST(&parser);
    } else {
        printf("Parser failed.");
    }*/
    //printAST(&parser);

    freeParser(&parser);
    freeLexer(&lexer);
    freeAnalyser(&analyser);
    freeCompiler(&compiler);
    freeBytecodeStream(&stream);
    freeVM(&vm);
}

int main(int argc, char* argv[]) {
    if (argc == 2) {
        runFile(argv[1]);
    } else {
        fprintf(stderr, "Usage pseudo [path]\n");
    }
}
