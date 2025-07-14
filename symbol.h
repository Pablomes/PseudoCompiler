#ifndef PSEUDOCOMPILER_SYMBOL_H
#define PSEUDOCOMPILER_SYMBOL_H

#include "parser.h"

typedef enum {
    SYMBOL_VAR, SYMBOL_PARAM, SYMBOL_CONST, SYMBOL_ARRAY, SYMBOL_FUNC, SYMBOL_PROC,
    SYMBOL_FOR_COUNTER, SYMBOL_FILE, SYMBOL_BUILTIN_FUNC, SYMBOL_NONE
} SymbolType;

typedef enum {
    SCOPE_GLOBAL, SCOPE_LOOP, SCOPE_FUNCTION,
    SCOPE_PROCEDURE, SCOPE_CONDITIONAL
} ScopeType;

typedef struct {
    SymbolType type;
    ASTNode* node;
    bool initialised; // CHECK SYMBOL HAS VALUE GIVEN TO IT BEFORE USE
    int pos;
    bool isRelative;
    bool byref;
    FileAccessType access;
} Symbol;

typedef struct {
    DataType* parameterTypes;
    int numParams;
    DataType returnType;
    int builtinIdx;
} Builtin;

typedef struct {
    char* key;
    Symbol* symbol;
} Entry;

typedef struct SymbolTable{
    int count;
    int capacity;
    Entry* entries;
    struct SymbolTable* enclosing;
    ScopeType scopeType;
    int nextPos;
    int filesOpened;
} SymbolTable;

void initTable(SymbolTable* table);
void freeTable(SymbolTable* table);
bool getCurrTable(SymbolTable* table, const char* key, Symbol* symbol);
bool getTable(SymbolTable* table, const char* key, Symbol* symbol);
Symbol* getTablePointer(SymbolTable* table, const char* key);
bool setTable(SymbolTable* table, const char* key, ASTNode* node, SymbolType type, int pos, bool isRelative, bool byref);
bool setTableFile(SymbolTable* table, const char* key, ASTNode* node, SymbolType type, int pos, bool isRelative, bool byref, FileAccessType access);
bool deleteTable(SymbolTable* table, const char* key);
void copyOverTable(SymbolTable* from, SymbolTable* to);
void clearTable(SymbolTable* table);

void createBuiltin(Builtin* func, int numParams, DataType returnType, int idx);
void addParamDatatype(Builtin* func, DataType type, int idx);

#endif //PSEUDOCOMPILER_SYMBOL_H
