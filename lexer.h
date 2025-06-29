#ifndef PSEUDOCOMPILER_LEXER_H
#define PSEUDOCOMPILER_LEXER_H

#include "token.h"

typedef struct Node {
    struct Node* trans[26];
    bool isFinal;
    TokenType keyword;
} Node;

typedef struct {
    const char* start;
    const char* current;
    TokenArray* array;
    Node* keywordTrie;
    int line;
    int col;
} Lexer;

void initLexer(Lexer* lexer, const char* source);
void freeLexer(Lexer* lexer);
bool scanSource(Lexer* lexer);
void printTokens(Lexer* lexer);

#endif //PSEUDOCOMPILER_LEXER_H
