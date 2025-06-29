#ifndef PSEUDOCOMPILER_SEMANTIC_H
#define PSEUDOCOMPILER_SEMANTIC_H

#include "parser.h"
#include "symbol.h"

typedef struct {
    SymbolTable* globalTable;
    SymbolTable* symbolTable;
    int depth;
    ASTNode* currentFunction;
    bool hasReturned;
    bool assigning;
    bool hasDefault;
    bool hadError;
    bool caseReturns;
} Analyser;


void initAnalyser(Analyser* analyser);
void freeAnalyser(Analyser* analyser);
bool semanticAnalysis(Analyser* analyser, AST* ast);

#endif //PSEUDOCOMPILER_SEMANTIC_H