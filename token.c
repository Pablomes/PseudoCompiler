
#include "token.h"

TokenArray* initTokenArray() {
    TokenArray* array = (TokenArray*) malloc(sizeof(TokenArray));

    if (array == NULL) return NULL;

    array->count = 0;
    array->start = (Token*) malloc(sizeof(Token) * 8);

    if (array->start == NULL) { free(array); return NULL; }

    array->size = 8;

    return array;
}

bool addToken(TokenArray* array, Token token) {
    if (array->count + 1 > array->size) {
        int newSize = array->size * 2;
        Token* newArr = realloc(array->start, sizeof(Token) * newSize);
        if (newArr == NULL) return false;

        array->size = newSize;
        array->start = newArr;
    }

    array->start[array->count++] = token;

    return true;
}

const char* tokenTypeNames[] = {
        "TOK_AND", "TOK_APPEND", "TOK_ARRAY", "TOK_BOOLEAN", "TOK_BYREF", "TOK_CALL", "TOK_CASE", "TOK_CHAR", "TOK_CLASS",
        "TOK_CLOSEFILE", "TOK_CONSTANT", "TOK_DATE", "TOK_DECLARE", "TOK_DIV", "TOK_DO", "TOK_ELSE", "TOK_ENDCASE", "TOK_ENDFUNCTION",
        "TOK_ENDIF", "TOK_ENDPROCEDURE", "TOK_ENDWHILE", "TOK_FALSE", "TOK_FOR", "TOK_FUNCTION", "TOK_IF", "TOK_INHERITS", "TOK_INPUT",
        "TOK_INTEGER", "TOK_MOD", "TOK_NEXT", "TOK_NEW", "TOK_NOT", "TOK_OF", "TOK_OPENFILE", "TOK_OR", "TOK_OTHERWISE", "TOK_OUTPUT",
        "TOK_PRIVATE", "TOK_PROCEDURE", "TOK_PUBLIC", "TOK_READ", "TOK_READFILE", "TOK_REAL", "TOK_REPEAT", "TOK_RETURN", "TOK_RETURNS",
        "TOK_SET", "TOK_STEP", "TOK_STRING", "TOK_THEN", "TOK_TO", "TOK_TRUE", "TOK_TYPE", "TOK_UNTIL", "TOK_WHILE", "TOK_WRITE", "TOK_WRITEFILE",
        "TOK_IDENTIFIER", "TOK_INT_LIT", "TOK_REAL_LIT", "TOK_CHAR_LIT", "TOK_STRING_LIT",
        "TOK_LEFT_PAREN", "TOK_RIGHT_PAREN", "TOK_LEFT_BRACKET", "TOK_RIGHT_BRACKET", "TOK_DOT", "TOK_COMMA", "TOK_PLUS",
        "TOK_MINUS", "TOK_STAR", "TOK_SLASH", "TOK_CARAT", "TOK_LESS", "TOK_GREATER", "TOK_EQUAL", "TOK_NEW_LINE",
        "TOK_COLON", "TOK_AMPERSAND",
        "TOK_NOT_EQUAL", "TOK_LESS_EQUAL", "TOK_GREATER_EQUAL", "TOK_ASSIGN",
        "TOK_ERROR", "TOK_EOF"
};

static void printToken(Token* token) {
    printf("[\"%.*s\", TYPE = %s, line %d]", token->length, token->start, tokenTypeNames[token->type], token->line);
}

void printTokenArray(TokenArray* array) {
    for (int i = 0; i < array->count; i++) {
        printToken(&array->start[i]);
    }
}

void freeTokenArray(TokenArray* array) {
    free(array->start);
    free(array);
}