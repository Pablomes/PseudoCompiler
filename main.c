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

    //printf("ANALYSED CORRECTLY\n");

    //printAST(&parser);

    BytecodeStream stream;
    initBytecodeStream(&stream);

    Compiler compiler;
    initCompiler(&compiler, &stream);

    compile(&compiler, &parser.ast);

    //printf("COMPILED\n");

    //printCompileResult(&compiler);


    VM vm;
    initVM(&vm, 1024, 1024, 256, compiler.bStream);

    /*printf("_______________________________________________\n");
    printf("RUN RESULT\n");
    printf("_______________________________________________\n\n");*/

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

static void compileFile(const char* path, const char* target) {
    char* source = readFile(path);

    Lexer lexer;
    initLexer(&lexer, source);

    scanSource(&lexer);

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

    BytecodeStream stream;
    initBytecodeStream(&stream);

    Compiler compiler;
    initCompiler(&compiler, &stream);

    compile(&compiler, &parser.ast);

    bool genRes = genBinFile(compiler.bStream, target);

    if (!genRes) {
        fprintf(stderr, "Something went wrong generating the binary file.\n");
    }

    freeParser(&parser);
    freeLexer(&lexer);
    freeAnalyser(&analyser);
    freeCompiler(&compiler);
    freeBytecodeStream(&stream);
}

static void runBytecode(const char* path) {
    bool addExtension = !hasExtension(path, ".pcbc");

    BytecodeStream stream;
    initBytecodeStream(&stream);

    bool res = readBinFile(&stream, path, addExtension);


    VM vm;
    initVM(&vm, 1024, 1024, 256, &stream);

    run(&vm);

    freeBytecodeStream(&stream);
    freeVM(&vm);
}

int main(int argc, char* argv[]) {
    if (argc >= 2) {
        /*const char* invoked_as = strrchr(argv[0], '/');
        invoked_as = invoked_as ? invoked_as + 1 : argv[0];*/

        const char* invoked_as = getExecutableName(argv[0]);

        const char* path = argv[1];

        if (strcmp(invoked_as, "pseudor") == 0) {
            if (argc != 2) {
                fprintf(stderr, "Usage: pseudor <file path>\n");
                return 1;
            }
            runFile(path);
        } else if (strcmp(invoked_as, "pseudoc") == 0) {
            if (argc != 3) {
                fprintf(stderr, "Usage: pseudoc <file path> <target name>\n");
                return 1;
            }

            compileFile(path, argv[2]);
        } else if (strcmp(invoked_as, "pseudo") == 0) {
            if (argc != 2) {
                fprintf(stderr, "Usage: pesudo <file path>\n");
                return 1;
            }
            runBytecode(path);
        } else {
            fprintf(stderr, "Unknown command: %s\nAvailable commands are:\n- pseudor <file path> : Compiles and runs the program, discarding its bytecode.\n- pseudoc <file path> <target name> : Compiles the program source into .pcbc bytecode.\n- pseudo <file path> : Executes a .pcbc bytecode file.", invoked_as);
            return 1;
        }
    } else {
        fprintf(stderr, "Usage: %s <file path> <target name>?\n", argv[0]);
        return 1;
    }

    return 0;
}
