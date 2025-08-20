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

const char* getExecutableName(const char* fullPath) {
    // Find last slash in the path (Windows uses \, Unix uses /)
    const char* lastSlash = strrchr(fullPath, '\\');
    const char* lastForwardSlash = strrchr(fullPath, '/');

    // Choose the one that appears later
    const char* base = fullPath;
    if (lastSlash && lastForwardSlash)
        base = (lastSlash > lastForwardSlash) ? lastSlash + 1 : lastForwardSlash + 1;
    else if (lastSlash)
        base = lastSlash + 1;
    else if (lastForwardSlash)
        base = lastForwardSlash + 1;

    // Check if it ends in ".exe" (case-insensitive)
    size_t len = strlen(base);
    if (len > 4 && _stricmp(base + len - 4, ".exe") == 0) {
        static char buffer[256];  // buffer to store result
        strncpy(buffer, base, len - 4); // copy without ".exe"
        buffer[len - 4] = '\0';         // null-terminate
        return buffer;
    }

    return base;  // return base name as-is
}

static bool hasExtension(const char* filename, const char* extension) {
    const char* dot = strrchr(filename, '.');
    if (!dot || dot == filename) return false;
    return strcmp(dot, extension) == 0;
}

static void runFile(const char* path, bool debug) {
    char* source = readFile(path);

    Lexer lexer;
    initLexer(&lexer, source);

    scanSource(&lexer);

    if (debug) printTokens(&lexer);

    Parser parser;
    initParser(&parser, lexer.array);

    bool res = genAST(&parser);

    if (res) {
        freeLexer(&lexer);
        freeParser(&parser);
        return;
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

    if (debug)  printf("ANALYSED CORRECTLY\n");

    if (debug) printAST(&parser);

    BytecodeStream stream;
    initBytecodeStream(&stream);

    Compiler compiler;
    initCompiler(&compiler, &stream);

    compile(&compiler, &parser.ast);

    if (debug) printf("COMPILED\n");

    if (debug) printCompileResult(&compiler);


    VM vm;
    initVM(&vm, 1024, 1024, 256, compiler.bStream);

    if (debug) printf("_______________________________________________\n");
    if (debug) printf("RUN RESULT\n");
    if (debug) printf("_______________________________________________\n\n");

    run(&vm, debug);

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

static void compileFile(const char* path, const char* target, bool debug) {
    char* source = readFile(path);

    Lexer lexer;
    initLexer(&lexer, source);

    scanSource(&lexer);

    if (debug) printTokens(&lexer);

    Parser parser;
    initParser(&parser, lexer.array);

    bool res = genAST(&parser);

    if (res) {
        freeLexer(&lexer);
        freeParser(&parser);
        return;
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

    if (debug)  printf("ANALYSED CORRECTLY\n");

    if (debug) printAST(&parser);

    BytecodeStream stream;
    initBytecodeStream(&stream);

    Compiler compiler;
    initCompiler(&compiler, &stream);

    compile(&compiler, &parser.ast);

    bool genRes = genBinFile(compiler.bStream, target);

    if (!genRes) {
        fprintf(stderr, "Something went wrong generating the binary file.\n");
    }

    if (debug) printf("COMPILED\n");

    if (debug) printCompileResult(&compiler);

    freeParser(&parser);
    freeLexer(&lexer);
    freeAnalyser(&analyser);
    freeCompiler(&compiler);
    freeBytecodeStream(&stream);
}

static void runBytecode(const char* path, bool debug) {
    bool addExtension = !hasExtension(path, ".pcbc");

    BytecodeStream stream;
    initBytecodeStream(&stream);

    bool res = readBinFile(&stream, path, addExtension);


    VM vm;
    initVM(&vm, 1024, 1024, 256, &stream);

    if (debug) printf("_______________________________________________\n");
    if (debug) printf("RUN RESULT\n");
    if (debug) printf("_______________________________________________\n\n");

    run(&vm, debug);

    freeBytecodeStream(&stream);
    freeVM(&vm);
}

static void printHelp() {
    printf("\nCambridge Psuedocode Compiler and Virtual Machine\n"
           "By Pablo Mestre Alonso              2025\n"
           "\n"
           "Commands:\n"
           "-h -> Show help menu\n"
           "-cr <file path> -> Compiles and runs pseudocode source.\n"
           "-c <file path> <target name> -> Compiles pseudocode source and saves bytecode result as .pcbc.\n"
           "-r <file path> -> Runs pseudocode bytecode (.pcbc file).\n\n");
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        printHelp();
        return 0;
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            printHelp();
            return 0;
        }
    }

    if (argc >= 2) {
        if (strcmp(argv[1], "-cr") == 0) {
            const char* path = argv[2];

            if (argc == 4 && strcmp(argv[3], "true") == 0) {
                runFile(path, true);
                return 0;
            }
            if (argc != 3) {
                fprintf(stderr, "Usage: pseudo -r <file path>\n");
                return 1;
            }
            runFile(path, false);
        } else if (strcmp(argv[1], "-c") == 0) {
            const char* path = argv[2];

            if (argc == 5 && strcmp(argv[4], "true") == 0) {
                compileFile(path, argv[3], true);
                return 0;
            }
            if (argc != 4) {
                fprintf(stderr, "Usage: pseudo -c <file path> <target name>\n");
                return 1;
            }

            compileFile(path, argv[3], false);
        } else if (strcmp(argv[1], "-r") == 0) {
            const char* path = argv[2];

            if (argc == 4 && strcmp(argv[3], "true") == 0) {
                runBytecode(path, true);
                return 0;
            }
            if (argc != 3) {
                fprintf(stderr, "Usage: pseudo -r <file path>\n");
                return 1;
            }
            runBytecode(path, false);
        } else {
            fprintf(stderr, "Unknown command.\n");
            printHelp();
        }
    }

    return 0;
}
