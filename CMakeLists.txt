cmake_minimum_required(VERSION 3.29)
project(PseudoCompiler C)

set(CMAKE_C_STANDARD 11)

add_executable(PseudoCompiler main.c
        common.h
        memory.h
        memory.c
        lexer.h
        lexer.c
        token.h
        token.c
        parser.h
        parser.c
        semantic.h
        semantic.c
        symbol.h
        symbol.c
        common.c
        object.h
        object.c
        stack.h
        stack.c
        bytecode.h
        bytecode.c
        compiler.h
        compiler.c
        vm.h
        vm.c
)
