#ifndef PSEUDOCOMPILER_PARSER_H
#define PSEUDOCOMPILER_PARSER_H

#include "common.h"
#include "token.h"

typedef enum {
    // EXPRESSIONS
    EXPR_BINARY, EXPR_ASSIGN, EXPR_CALL,
    EXPR_GET, EXPR_GROUP, EXPR_LITERAL,
    EXPR_LOGICAL, EXPR_SET, EXPR_SUPER,
    EXPR_THIS, EXPR_UNARY, EXPR_VARIABLE,
    EXPR_ARRAY_ACCESS,

    // STATEMENTS
    STMT_BLOCK, STMT_EXPR, STMT_SUBROUTINE,
    STMT_IF, STMT_OUTPUT, STMT_INPUT, STMT_RETURN,
    STMT_WHILE, STMT_VAR_DECLARE, STMT_CONST_DECLARE,
    STMT_ARRAY_DECLARE, STMT_CASE, STMT_REPEAT, STMT_CALL,
    STMT_CASE_BLOCK, STMT_PROGRAM, STMT_FOR, STMT_CASE_LINE,

    // MISCELLANEOUS
    AST_PARAMETER

} ASTNodeType;

typedef enum {
    TYPE_INTEGER, TYPE_REAL, TYPE_BOOLEAN,
    TYPE_CHAR, TYPE_STRING, TYPE_ARRAY, TYPE_SUBROUTINE, TYPE_NONE, TYPE_ERROR
} DataType;

typedef enum {
    TYPE_FUNCTION, TYPE_PROCEDURE
} SubroutineType;

typedef enum {
    OP_NONE,

    // BINARY OPS
    BIN_ADD, BIN_MINUS, BIN_MULT, BIN_DIV, BIN_MOD,
    BIN_FDIV, BIN_CONCAT, BIN_POWER,

    // LOGICAL OPS
    LOGIC_EQUAL, LOGIC_NOT_EQUAL, LOGIC_AND, LOGIC_OR,
    LOGIC_LESS, LOGIC_LESS_EQUAL, LOGIC_GREATER, LOGIC_GREATER_EQUAL,

    //UNARY OPS
    UNARY_NEG, UNARY_NOT, UNARY_PLUS
} Operation;

typedef struct ASTNode ASTNode;

typedef struct {
    ASTNode** start;
    int size;
    int count;
} ASTNodeArray;

struct ASTNode{
    ASTNodeType type;
    const char* start;
    long length;
    int line;
    int col;

    union {
        struct {
            DataType resultType;
        } Expr;

        struct {
            DataType resultType;
            Token* value;
        } LiteralExpr;

        struct {
            DataType resultType;
            Token* name;
            ASTNodeArray arguments;
        } CallExpr;

        struct {
            DataType resultType;
            struct ASTNode* subExpr;
        } GroupExpr;

        struct {
            DataType resultType;
            Token* name;
            bool assigned;
        } VariableExpr;

        struct {
            DataType resultType;
            Token* name;
            struct ASTNode* indices[2];
            bool assigned;
        } ArrayAccessExpr;

        struct {
            DataType resultType;
            Operation op;
            struct ASTNode* right;
        } UnaryExpr;

        struct {
            DataType resultType;
            Operation op;
            struct ASTNode* left;
            struct ASTNode* right;
            DataType leftType;
            DataType rightType;
        } BinaryExpr;

        struct {
            DataType resultType;
            struct ASTNode* left;
            struct ASTNode* right;
        } AssignmentExpr;

        struct {
            ASTNodeArray body;
        } BlockStmt;

        struct {
            struct ASTNode* expr;
            DataType resultType;
        } ExprStmt;

        struct {
            Token* name;
            ASTNodeArray parameters;
            SubroutineType subroutineType;
            DataType returnValue;
            struct ASTNode* body;
        } SubroutineStmt;

        struct {
            struct ASTNode* condition;
            struct ASTNode* thenBranch;
            struct ASTNode* elseBranch;
        } IfStmt;

        struct {
            ASTNodeArray expressions;
        } OutputStmt;

        struct {
            Token* name; // REMOVE WHEN DONE
            struct ASTNode* varAccess;
            DataType expectedType;
        } InputStmt;

        struct {
            struct ASTNode* expr;
            DataType returnType;
        } ReturnStmt;

        struct {
            struct ASTNode* condition;
            struct ASTNode* body;
        } WhileStmt;

        struct {
            Token* name;
            DataType type;
        } VarDeclareStmt;

        struct {
            Token* name;
            Token* value;
            DataType type;
        } ConstDeclareStmt;

        struct {
            Token* name;
            DataType type;
            ASTNode* dimensions[4];
            bool is2D;
        } ArrayDeclareStmt;

        struct {
            struct ASTNode* expr;
            struct ASTNode* body;
            DataType exprType;
        } CaseStmt;

        struct {
            struct ASTNode* condition;
            struct ASTNode* body;
        } RepeatStmt;

        struct {
            Token* counterName;
            struct ASTNode* init;
            struct ASTNode* end;
            struct ASTNode* step;
            struct ASTNode* body;
        } ForStmt;

        struct {
            Token* name;
            ASTNodeArray arguments;
        } CallStmt;

        struct {
            struct ASTNode* value;
            struct ASTNode* result;
        } CaseLineStmt;

        struct {
            struct ASTNode* body;
        } CaseBlockStmt;

        struct {
            ASTNodeArray body;
        } ProgramStmt;

        struct {
            Token* name;
            DataType type;
            bool byref;
            bool isArray;
            bool is2D;
        } Parameter;

    } as;

};

typedef struct {
    ASTNode* program;
} AST;

typedef struct {
    Token* current;
    bool hadError;
    bool panicMode;
    AST ast;
} Parser;

void initParser(Parser* parser, TokenArray* array);
void freeParser(Parser* parser);
bool genAST(Parser* parser);
void printAST(Parser* parser);

#endif //PSEUDOCOMPILER_PARSER_H