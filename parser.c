
#include "parser.h"

//#define ALLOC_AST(parser, type)     (type*)allocArena(&parser->memArena, sizeof(type));

static void* allocMem(size_t allocSize) {
    return malloc(allocSize);
}

static void* reallocMem(void* ptr, size_t oldSize, size_t newSize) {
    if (newSize == 0) {
        free(ptr);
        return NULL;
    } else {
        return realloc(ptr, newSize);
    }
}

#define ALLOC_AST                               (ASTNode*)allocMem(sizeof(ASTNode))
#define FREE_AST(ptr)                           reallocMem((void*)(ptr), sizeof(ASTNode), 0)
#define ALLOC_AST_ARRAY(size)                   (ASTNode**)allocMem(sizeof(ASTNode*) * size)
#define GROW_AST_ARRAY(ptr, oldSize, newSize)   (ASTNode**)reallocMem((void**)(ptr), sizeof(ASTNode*) * oldSize, sizeof(ASTNode*) * newSize)
#define FREE_AST_ARRAY(ptr)                     reallocMem((void*)(ptr), 1, 0)

static bool addASTNode(ASTNodeArray* array, ASTNode* node) {
    if (array->start == NULL || array->size == 0) {
        array->start = ALLOC_AST_ARRAY(4);
        if (array->start == NULL) return false;

        array->size = 4;
    }

    if (array->count >= array->size) {
        array->start = GROW_AST_ARRAY(array->start, array->size, array->size * 2);
        if (array->start == NULL) return false;

        array->size *= 2;
    }

    array->start[array->count++] = node;
    return true;
}

static void freeASTNode(ASTNode* node);

static void freeASTArray(ASTNodeArray* array) {
    for (int i = 0; i < array->count; i++) {
        freeASTNode(array->start[i]);
    }

    FREE_AST_ARRAY(array->start);
}

void initParser(Parser* parser, TokenArray* array) {
    parser->hadError = false;
    parser->panicMode = false;
    parser->ast.program = NULL;
    parser->produceErrors = true;

    parser->current = array->start;
}

static void freeASTNode(ASTNode* node) {
    if (node == NULL) return;

    switch (node->type) {
        case EXPR_CALL:
            freeASTArray(&node->as.CallExpr.arguments);
            break;
        case EXPR_GROUP:
            freeASTNode(node->as.GroupExpr.subExpr);
            break;
        case EXPR_ARRAY_ACCESS:
            freeASTNode(node->as.ArrayAccessExpr.indices[0]);
            freeASTNode(node->as.ArrayAccessExpr.indices[1]);
            break;
        case EXPR_UNARY:
            freeASTNode(node->as.UnaryExpr.right);
            break;
        case EXPR_BINARY:
            freeASTNode(node->as.BinaryExpr.left);
            freeASTNode(node->as.BinaryExpr.right);
            break;
        case EXPR_ASSIGN:
            freeASTNode(node->as.AssignmentExpr.left);
            freeASTNode(node->as.AssignmentExpr.right);
            break;
        case STMT_BLOCK:
            freeASTArray(&node->as.BlockStmt.body);
            break;
        case STMT_EXPR:
            freeASTNode(node->as.ExprStmt.expr);
            break;
        case STMT_SUBROUTINE:
            freeASTArray(&node->as.SubroutineStmt.parameters);
            freeASTNode(node->as.SubroutineStmt.body);
            break;
        case STMT_IF:
            freeASTNode(node->as.IfStmt.condition);
            freeASTNode(node->as.IfStmt.thenBranch);
            freeASTNode(node->as.IfStmt.elseBranch);
            break;
        case STMT_INPUT:
            freeASTNode(node->as.InputStmt.varAccess);
            break;
        case STMT_OUTPUT:
            freeASTArray(&node->as.OutputStmt.expressions);
            break;
        case STMT_RETURN:
            freeASTNode(node->as.ReturnStmt.expr);
            break;
        case STMT_WHILE:
            freeASTNode(node->as.WhileStmt.condition);
            freeASTNode(node->as.WhileStmt.body);
            break;
        case STMT_ARRAY_DECLARE:
            for (int i = 0; i < 4; i++) {
                freeASTNode(node->as.ArrayDeclareStmt.dimensions[i]);
            }
            break;
        case STMT_CASE:
            freeASTNode(node->as.CaseStmt.expr);
            freeASTNode(node->as.CaseStmt.body);
            break;
        case STMT_REPEAT:
            freeASTNode(node->as.RepeatStmt.condition);
            freeASTNode(node->as.RepeatStmt.body);
            break;
        case STMT_FOR:
            freeASTNode(node->as.ForStmt.init);
            freeASTNode(node->as.ForStmt.end);
            freeASTNode(node->as.ForStmt.step);
            freeASTNode(node->as.ForStmt.body);
            break;
        case STMT_CALL:
            freeASTArray(&node->as.CallStmt.arguments);
            break;
        case STMT_CASE_LINE:
            freeASTNode(node->as.CaseLineStmt.value);
            freeASTNode(node->as.CaseLineStmt.result);
            break;
        case STMT_CASE_BLOCK:
            freeASTNode(node->as.CaseBlockStmt.body);
            break;
        case STMT_READFILE:
            freeASTNode(node->as.ReadfileStmt.varAccess);
            break;
        case STMT_WRITEFILE:
            freeASTArray(&node->as.WritefileStmt.expressions);
            break;
        case STMT_PROGRAM:
            freeASTArray(&node->as.ProgramStmt.body);
            break;
        default:
            break;
    }

    FREE_AST(node);
}

static void printTreeLinePrefix(int depth) {
    for (int i = 0; i < depth; i++) {
        printf("|   ");
    }
}

static void printOp(ASTNode* node) {
    Operation op = OP_NONE;

    switch (node->type) {
        case EXPR_BINARY:
            op = node->as.BinaryExpr.op;
            break;
        case EXPR_UNARY:
            op = node->as.UnaryExpr.op;
            break;
        default:
            break;
    }

    switch (op) {
        case OP_NONE:
            printf("NONE");
            break;
        case BIN_ADD:
            printf("ADD");
            break;
        case BIN_MINUS:
            printf("MINUS");
            break;
        case BIN_MULT:
            printf("MULTIPLY");
            break;
        case BIN_DIV:
            printf("DIVIDE");
            break;
        case BIN_MOD:
            printf("MODULO");
            break;
        case BIN_FDIV:
            printf("FLOOR DIVIDE");
            break;
        case BIN_CONCAT:
            printf("CONCATENATE");
            break;
        case BIN_POWER:
            printf("POWER");
            break;
        case LOGIC_EQUAL:
            printf("EQUAL");
            break;
        case LOGIC_NOT_EQUAL:
            printf("NOT EQUAL");
            break;
        case LOGIC_AND:
            printf("AND");
            break;
        case LOGIC_OR:
            printf("OR");
            break;
        case LOGIC_LESS:
            printf("LESS THAN");
            break;
        case LOGIC_LESS_EQUAL:
            printf("LESS THAN OR EQUAL");
            break;
        case LOGIC_GREATER:
            printf("GREATER THAN");
            break;
        case LOGIC_GREATER_EQUAL:
            printf("GREATER THAN OR EQUAL");
            break;
        case UNARY_NEG:
            printf("NEGATE");
            break;
        case UNARY_NOT:
            printf("NOT");
            break;
        case UNARY_PLUS:
            printf("UNARY PLUS");
            break;
        default:
            printf("UNKNOWN OPERATION");
            break;
    }
}

static void printReturnType(DataType type) {
    switch (type) {
        case TYPE_INTEGER:
            printf("INTEGER");
            break;
            case TYPE_REAL:
                printf("REAL");
                break;
        case TYPE_BOOLEAN:
            printf("BOOLEAN");
            break;case TYPE_CHAR:
                printf("CHAR");
                break;
        case TYPE_STRING:
            printf("STRING");
            break;
        case TYPE_ARRAY:
            printf("ARRAY");
            break;
        case TYPE_FILE:
            printf("FILE");
            break;
        case TYPE_NONE:
            printf("NONE");
            break;
        default:
            printf("UNKNOWN");
            break;
    }
}

static void printAccessType(FileAccessType type) {
    switch (type) {
        case ACCESS_READ:
            printf("READ");
            break;
        case ACCESS_WRITE:
            printf("WRITE");
            break;
        case ACCESS_APPEND:
            printf("APPEND");
            break;
        default:
            printf("-");
            break;
    }
}

static void printExprReturnType(ASTNode* node) {
    printReturnType(node->as.Expr.resultType);
}

static void printASTNode(ASTNode* node, int depth) {
#define PREFIX printTreeLinePrefix(depth)

    if (node == NULL) {
        PREFIX; printf("NULL.\n");
        return;
    }

    switch (node->type) {
        case EXPR_CALL:
            PREFIX;
            printf("CALL EXPR.\n");
            PREFIX;
            printf("name: %.*s\n", node->as.CallExpr.name->length, node->as.CallExpr.name->start);
            PREFIX;
            printf("arguments:\n");
            for (int i = 0; i < node->as.CallExpr.arguments.count; i++) {
                printASTNode(node->as.CallExpr.arguments.start[i], depth + 1);
                PREFIX;
                printf("\n");
            }
            PREFIX; printf("\n");
            break;
        case EXPR_GROUP:
            PREFIX;
            printf("GROUP EXPR.\n");
            printASTNode(node->as.GroupExpr.subExpr, depth + 1);
            PREFIX; printf("\n");
            break;
        case EXPR_VARIABLE:
            PREFIX; printf("VARIABLE EXPR.\n");
            PREFIX; printf("name: %.*s\n", node->as.VariableExpr.name->length, node->as.VariableExpr.name->start);
            PREFIX; printf("datatype: "); printExprReturnType(node); printf("\n");
            PREFIX; printf("assigned: "); printf(node->as.VariableExpr.assigned ? "yes" : "no"); printf("\n");
            PREFIX; printf("\n");
            break;
        case EXPR_ARRAY_ACCESS:
            PREFIX;
            printf("ARRAY ACCESS EXPR.\n");
            PREFIX;
            printf("name: %.*s\n", node->as.ArrayAccessExpr.name->length, node->as.ArrayAccessExpr.name->start);
            PREFIX;
            printf("indices:\n");
            printASTNode(node->as.ArrayAccessExpr.indices[0], depth + 1);
            PREFIX; printf("\n");
            printASTNode(node->as.ArrayAccessExpr.indices[1], depth + 1);
            PREFIX; printf("\n");
            break;
        case EXPR_UNARY:
            PREFIX;
            printf("UNARY EXPR.\n");
            PREFIX;
            printf("operation: ");
            printOp(node);
            printf("\n");
            PREFIX;
            printf("operand:\n");
            printASTNode(node->as.UnaryExpr.right, depth + 1);
            PREFIX; printf("\n");
            break;
        case EXPR_BINARY:
            PREFIX;
            printf("BINARY EXPR.\n");
            PREFIX;
            printf("operation: ");
            printOp(node);
            printf("\n");
            PREFIX;
            printf("left operand:\n");
            printASTNode(node->as.BinaryExpr.left, depth + 1);
            PREFIX;
            printf("right operand:\n");
            printASTNode(node->as.BinaryExpr.right, depth + 1);
            PREFIX; printf("\n");
            break;
        case EXPR_ASSIGN:
            PREFIX;
            printf("ASSIGNMENT EXPR.\n");
            PREFIX; printf("left:\n");
            printASTNode(node->as.AssignmentExpr.left, depth + 1);
            PREFIX; printf("\n"); PREFIX;
            printf("right:\n");
            printASTNode(node->as.AssignmentExpr.right, depth + 1);
            PREFIX; printf("\n");
            break;
        case EXPR_LITERAL:
            PREFIX; printf("LITERAL EXPR.\n");
            PREFIX; printf("value: %.*s\n", node->as.LiteralExpr.value->length, node->as.LiteralExpr.value->start);
            PREFIX; printf("type: "); printExprReturnType(node); printf("\n");
            PREFIX; printf("\n");
            break;
        case STMT_BLOCK:
            PREFIX; printf("BLOCK STMT.\n"); PREFIX;
            printf("statements:\n");
            for (int i = 0; i < node->as.BlockStmt.body.count; i++) {
                printASTNode(node->as.BlockStmt.body.start[i], depth + 1);
                PREFIX;
                printf("\n");
            }
            PREFIX; printf("\n");
            break;
        case STMT_EXPR:
            PREFIX; printf("EXPRESSION STMT.\n"); PREFIX;
            printf("expression:\n");
            printASTNode(node->as.ExprStmt.expr, depth + 1);
            PREFIX; printf("\n");
            break;
        case STMT_SUBROUTINE:
            PREFIX; printf("SUBROUTINE STMT.\n"); PREFIX;
            printf("Subroutine type: ");
            if (node->as.SubroutineStmt.subroutineType == TYPE_FUNCTION) {
                printf("Function\n");
            } else {
                printf("Procedure\n");
            }
            PREFIX; printf("name: %.*s\n", node->as.SubroutineStmt.name->length, node->as.SubroutineStmt.name->start);
            PREFIX; printf("parameters:\n");
            for (int i = 0; i < node->as.SubroutineStmt.parameters.count; i++) {
                printASTNode(node->as.SubroutineStmt.parameters.start[i], depth + 1);
                PREFIX;
                printf("\n");
            }
            PREFIX; printf("body:\n");
            printASTNode(node->as.SubroutineStmt.body, depth + 1);
            PREFIX; printf("\n");
            break;
        case STMT_IF:
            PREFIX; printf("IF STMT.\n");
            PREFIX; printf("condition:\n");
            printASTNode(node->as.IfStmt.condition, depth + 1);
            PREFIX; printf("THEN branch:\n");
            printASTNode(node->as.IfStmt.thenBranch, depth + 1);
            PREFIX; printf("ELSE branch:\n");
            printASTNode(node->as.IfStmt.elseBranch, depth + 1);
            PREFIX; printf("\n");
            break;
        case STMT_OUTPUT:
            PREFIX; printf("OUTPUT STMT.\n");
            PREFIX; printf("expressions:\n");
            for (int i = 0; i < node->as.OutputStmt.expressions.count; i++) {
                printASTNode(node->as.OutputStmt.expressions.start[i], depth + 1);
                PREFIX;
                printf("\n");
            }
            PREFIX; printf("\n");
            break;
        case STMT_INPUT:
            PREFIX; printf("INPUT STMT.\n");
            PREFIX; printf("target name: %.*s\n", node->as.InputStmt.name->length, node->as.InputStmt.name->start);
            PREFIX; printf("expected type: "); printReturnType(node->as.InputStmt.expectedType);
            printf("\n");
            PREFIX; printf("\n");
            break;
        case STMT_RETURN:
            PREFIX; printf("RETURN STMT.\n");
            PREFIX; printf("expression:\n");
            printASTNode(node->as.ReturnStmt.expr, depth + 1);
            PREFIX; printf("\n");
            break;
        case STMT_WHILE:
            PREFIX; printf("WHILE STMT.\n");
            PREFIX; printf("condition:\n");
            printASTNode(node->as.WhileStmt.condition, depth + 1);
            PREFIX; printf("body:\n");
            printASTNode(node->as.WhileStmt.body, depth + 1);
            PREFIX; printf("\n");
            break;
        case STMT_VAR_DECLARE:
            PREFIX; printf("VAR DECLARE STMT.\n");
            PREFIX; printf("name: %.*s\n", node->as.VarDeclareStmt.name->length, node->as.VarDeclareStmt.name->start);
            PREFIX; printf("type: "); printReturnType(node->as.VarDeclareStmt.type); printf("\n");
            PREFIX; printf("\n");
            break;
        case STMT_CONST_DECLARE:
            PREFIX; printf("CONSTANT DECLARE STMT.\n");
            PREFIX; printf("name: %.*s\n", node->as.ConstDeclareStmt.name->length, node->as.ConstDeclareStmt.name->start);
            PREFIX; printf("value: %.*s\n", node->as.ConstDeclareStmt.value->length, node->as.ConstDeclareStmt.value->start);
            PREFIX; printf("type: "); printReturnType(node->as.ConstDeclareStmt.type); printf("\n");
            PREFIX; printf("\n");
            break;
        case STMT_ARRAY_DECLARE:
            PREFIX; printf("ARRAY DECLARE STMT.\n");
            PREFIX; printf("name: %.*s\n", node->as.ArrayDeclareStmt.name->length, node->as.ArrayDeclareStmt.name->start);
            PREFIX; printf("dimensions:\n");
            for (int i = 0; i < 4; i++) {
                printASTNode(node->as.ArrayDeclareStmt.dimensions[i], depth + 1);
                PREFIX; printf("\n");
            }
            PREFIX; printf("\n");
            break;
        case STMT_CASE:
            PREFIX; printf("CASE STMT.\n");
            PREFIX; printf("expression:\n");
            printASTNode(node->as.CaseStmt.expr, depth + 1);
            PREFIX; printf("body:\n");
            printASTNode(node->as.CaseStmt.body, depth + 1);
            PREFIX; printf("\n");
            break;
        case STMT_REPEAT:
            PREFIX; printf("REPEAT STMT.\n");
            PREFIX; printf("condition:\n");
            printASTNode(node->as.RepeatStmt.condition, depth + 1);
            PREFIX; printf("body:\n");
            printASTNode(node->as.RepeatStmt.body, depth + 1);
            PREFIX; printf("\n");
            break;
        case STMT_FOR:
            PREFIX; printf("FOR STMT.\n");
            PREFIX; printf("counter: %.*s\n", node->as.ForStmt.counterName->length, node->as.ForStmt.counterName->start);
            PREFIX; printf("init:\n");
            printASTNode(node->as.ForStmt.init, depth + 1);
            PREFIX; printf("end:\n");
            printASTNode(node->as.ForStmt.end, depth + 1);
            PREFIX; printf("step:\n");
            printASTNode(node->as.ForStmt.step, depth + 1);
            PREFIX; printf("body:\n");
            printASTNode(node->as.ForStmt.body, depth + 1);
            PREFIX; printf("\n");
            break;
        case STMT_CALL:
            PREFIX; printf("CALL STMT.");
            PREFIX; printf("name: %.*s\n", node->as.CallStmt.name->length, node->as.CallStmt.name->start);
            for (int i = 0; i < node->as.CallStmt.arguments.count; i++) {
                printASTNode(node->as.CallStmt.arguments.start[i], depth + 1);
                PREFIX;
                printf("\n");
            }
            PREFIX; printf("\n");
            break;
        case STMT_CASE_LINE:
            PREFIX; printf("CASE LINE STMT.\n");
            PREFIX; printf("value:\n");
            printASTNode(node->as.CaseLineStmt.value, depth + 1);
            PREFIX; printf("result:\n");
            printASTNode(node->as.CaseLineStmt.result, depth + 1);
            PREFIX; printf("\n");
            break;
        case STMT_CASE_BLOCK:
            PREFIX; printf("CASE BLOCK.\n");
            PREFIX; printf("body:\n");
            printASTNode(node->as.CaseBlockStmt.body, depth + 1);
            PREFIX; printf("\n");
            break;
        case STMT_OPENFILE:
            PREFIX; printf("OPENFILE STMT.\n");
            PREFIX; printf("file name: %.*s\n", node->as.OpenfileStmt.filename->length, node->as.OpenfileStmt.filename->start);
            PREFIX; printf("access type: ");
            printAccessType(node->as.OpenfileStmt.accessType);
            printf("\n");
            PREFIX; printf("\n");
            break;
        case STMT_CLOSEFILE:
            PREFIX; printf("CLOSEFILE STMT.\n");
            PREFIX; printf("file name: %.*s\n", node->as.ClosefileStmt.filename->length, node->as.ClosefileStmt.filename->start);
            PREFIX; printf("\n");
            break;
        case STMT_READFILE:
            PREFIX; printf("READFILE STMT.\n");
            PREFIX; printf("file name: %.*s\n", node->as.ReadfileStmt.filename->length, node->as.ReadfileStmt.filename->start);
            PREFIX; printf("var access:\n");
            printASTNode(node->as.ReadfileStmt.varAccess, depth + 1);
            PREFIX; printf("\n");
            break;
        case STMT_WRITEFILE:
            PREFIX; printf("WRITEFILE STMT.\n");
            PREFIX; printf("file name: %.*s\n", node->as.WritefileStmt.filename->length, node->as.WritefileStmt.filename->start);
            PREFIX; printf("expressions:\n");
            for (int i = 0; i < node->as.WritefileStmt.expressions.count; i++) {
                printASTNode(node->as.WritefileStmt.expressions.start[i], depth + 1);
                PREFIX; printf("\n");
            }
            break;
        case STMT_PROGRAM:
            PREFIX; printf("PROGRAM.\n");
            PREFIX; printf("body:\n");
            for (int i = 0; i < node->as.ProgramStmt.body.count; i++) {
                printASTNode(node->as.ProgramStmt.body.start[i], depth + 1);
                PREFIX;
                printf("\n");
            }
            PREFIX; printf("\n");
            break;
        case AST_PARAMETER:
            PREFIX; printf("PAREMETER.\n");
            PREFIX; printf("name: %.*s\n", node->as.Parameter.name->length, node->as.Parameter.name->start);
            PREFIX; printf("type: "); printReturnType(node->as.Parameter.type); printf("\n");
            PREFIX; printf("BYREF: "); printf(node->as.Parameter.byref ? "TRUE" : "FALSE"); printf("\n");
            PREFIX; printf("IsArray: "); printf(node->as.Parameter.isArray ? "TRUE" : "FALSE"); printf("\n");
            PREFIX; printf("Is2D: "); printf(node->as.Parameter.is2D ? "TRUE" : "FALSE"); printf("\n");
            PREFIX; printf("\n");
            break;
        default:
            PREFIX; printf("UNKNOWN.\n");
            PREFIX; printf("\n");
            break;
    }

    PREFIX; printf("____________________\n");
    PREFIX; printf("%.*s\n", (int)node->length, node->start);
    PREFIX; printf("\n");

#undef PREFIX
}

void freeParser(Parser* parser) {
    freeASTNode(parser->ast.program);
}

static void errorAt(Parser* parser, Token* token, const char* message) {
    if (parser->panicMode) return;
    parser->panicMode = true;
    fprintf(stderr, "[line: %d, col: %d] Error", token->line, token->col);

    if (token->type == TOK_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOK_ERROR) {
        // SKIP
    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    parser->hadError = true;
}

static void error(Parser* parser, const char* message) {
    errorAt(parser, &parser->current[-1], message);
}

static void errorAtCurrent(Parser* parser, const char* message) {
    errorAt(parser, parser->current, message);
}

static void advance(Parser* parser) {
    for (;;) {
        if (parser->current->type == TOK_EOF) break;

        parser->current++;
        if (parser->current->type != TOK_ERROR) break;

        errorAtCurrent(parser, parser->current->start);
    }
}

static bool consume(Parser* parser, TokenType type, const char* message) {
    if (parser->current->type == type) {
        advance(parser);
        return true;
    }

    if (parser->current->type == TOK_EOF && type == TOK_NEW_LINE) {
        return true;
    }

    errorAtCurrent(parser, message);
    return false;
}

static bool check(Parser* parser, TokenType type) {
    return parser->current->type == type;
}

static bool match(Parser* parser, TokenType type) {
    if (!check(parser, type)) return false;

    advance(parser);
    return true;
}

static void synchronise(Parser* parser) {
    parser->panicMode = false;

    while (parser->current->type != TOK_EOF) {
        if (parser->current->type == TOK_NEW_LINE) {
            advance(parser);
            return;
        }

        switch (parser->current->type) {
            case TOK_DECLARE:
            case TOK_FUNCTION:
            case TOK_PROCEDURE:
            case TOK_CALL:
            case TOK_INPUT:
            case TOK_OUTPUT:
            case TOK_IF:
            case TOK_WHILE:
            case TOK_RETURN:
            case TOK_FOR:
            case TOK_REPEAT:
                return;
            default:
                ;
        }

        advance(parser);
    }
}

static ASTNode* expression(Parser* parser);
static ASTNode* assignmentExpression(Parser* parser);

static ASTNode* groupExpression(Parser* parser) {
    ASTNode* group = ALLOC_AST;
    group->type = EXPR_GROUP;
    group->start = parser->current[-1].start;
    group->line = parser->current[-1].line;
    group->col = parser->current[-1].col;

    ASTNode* expr = expression(parser);
    if (expr == NULL) {
        freeASTNode(group);
        return NULL;
    }

    group->as.GroupExpr.subExpr = expr;

    if (!consume(parser, TOK_RIGHT_PAREN, "Expect closing parenthesis ')' after group expression.")) {
        freeASTNode(group);
        return NULL;
    }

    group->length = (parser->current[-1].start + parser->current[-1].length) - group->start;

    return group;
}

static ASTNode* symbolExpression(Parser* parser) {
    ASTNode* symbol = ALLOC_AST;
    Token* name = parser->current - 1;

    if (match(parser, TOK_LEFT_BRACKET)) {
        symbol->type = EXPR_ARRAY_ACCESS;
        symbol->start = parser->current[-2].start;
        symbol->line = parser->current[-2].line;
        symbol->col = parser->current[-2].col;

#define ARRAY symbol->as.ArrayAccessExpr

        ARRAY.name = name;
        ARRAY.indices[0] = NULL;
        ARRAY.indices[1] = NULL;

        ASTNode* index = expression(parser);
        if (index == NULL) {
            freeASTNode(symbol);
            return NULL;
        }

        ARRAY.indices[0] = index;

        if (match(parser, TOK_COMMA)) {
            index = expression(parser);
            if (index == NULL) {
                freeASTNode(symbol);
                return NULL;
            }

            ARRAY.indices[1] = index;
        }

        if (!consume(parser, TOK_RIGHT_BRACKET, "Expected ']' after array indices.")) {
            freeASTNode(symbol);
            return NULL;
        }

        // DETERMINE THE RESULT TYPE VIA THE SYMBOL TABLE (TO BE IMPLEMENTED)

        symbol->length = (parser->current[-1].start + parser->current[-1].length) - symbol->start;

        return symbol;

#undef ARRAY
    } else if (match(parser, TOK_LEFT_PAREN)) {
        symbol->type = EXPR_CALL;
        symbol->start = parser->current[-2].start;
        symbol->line = parser->current[-2].line;
        symbol->col = parser->current[-2].col;

#define CALL symbol->as.CallExpr

        CALL.name = name;
        CALL.arguments.start = NULL;
        CALL.arguments.count = 0;
        CALL.arguments.size = 0;
        if (!check(parser, TOK_RIGHT_PAREN)) {
            do {
                ASTNode *arg = expression(parser);
                if (arg == NULL) {
                    freeASTNode(symbol);
                    return NULL;
                }

                addASTNode(&CALL.arguments, arg);
            } while (match(parser, TOK_COMMA));
        }

        if (!consume(parser, TOK_RIGHT_PAREN, "Expect closing ')' after arguments in call expression.")) {
            freeASTNode(symbol);
            return NULL;
        }

        symbol->length = (parser->current[-1].start + parser->current[-1].length) - symbol->start;

        return symbol;
#undef CALL
    } else {
        symbol->type = EXPR_VARIABLE;
        symbol->start = parser->current[-1].start;
        symbol->line = parser->current[-1].line;
        symbol->col = parser->current[-1].col;
        symbol->as.VariableExpr.name = name;
        symbol->length = (parser->current[-1].start + parser->current[-1].length) - symbol->start;

        return symbol;
    }
}

static ASTNode* primary(Parser* parser) {
    if (match(parser, TOK_LEFT_PAREN)) {
        return groupExpression(parser);
    } else if (match(parser, TOK_IDENTIFIER)) {
        return symbolExpression(parser);
    } else {
        ASTNode* prim = ALLOC_AST;
        prim->type = EXPR_LITERAL;
        prim->start = parser->current->start;
        prim->line = parser->current->line;
        prim->col = parser->current->col;
#define LIT prim->as.LiteralExpr

        switch (parser->current->type) {
            case TOK_INT_LIT:
                LIT.resultType = TYPE_INTEGER;
                break;
            case TOK_REAL_LIT:
                LIT.resultType = TYPE_REAL;
                break;
            case TOK_CHAR_LIT:
                LIT.resultType = TYPE_CHAR;
                break;
            case TOK_STRING_LIT:
                LIT.resultType = TYPE_STRING;
                break;
            case TOK_TRUE:
            case TOK_FALSE:
                LIT.resultType = TYPE_BOOLEAN;
                break;
            default:
                if (parser->produceErrors) {
                    errorAtCurrent(parser, "Expected primary literal or identifier.");
                    freeASTNode(prim);
                }
                return NULL;
        }

        LIT.value = parser->current;
        advance(parser);

        prim->length = (parser->current[-1].start + parser->current[-1].length) - prim->start;

        return prim;

#undef LIT
    }
}

static ASTNode* concatExpression(Parser* parser) {
    ASTNode* expr = primary(parser);
    if (expr == NULL) {
        return NULL;
    }

    while (match(parser, TOK_AMPERSAND)) {
        ASTNode* bin = ALLOC_AST;
        bin->type = EXPR_BINARY;
        bin->start = expr->start;
        bin->line = expr->line;
        bin->col = expr->col;
        bin->as.BinaryExpr.op = BIN_CONCAT;
        ASTNode* right = primary(parser);
        if (right == NULL) {
            freeASTNode(expr);
            freeASTNode(bin);
            return NULL;
        }

        bin->as.BinaryExpr.left = expr;
        bin->as.BinaryExpr.right = right;
        bin->length = (parser->current[-1].start + parser->current[-1].length) - bin->start;
        expr = bin;
    }

    return expr;
}

static ASTNode* unaryExpression(Parser* parser) {

    if (match(parser, TOK_PLUS) || match(parser, TOK_MINUS) || match(parser, TOK_NOT)) {
        ASTNode* unary = ALLOC_AST;
        unary->type = EXPR_UNARY;
        unary->start = parser->current[-1].start;
        unary->line = parser->current[-1].line;
        unary->col = parser->current[-1].col;

        switch (parser->current[-1].type) {
            case TOK_PLUS:
                unary->as.UnaryExpr.op = UNARY_PLUS;
                break;
            case TOK_MINUS:
                unary->as.UnaryExpr.op = UNARY_NEG;
                break;
            case TOK_NOT:
                unary->as.UnaryExpr.op = UNARY_NOT;
                break;
            default:
                // UNREACHABLE
                break;
        }

        ASTNode* right = unaryExpression(parser);
        if (right == NULL) {
            freeASTNode(unary);
            return NULL;
        }

        unary->as.UnaryExpr.right = right;
        unary->length = (parser->current[-1].start + parser->current[-1].length) - unary->start;

        return unary;
    }

    return concatExpression(parser);
}

static ASTNode* powerExpression(Parser* parser) {

    ASTNode* expr = unaryExpression(parser);
    if (expr == NULL) {
        return NULL;
    }

    ASTNode* res = NULL;

    while (match(parser, TOK_CARAT)) {
        ASTNode* bin = ALLOC_AST;
        bin->type = EXPR_BINARY;
        bin->as.BinaryExpr.op = BIN_POWER;

        ASTNode* right = unaryExpression(parser);
        if (right == NULL) {
            freeASTNode(expr);
            freeASTNode(bin);
            return NULL;
        }

        if (res == NULL) {
            bin->as.BinaryExpr.left = expr;
            bin->as.BinaryExpr.right = right;
            bin->start = expr->start;
            bin->line = expr->line;
            bin->col = expr->col;
            res = bin;
        } else {
            bin->as.BinaryExpr.left = expr->as.BinaryExpr.right;
            bin->as.BinaryExpr.right = right;
            expr->as.BinaryExpr.right = bin;
            bin->start = bin->as.BinaryExpr.left->start;
            bin->line = bin->as.BinaryExpr.left->line;
            bin->col = bin->as.BinaryExpr.left->col;
        }

        bin->length = (parser->current[-1].start + parser->current[-1].length) - bin->start;

        expr = bin;
    }

    if (res != NULL) {
        ASTNode* curr = res;

        while (curr->type == EXPR_BINARY && curr->as.BinaryExpr.op == BIN_POWER) {
            curr->length = (parser->current[-1].start + parser->current[-1].length) - curr->start;
            curr = curr->as.BinaryExpr.right;
        }
    }

    return res == NULL ? expr : res;
}

static ASTNode* factorExpression(Parser* parser) {
    ASTNode* expr = powerExpression(parser);
    if (expr == NULL) {
        return NULL;
    }

    while (match(parser, TOK_STAR) || match(parser, TOK_SLASH) ||
           match(parser, TOK_MOD) || match(parser, TOK_DIV)) {
        ASTNode* bin = ALLOC_AST;
        bin->type = EXPR_BINARY;
        bin->start = expr->start;
        bin->line = expr->line;
        bin->col = expr->col;

        switch (parser->current[-1].type) {
            case TOK_STAR:
                bin->as.BinaryExpr.op = BIN_MULT;
                break;
            case TOK_SLASH:
                bin->as.BinaryExpr.op = BIN_DIV;
                break;
            case TOK_MOD:
                bin->as.BinaryExpr.op = BIN_MOD;
                break;
            case TOK_DIV:
                bin->as.BinaryExpr.op = BIN_FDIV;
                break;
            default:
                // UNREACHABLE
                break;
        }

        ASTNode* right = powerExpression(parser);
        if (right == NULL) {
            freeASTNode(expr);
            freeASTNode(bin);
            return NULL;
        }

        bin->as.BinaryExpr.left = expr;
        bin->as.BinaryExpr.right = right;
        bin->length = (parser->current[-1].start + parser->current[-1].length) - bin->start;
        expr = bin;
    }

    return expr;
}

static ASTNode* additiveExpression(Parser* parser) {
    ASTNode* expr = factorExpression(parser);
    if (expr == NULL) {
        return NULL;
    }

    while (match(parser, TOK_PLUS) || match(parser, TOK_MINUS)) {
        ASTNode* bin = ALLOC_AST;
        bin->type = EXPR_BINARY;
        bin->start = expr->start;
        bin->line = expr->line;
        bin->col = expr->col;
        if (parser->current[-1].type == TOK_PLUS) {
            bin->as.BinaryExpr.op = BIN_ADD;
        } else {
            bin->as.BinaryExpr.op = BIN_MINUS;
        }
        ASTNode* right = factorExpression(parser);
        if (right == NULL) {
            freeASTNode(expr);
            freeASTNode(bin);
            return NULL;
        }

        bin->as.BinaryExpr.left = expr;
        bin->as.BinaryExpr.right = right;
        bin->length = (parser->current[-1].start + parser->current[-1].length) - bin->start;
        expr = bin;
    }

    return expr;
}

static ASTNode* relationalExpression(Parser* parser) {
    ASTNode* expr = additiveExpression(parser);
    if (expr == NULL) {
        return NULL;
    }

    while (match(parser, TOK_GREATER) || match(parser, TOK_GREATER_EQUAL) ||
                match(parser, TOK_LESS) || match(parser, TOK_LESS_EQUAL)) {
        ASTNode* bin = ALLOC_AST;
        bin->type = EXPR_BINARY;
        bin->start = expr->start;
        bin->line = expr->line;
        bin->col = expr->col;

        switch (parser->current[-1].type) {
            case TOK_LESS:
                bin->as.BinaryExpr.op = LOGIC_LESS;
                break;
            case TOK_LESS_EQUAL:
                bin->as.BinaryExpr.op = LOGIC_LESS_EQUAL;
                break;
            case TOK_GREATER:
                bin->as.BinaryExpr.op = LOGIC_GREATER;
                break;
            case TOK_GREATER_EQUAL:
                bin->as.BinaryExpr.op = LOGIC_GREATER_EQUAL;
                break;
            default:
                // UNREACHABLE
                break;
        }

        ASTNode* right = additiveExpression(parser);
        if (right == NULL) {
            freeASTNode(expr);
            freeASTNode(bin);
            return NULL;
        }

        bin->as.BinaryExpr.left = expr;
        bin->as.BinaryExpr.right = right;
        bin->length = (parser->current[-1].start + parser->current[-1].length) - bin->start;
        expr = bin;
    }

    return expr;
}

static ASTNode* equality(Parser* parser) {
    ASTNode* expr = relationalExpression(parser);
    if (expr == NULL) {
        return NULL;
    }

    while (match(parser, TOK_EQUAL) || match(parser, TOK_NOT_EQUAL)) {
        ASTNode* bin = ALLOC_AST;
        bin->type = EXPR_BINARY;
        bin->start = expr->start;
        bin->line = expr->line;
        bin->col = expr->col;

        switch (parser->current[-1].type) {
            case TOK_EQUAL:
                bin->as.BinaryExpr.op = LOGIC_EQUAL;
                break;
            case TOK_NOT_EQUAL:
                bin->as.BinaryExpr.op = LOGIC_NOT_EQUAL;
                break;
            default:
                // UNREACHABLE
                break;
        }

        ASTNode* right = relationalExpression(parser);
        if (right == NULL) {
            freeASTNode(expr);
            freeASTNode(bin);
            return NULL;
        }

        bin->as.BinaryExpr.left = expr;
        bin->as.BinaryExpr.right = right;
        bin->length = (parser->current[-1].start + parser->current[-1].length) - bin->start;
        expr = bin;
    }

    return expr;
}

static ASTNode* andExpression(Parser* parser) {
    ASTNode* expr = equality(parser);
    if (expr == NULL) {
        return NULL;
    }

    while (match(parser, TOK_AND)) {
        ASTNode* bin = ALLOC_AST;
        bin->type = EXPR_BINARY;
        bin->start = expr->start;
        bin->line = expr->line;
        bin->col = expr->col;
        bin->as.BinaryExpr.op = LOGIC_AND;
        ASTNode* right = equality(parser);
        if (right == NULL) {
            freeASTNode(expr);
            freeASTNode(bin);
            return NULL;
        }

        bin->as.BinaryExpr.left = expr;
        bin->as.BinaryExpr.right = right;
        bin->length = (parser->current[-1].start + parser->current[-1].length) - bin->start;
        expr = bin;
    }

    return expr;
}

static ASTNode* orExpression(Parser* parser) {
    ASTNode* expr = andExpression(parser);
    if (expr == NULL) {
        return NULL;
    }

    while (match(parser, TOK_OR)) {
        ASTNode* bin = ALLOC_AST;
        bin->type = EXPR_BINARY;
        bin->start = expr->start;
        bin->line = expr->line;
        bin->col = expr->col;
        bin->as.BinaryExpr.op = LOGIC_OR;
        ASTNode* right = andExpression(parser);
        if (right == NULL) {
            freeASTNode(expr);
            freeASTNode(bin);
            return NULL;
        }

        bin->as.BinaryExpr.left = expr;
        bin->as.BinaryExpr.right = right;
        bin->length = (parser->current[-1].start + parser->current[-1].length) - bin->start;
        expr = bin;
    }

    return expr;
}

static ASTNode* assignmentLeftSide(Parser* parser) {
    ASTNode* access = ALLOC_AST;
    access->start = parser->current->start;
    access->line = parser->current->line;
    access->col = parser->current->col;
    Token* name = parser->current++;

    if (match(parser, TOK_LEFT_BRACKET)) {
        access->type = EXPR_ARRAY_ACCESS;

#define ARRAY access->as.ArrayAccessExpr

        ARRAY.name = name;
        ARRAY.indices[0] = NULL;
        ARRAY.indices[1] = NULL;

        ASTNode* index = expression(parser);
        if (index == NULL) {
            freeASTNode(access);
            return NULL;
        }

        ARRAY.indices[0] = index;

        if (match(parser, TOK_COMMA)) {
            index = expression(parser);
            if (index == NULL) {
                freeASTNode(access);
                return NULL;
            }

            ARRAY.indices[1] = index;
        }

        if (!consume(parser, TOK_RIGHT_BRACKET, "Expected ']' after array indices.")) {
            freeASTNode(access);
            return NULL;
        }

        // DETERMINE THE RESULT TYPE VIA THE SYMBOL TABLE (TO BE IMPLEMENTED)

        access->length = (parser->current[-1].start + parser->current[-1].length) - access->start;

        return access;

#undef ARRAY
    }

    access->type = EXPR_VARIABLE;
    access->as.VariableExpr.name = name;

    access->length = (parser->current[-1].start + parser->current[-1].length) - access->start;

    // DETERMINE THE RESULT TYPE VIA THE SYMBOL TABLE (TO BE IMPLEMENTED)

    return access;
}

static ASTNode* assignment(Parser* parser) {
    ASTNode* assign = ALLOC_AST;
    assign->type = EXPR_ASSIGN;
    assign->start = parser->current->start;
    assign->line = parser->current->line;
    assign->col = parser->current->col;

    Token* prev = parser->current;

#define ASSIGN assign->as.AssignmentExpr

    ASSIGN.right = NULL;
    ASSIGN.left = NULL;

    if (!check(parser, TOK_IDENTIFIER)) {
        freeASTNode(assign);
        return orExpression(parser);
    }

    ASTNode* left =  assignmentLeftSide(parser);
    if (left == NULL) {
        freeASTNode(assign);
        return NULL;
    }

    ASSIGN.left = left;

    if (!match(parser, TOK_ASSIGN)) {
        parser->current = prev;
        freeASTNode(assign);
        return orExpression(parser);
    }

    ASTNode* right = assignmentExpression(parser);
    //ASTNode* right = orExpression(parser);
    if (right == NULL) {
        freeASTNode(assign);
        return NULL;
    }

    ASSIGN.right = right;
    ASSIGN.resultType = ASSIGN.right->as.Expr.resultType;

    assign->length = (parser->current[-1].start + parser->current[-1].length) - assign->start;

    return assign;

#undef ASSIGN
}

static ASTNode* assignmentExpression(Parser* parser) {
    if (check(parser, TOK_IDENTIFIER)) {
        return assignment(parser);
    }

    return orExpression(parser);
}

static ASTNode* expression(Parser* parser) {
    return assignmentExpression(parser);
}

static ASTNode* declaration(Parser* parser);
static ASTNode* statement(Parser* parser);

static ASTNode* parameter(Parser* parser) {
    ASTNode* param = ALLOC_AST;
    param->type = AST_PARAMETER;
    param->start = parser->current->start;
    param->line = parser->current->line;
    param->col = parser->current->col;

    bool valid = true;

#define PARAM param->as.Parameter
    PARAM.byref = false;
    PARAM.isArray = false;

    if (match(parser, TOK_BYREF)) {
        PARAM.byref = true;
    }

    valid &= consume(parser, TOK_IDENTIFIER, "Expected parameter name.");

    PARAM.name = &parser->current[-1];

    valid &= consume(parser, TOK_COLON, "Expect ':' between parameter name and datatype.");

    switch (parser->current->type) {
        case TOK_INTEGER:
            PARAM.type = TYPE_INTEGER;
            break;
        case TOK_REAL:
            PARAM.type = TYPE_REAL;
            break;
        case TOK_BOOLEAN:
            PARAM.type = TYPE_BOOLEAN;
            break;
        case TOK_CHAR:
            PARAM.type = TYPE_CHAR;
            break;
        case TOK_STRING:
            PARAM.type = TYPE_STRING;
            break;
        case TOK_ARRAY:
            PARAM.isArray = true;
            advance(parser);
            valid &= consume(parser, TOK_LEFT_BRACKET, "Expect '[]', or '[,]' after ARRAY keyword.");
            if (match(parser, TOK_COMMA)) {
                PARAM.is2D = true;
            }
            valid &= consume(parser, TOK_RIGHT_BRACKET, "Expect closing ']'.");
            valid &= consume(parser, TOK_OF, "Expect keyword 'OF' between ARRAY specification and primitive type.");
            switch(parser->current->type) {
                case TOK_INTEGER:
                    PARAM.type = TYPE_INTEGER;
                    break;
                case TOK_REAL:
                    PARAM.type = TYPE_REAL;
                    break;
                case TOK_BOOLEAN:
                    PARAM.type = TYPE_BOOLEAN;
                    break;
                case TOK_CHAR:
                    PARAM.type = TYPE_CHAR;
                    break;
                case TOK_STRING:
                    PARAM.type = TYPE_STRING;
                    break;
                default:
                    errorAtCurrent(parser, "Expected valid primitive type for ARRAY.");
                    valid = false;
                    break;
            }
            break;
        default:
            errorAtCurrent(parser, "Expected valid parameter datatype.");
            valid = false;
            break;
    }

    advance(parser);

    param->length = (parser->current[-1].start + parser->current[-1].length) - param->start;

    if (!valid) {
        freeASTNode(param);
        return NULL;
    }

#undef PARAM

    return param;
}

static ASTNode* block(Parser* parser, TokenType endToken) {
    ASTNode* blck = ALLOC_AST;
    blck->type = STMT_BLOCK;
    blck->start = parser->current->start;
    blck->line = parser->current->line;
    blck->col = parser->current->col;

#define BLOCK blck->as.BlockStmt

    BLOCK.body.start = NULL;
    BLOCK.body.count = 0;
    BLOCK.body.size = 0;

    while (!check(parser, endToken) && !check(parser, TOK_EOF)) {
        if (match(parser, TOK_NEW_LINE)) continue;
        ASTNode* stmt = declaration(parser);

        if (stmt == NULL) {
            freeASTNode(blck);
            return NULL;
        }

        addASTNode(&BLOCK.body, stmt);
    }

    blck->length = (parser->current[-1].start + parser->current[-1].length) - blck->start;

    return blck;

#undef BLOCK
}

static ASTNode* function(Parser* parser) {
    ASTNode* func = ALLOC_AST;
    func->type = STMT_SUBROUTINE;
    func->start = parser->current[-1].start;
    func->line = parser->current[-1].line;
    func->col = parser->current[-1].col;

    bool valid = true;

#define FUNC func->as.SubroutineStmt

    FUNC.parameters.start = NULL;
    FUNC.parameters.count = 0;
    FUNC.parameters.size = 0;
    FUNC.body = NULL;

    FUNC.subroutineType = TYPE_FUNCTION;

    valid &= consume(parser, TOK_IDENTIFIER, "Expect function name.");

    FUNC.name = parser->current - 1;

    valid &= consume(parser, TOK_LEFT_PAREN, "Expect '(' after function name.");

    if (!check(parser, TOK_RIGHT_PAREN)) {
        do {
            ASTNode* param = parameter(parser);

            if (param == NULL) {
                freeASTNode(func);
                return NULL;
            }

            addASTNode(&FUNC.parameters, param);

        } while (match(parser, TOK_COMMA));
    }

    valid &= consume(parser, TOK_RIGHT_PAREN, "Expect ')' after parameter declarations.");

    valid &= consume(parser, TOK_RETURNS, "Expect 'RETURNS' keyword.");

    switch (parser->current->type) {
        case TOK_INTEGER:
            FUNC.returnValue = TYPE_INTEGER;
            break;
        case TOK_REAL:
            FUNC.returnValue = TYPE_REAL;
            break;
        case TOK_BOOLEAN:
            FUNC.returnValue = TYPE_BOOLEAN;
            break;
        case TOK_CHAR:
            FUNC.returnValue = TYPE_CHAR;
            break;
        case TOK_STRING:
            FUNC.returnValue = TYPE_STRING;
            break;
        case TOK_ARRAY:
            FUNC.returnValue = TYPE_ARRAY;
            break;
        default:
            errorAtCurrent(parser, "Expected valid primitive return datatype.");
            break;
    }

    advance(parser);

    valid &= consume(parser, TOK_NEW_LINE, "Expect new line after function header.");

    ASTNode* body = block(parser, TOK_ENDFUNCTION);

    if (body == NULL) {
        freeASTNode(func);
        return NULL;
    }

    FUNC.body = body;

    valid &= consume(parser, TOK_ENDFUNCTION, "Expect 'ENDFUNCTION' keyword to close function body.");

    valid &= consume(parser, TOK_NEW_LINE, "Expect new line after function declaration.");

    func->length = (parser->current[-1].start + parser->current[-1].length) - func->start;

    if (!valid) {
        freeASTNode(func);
        return NULL;
    }

    return func;

#undef FUNC
}

static ASTNode* procedure(Parser* parser) {
    ASTNode* proc = ALLOC_AST;
    proc->type = STMT_SUBROUTINE;
    proc->start = parser->current[-1].start;
    proc->line = parser->current[-1].line;
    proc->col = parser->current[-1].col;

    bool valid = true;

#define PROC proc->as.SubroutineStmt

    PROC.parameters.start = NULL;
    PROC.parameters.count = 0;
    PROC.parameters.size = 0;

    PROC.subroutineType = TYPE_PROCEDURE;

    valid &= consume(parser, TOK_IDENTIFIER, "Expect procedure name.");

    PROC.name = parser->current - 1;

    valid &= consume(parser, TOK_LEFT_PAREN, "Expect '(' after procedure name.");

    if (!check(parser, TOK_RIGHT_PAREN)) {
        do {
            ASTNode* param = parameter(parser);

            if (param == NULL) {
                freeASTNode(proc);
                return NULL;
            }

            addASTNode(&PROC.parameters, param);

        } while (match(parser, TOK_COMMA));
    }

    valid &= consume(parser, TOK_RIGHT_PAREN, "Expect ')' after parameter declarations.");

    valid &= consume(parser, TOK_NEW_LINE, "Expect new line after procedure header.");

    ASTNode* body = block(parser, TOK_ENDPROCEDURE);

    if (body == NULL) {
        freeASTNode(proc);
        return NULL;
    }

    PROC.body = body;

    valid &= consume(parser, TOK_ENDPROCEDURE, "Expect 'ENDPROCEDURE' keyword to close procedure body.");

    valid &= consume(parser, TOK_NEW_LINE, "Expect new line after procedure declaration.");

    proc->length = (parser->current[-1].start + parser->current[-1].length) - proc->start;

    if (!valid) {
        freeASTNode(proc);
        return NULL;
    }

    return proc;

#undef PROC
}

static ASTNode* symbolDeclaration(Parser* parser) {
    ASTNode* symbol = ALLOC_AST;

    bool valid = true;
    symbol->start = parser->current[-1].start;
    symbol->line = parser->current[-1].line;
    symbol->col = parser->current[-1].col;

    valid &= consume(parser, TOK_IDENTIFIER, "Expected variable name");

    Token* name = &parser->current[-1];

    valid &= consume(parser, TOK_COLON, "Expected ':' after variable name.");

    if (match(parser, TOK_ARRAY)) {
        symbol->type = STMT_ARRAY_DECLARE;

#define ARRAY symbol->as.ArrayDeclareStmt

        ARRAY.name = name;
        ARRAY.is2D = false;

        valid &= consume(parser, TOK_LEFT_BRACKET, "Expected '[' before array dimensions.");

        ASTNode* lBound = expression(parser);
        if (lBound == NULL) {
            freeASTNode(symbol);
            return NULL;
        }

        valid &= consume(parser, TOK_COLON, "Expected ':' delimiter between array dimensions.");
        ASTNode* rBound = expression(parser);
        if (rBound == NULL) {
            freeASTNode(lBound);
            freeASTNode(symbol);
            return NULL;
        }

        ARRAY.dimensions[0] = lBound;
        ARRAY.dimensions[1] = rBound;
        ARRAY.dimensions[2] = NULL;
        ARRAY.dimensions[3] = NULL;

        if (match(parser, TOK_COMMA)) {
            lBound = expression(parser);
            if (lBound == NULL) {
                freeASTNode(symbol);
                return NULL;
            }

            valid &= consume(parser, TOK_COLON, "Expected delimiter between array dimensions.");

            rBound = expression(parser);
            if (rBound == NULL) {
                freeASTNode(lBound);
                freeASTNode(symbol);
                freeASTNode(symbol);
                return NULL;
            }

            ARRAY.dimensions[2] = lBound;
            ARRAY.dimensions[3] = rBound;
            ARRAY.is2D = true;
        }

        valid &= consume(parser, TOK_RIGHT_BRACKET, "Expected closing ']' after array dimensions.");

        valid &= consume(parser, TOK_OF, "Expected 'OF' after array dimensions.");

        switch (parser->current->type) {
            case TOK_INTEGER:
                ARRAY.type = TYPE_INTEGER;
                break;
            case TOK_REAL:
                ARRAY.type = TYPE_REAL;
                break;
            case TOK_BOOLEAN:
                ARRAY.type = TYPE_BOOLEAN;
                break;
            case TOK_CHAR:
                ARRAY.type = TYPE_CHAR;
                break;
            case TOK_STRING:
                ARRAY.type = TYPE_STRING;
                break;
            case TOK_ARRAY:
                errorAtCurrent(parser, "Not possible to create nested array. Try bi-dimensional array notation instead.");
                freeASTNode(symbol);
                return NULL;
            default:
                errorAtCurrent(parser, "Not valid datatype for array.");
                freeASTNode(symbol);
                return NULL;
        }

        advance(parser);

        valid &= consume(parser, TOK_NEW_LINE, "Expected new line after array declaration.");

        symbol->length = (parser->current[-1].start + parser->current[-1].length) - symbol->start;

        if (!valid) {
            freeASTNode(symbol);
            return NULL;
        }

        return symbol;

#undef ARRAY
    }

    symbol->type = STMT_VAR_DECLARE;

#define VAR symbol->as.VarDeclareStmt

    VAR.name = name;

    switch (parser->current->type) {
        case TOK_INTEGER:
            VAR.type = TYPE_INTEGER;
            break;
        case TOK_REAL:
            VAR.type = TYPE_REAL;
            break;
        case TOK_CHAR:
            VAR.type = TYPE_CHAR;
            break;
        case TOK_BOOLEAN:
            VAR.type = TYPE_BOOLEAN;
            break;
        case TOK_STRING:
            VAR.type = TYPE_STRING;
            break;
        default:
            errorAtCurrent(parser, "Expected valid primitive datatype.");
            freeASTNode(symbol);
            return NULL;
    }

    advance(parser);

    valid &= consume(parser, TOK_NEW_LINE, "Expected new line after variable declaration.");

    symbol->length = (parser->current[-1].start + parser->current[-1].length) - symbol->start;

    if (!valid) {
        freeASTNode(symbol);
        return NULL;
    }

    return symbol;

#undef VAR
}

static ASTNode* constantDeclaration(Parser* parser) {
    bool valid = true;

    ASTNode* cst = ALLOC_AST;
    cst->type = STMT_CONST_DECLARE;
    cst->start = parser->current[-1].start;
    cst->line = parser->current[-1].line;
    cst->col = parser->current[-1].col;

#define CONST cst->as.ConstDeclareStmt

    valid &= consume(parser, TOK_IDENTIFIER, "Expected identifier for constant.");

    CONST.name = parser->current - 1;

    valid &= consume(parser, TOK_EQUAL, "Expected assignment of constant value using '='.");

    switch (parser->current->type) {
        case TOK_INT_LIT:
            CONST.value = parser->current;
            CONST.type = TYPE_INTEGER;
            break;
        case TOK_REAL_LIT:
            CONST.value = parser->current;
            CONST.type = TYPE_REAL;
            break;
        case TOK_CHAR_LIT:
            CONST.value = parser->current;
            CONST.type = TYPE_CHAR;
            break;
        case TOK_TRUE:
        case TOK_FALSE:
            CONST.value = parser->current;
            CONST.type = TYPE_BOOLEAN;
            break;
        case TOK_STRING_LIT:
            CONST.value = parser->current;
            CONST.type = TYPE_STRING;
            break;
        default:
            errorAtCurrent(parser, "Not a valid literal for CONSTANT value.");
            freeASTNode(cst);
            return NULL;
    }

    advance(parser);

    valid &= consume(parser, TOK_NEW_LINE, "Expected new line after constant literal.");

    cst->length = (parser->current[-1].start + parser->current[-1].length) - cst->start;

    if (!valid) {
        freeASTNode(cst);
        return NULL;
    }

    return cst;

#undef CONST
}

static ASTNode* ifBody(Parser* parser) {
    ASTNode* blck = ALLOC_AST;
    blck->type = STMT_BLOCK;
    blck->start = parser->current->start;
    blck->line = parser->current->line;
    blck->col = parser->current->col;

#define BLOCK blck->as.BlockStmt

    BLOCK.body.start = NULL;
    BLOCK.body.count = 0;
    BLOCK.body.size = 0;

    while (!check(parser, TOK_ELSE) && !check(parser, TOK_ENDIF) && !check(parser, TOK_EOF)) {
        if (match(parser, TOK_NEW_LINE)) continue;

        ASTNode* stmt = declaration(parser);

        if (stmt == NULL) {
            freeASTNode(blck);
            return NULL;
        }

        addASTNode(&BLOCK.body, stmt);
    }

    blck->length = (parser->current[-1].start + parser->current[-1].length) - blck->start;

    return blck;

#undef BLOCK
}

static ASTNode* ifStatement(Parser* parser) {
    bool valid = true;

    ASTNode* ifStmt = ALLOC_AST;
    ifStmt->type = STMT_IF;
    ifStmt->start = parser->current[-1].start;
    ifStmt->line = parser->current[-1].line;
    ifStmt->col = parser->current[-1].col;
#define IF ifStmt->as.IfStmt

    IF.thenBranch = NULL;
    IF.elseBranch = NULL;

    ASTNode* condition = expression(parser);
    if (condition == NULL) {
        freeASTNode(ifStmt);
        return NULL;
    }

    IF.condition = condition;

    match(parser, TOK_NEW_LINE);

    valid &= consume(parser, TOK_THEN, "Expected 'THEN' keyword before IF-THEN branch.");

    valid &= consume(parser, TOK_NEW_LINE, "Expect new line before IF-THEN branch.");

    ASTNode* thenBranch = ifBody(parser);

    if (thenBranch == NULL) {
        freeASTNode(ifStmt);
        return NULL;
    }

    IF.thenBranch = thenBranch;

    if (check(parser, TOK_ELSE)) {
        advance(parser);

        valid &= consume(parser, TOK_NEW_LINE, "Expect new line after ELSE.");

        ASTNode* elseBranch = block(parser, TOK_ENDIF);

        if (elseBranch == NULL) {
            freeASTNode(ifStmt);
            return NULL;
        }

        IF.elseBranch = elseBranch;
    }

    valid &= consume(parser, TOK_ENDIF, "Unterminated IF statement. Expected ENDIF keyword.");

    valid &= consume(parser, TOK_NEW_LINE, "Expect new line after ENDIF keyword.");

    ifStmt->length = (parser->current[-1].start + parser->current[-1].length) - ifStmt->start;

    if (!valid) {
        freeASTNode(ifStmt);
        return NULL;
    }

    return ifStmt;

#undef IF
}

static ASTNode* caseLine(Parser* parser) {
    bool valid = true;

    ASTNode* line = ALLOC_AST;
    line->type = STMT_CASE_LINE;
    line->start = parser->current->start;
    line->line = parser->current->line;
    line->col = parser->current->col;


    ASTNode* block = ALLOC_AST;
    block->type = STMT_BLOCK;
    block->start = parser->current->start;
    block->line = parser->current->line;
    block->col = parser->current->col;

#define LINE line->as.CaseLineStmt
#define BLCK block->as.BlockStmt

    LINE.value = NULL;
    LINE.result = block;
    BLCK.body.start = NULL;
    BLCK.body.count = 0;
    BLCK.body.count = 0;

    if (!match(parser, TOK_OTHERWISE)) {
        ASTNode* expr = expression(parser);
        if (expr == NULL) {
            freeASTNode(line);
            return NULL;
        }

        LINE.value = expr;
    }

    valid &= consume(parser, TOK_COLON, "Expected ':' after OTHERWISE keyword.");

    match(parser, TOK_NEW_LINE);

    ASTNode* res = declaration(parser);
    if (res == NULL) {
        freeASTNode(line);
        freeASTNode(block);
        return NULL;
    }

    addASTNode(&BLCK.body, res);

    while (true) {
        Token* temp = parser->current;
        if (parser->current->type == TOK_OTHERWISE || parser->current->type == TOK_EOF || parser->current->type == TOK_ENDCASE) {
            break;
        }
        parser->produceErrors = false;
        ASTNode* testExpr = expression(parser);
        parser->produceErrors = true;
        if (testExpr != NULL && parser->current->type == TOK_COLON) {
            parser->current = temp;
            freeASTNode(testExpr);
            break;
        }

        parser->current = temp;
        ASTNode* subRes = declaration(parser);
        if (subRes == NULL) {
            freeASTNode(line);
            freeASTNode(block);
            return NULL;
        }

        addASTNode(&BLCK.body, subRes);
    }

    line->length = (parser->current[-1].start + parser->current[-1].length) - line->start;
    block->length = (parser->current[-1].start + parser->current[-1].length) - block->start;

    if (!valid) {
        freeASTNode(line);
        freeASTNode(block);
        return NULL;
    }

    return line;

#undef LINE
#undef BLCK
}

static ASTNode* caseBlock(Parser* parser) {
    ASTNode* caseBlck = ALLOC_AST;
    caseBlck->type = STMT_CASE_BLOCK;
    caseBlck->start = parser->current->start;
    caseBlck->line = parser->current->line;
    caseBlck->col = parser->current->col;

    ASTNode* blck = ALLOC_AST;
    blck->type = STMT_BLOCK;
    blck->start = parser->current->start;
    blck->line = parser->current->line;
    blck->col = parser->current->col;

    caseBlck->as.CaseBlockStmt.body = blck;

#define BLOCK blck->as.BlockStmt

    BLOCK.body.start = NULL;
    BLOCK.body.count = 0;
    BLOCK.body.size = 0;

    while (!check(parser, TOK_ENDCASE) && !check(parser, TOK_EOF)) {
        if (match(parser, TOK_NEW_LINE)) continue;

        ASTNode* stmt = caseLine(parser);

        if (stmt == NULL) {
            freeASTNode(caseBlck);
            return NULL;
        }

        addASTNode(&BLOCK.body, stmt);
    }

    blck->length = (parser->current[-1].start + parser->current[-1].length) - blck->start;
    caseBlck->length = (parser->current[-1].start + parser->current[-1].length) - caseBlck->start;

    return caseBlck;

#undef BLOCK
}

static ASTNode* caseStatement(Parser* parser) {
    bool valid = true;

    ASTNode* caseStmt = ALLOC_AST;
    caseStmt->type = STMT_CASE;
    caseStmt->start = parser->current[-1].start;
    caseStmt->line = parser->current[-1].line;
    caseStmt->col = parser->current[-1].col;

#define CASE caseStmt->as.CaseStmt

    ASTNode* expr = expression(parser);
    if (expr == NULL) {
        freeASTNode(caseStmt);
        return NULL;
    }

    CASE.expr = expr;

    valid &= consume(parser, TOK_OF, "Expected OF keyword after CASE expression.");

    valid &= consume(parser, TOK_NEW_LINE, "Expect new line after CASE header.");

    ASTNode* body = caseBlock(parser);
    if (body == NULL) {
        freeASTNode(caseStmt);
        return NULL;
    }

    CASE.body = body;

    valid &= consume(parser, TOK_ENDCASE, "Expected ENDCASE after CASE body.");

    valid &= consume(parser, TOK_NEW_LINE, "Expected new line after CASE statement.");

    caseStmt->length = (parser->current[-1].start + parser->current[-1].length) - caseStmt->start;

    if (!valid) {
        freeASTNode(caseStmt);
        return NULL;
    }

    return caseStmt;
#undef CASE
}

static ASTNode* forStatement(Parser* parser) {
    bool valid = true;

    ASTNode* forStmt = ALLOC_AST;
    forStmt->type = STMT_FOR;
    forStmt->start = parser->current[-1].start;
    forStmt->line = parser->current[-1].line;
    forStmt->col = parser->current[-1].col;

#define FOR forStmt->as.ForStmt

    valid &= consume(parser, TOK_IDENTIFIER, "Expected counter identifier.");

    FOR.counterName = parser->current - 1;

    valid &= consume(parser, TOK_ASSIGN, "Expected assignment operator '<-'.");

    ASTNode* init = expression(parser);
    if (init == NULL) {
        freeASTNode(forStmt);
        return NULL;
    }

    FOR.init = init;

    valid &= consume(parser, TOK_TO, "Expected 'TO' keyword.");

    ASTNode* end = expression(parser);
    if (end == NULL) {
        freeASTNode(forStmt);
        return NULL;
    }

    FOR.end = end;
    FOR.step = NULL;

    if (match(parser, TOK_STEP)) {
        //ASTNode* step = primary(parser);
        ASTNode* step = unaryExpression(parser);
        if (step == NULL) {
            freeASTNode(forStmt);
            return NULL;
        }

        FOR.step = step;
    }

    valid &= consume(parser, TOK_NEW_LINE, "Expected new line after FOR loop header.");

    ASTNode* body = block(parser, TOK_NEXT);
    if (body == NULL) {
        freeASTNode(forStmt);
        return NULL;
    }

    FOR.body = body;

    advance(parser);

    valid &= consume(parser, TOK_IDENTIFIER, "Expected counter name but got none.");

    Token* prev = parser->current - 1;

    if (prev->length != FOR.counterName->length || memcmp(prev->start, FOR.counterName->start, FOR.counterName->length) != 0) {
        errorAtCurrent(parser, "");
        fprintf(stderr, "Expected counter name '%.*s' after NEXT, but got '%.*s'.", FOR.counterName->length, FOR.counterName->start, prev->length, prev->start);
        valid = false;
    }

    valid &= consume(parser, TOK_NEW_LINE, "Expected new line after FOR statement.");

    forStmt->length = (parser->current[-1].start + parser->current[-1].length) - forStmt->start;

    if (!valid) {
        freeASTNode(forStmt);
        return NULL;
    }

    return forStmt;
#undef FOR
}

static ASTNode* whileStatement(Parser* parser) {
    bool valid = true;

    ASTNode* whileStmt = ALLOC_AST;
    whileStmt->type = STMT_WHILE;
    whileStmt->start = parser->current[-1].start;
    whileStmt->line = parser->current[-1].line;
    whileStmt->col = parser->current[-1].col;

#define WHILE whileStmt->as.WhileStmt

    ASTNode* condition = expression(parser);

    if (condition == NULL) {
        freeASTNode(whileStmt);
        return NULL;
    }

    WHILE.condition = condition;

    match(parser, TOK_NEW_LINE);

    valid &= consume(parser, TOK_DO, "Expected 'DO' keyword after WHILE statement condition.");

    valid &= consume(parser, TOK_NEW_LINE, "Expect new line after 'DO' keyword in WHILE loop header.");

    ASTNode* body = block(parser, TOK_ENDWHILE);
    if (body == NULL) {
        freeASTNode(whileStmt);
        return NULL;
    }

    WHILE.body = body;

    valid &= consume(parser, TOK_ENDWHILE, "Expected 'ENDWHILE' keyword to close WHILE loop body.");
    valid &= consume(parser, TOK_NEW_LINE, "Expected new line after WHILE statement.");

    whileStmt->length = (parser->current[-1].start + parser->current[-1].length) - whileStmt->start;

    if (!valid) {
        freeASTNode(whileStmt);
        return NULL;
    }

    return whileStmt;
#undef WHILE
}

static ASTNode* repeatStatement(Parser* parser) {
    bool valid = true;

    ASTNode* rep = ALLOC_AST;
    rep->type = STMT_REPEAT;
    rep->start = parser->current[-1].start;
    rep->line = parser->current[-1].line;
    rep->col = parser->current[-1].col;

#define REPEAT rep->as.RepeatStmt

    valid &= consume(parser, TOK_NEW_LINE, "Expect new line after 'REPEAT' keyword.");

    ASTNode* body = block(parser, TOK_UNTIL);
    if (body == NULL) {
        freeASTNode(rep);
        return NULL;
    }

    REPEAT.body = body;

    valid &= consume(parser, TOK_UNTIL, "Expected 'UNTIL' keyword closing REPEAT-UNTIL loop body.");

    ASTNode* until = expression(parser);
    if (until == NULL) {
        freeASTNode(rep);
        return NULL;
    }

    REPEAT.condition = until;

    valid &= consume(parser, TOK_NEW_LINE, "Expected new line after REPEAT statement.");

    rep->length = (parser->current[-1].start + parser->current[-1].length) - rep->start;

    if (!valid) {
        freeASTNode(rep);
        return NULL;
    }

    return rep;

#undef REPEAT
}

static ASTNode* returnStatement(Parser* parser) {
    bool valid = true;

    ASTNode* ret = ALLOC_AST;
    ret->type = STMT_RETURN;
    ret->start = parser->current[-1].start;
    ret->line = parser->current[-1].line;
    ret->col = parser->current[-1].col;

#define RETURN ret->as.ReturnStmt

    RETURN.expr = NULL;

    ASTNode* expr = expression(parser);
    if (expr == NULL) {
        freeASTNode(ret);
        return NULL;
    }

    RETURN.expr = expr;
    RETURN.returnType = expr->as.Expr.resultType;

    valid &= consume(parser, TOK_NEW_LINE, "Expected mew line after return statement.");

    ret->length = (parser->current[-1].start + parser->current[-1].length) - ret->start;

    if (!valid) {
        freeASTNode(ret);
        return NULL;
    }

    return ret;

#undef RETURN
}

static ASTNode* callStatement(Parser* parser) {
    bool valid = true;

    ASTNode* call = ALLOC_AST;
    call->type = STMT_CALL;
    call->start = parser->current[-1].start;
    call->line = parser->current[-1].line;
    call->col = parser->current[-1].col;

#define CALL call->as.CallStmt

    CALL.arguments.start = NULL;
    CALL.arguments.count = 0;
    CALL.arguments.size = 0;

    valid &= consume(parser, TOK_IDENTIFIER, "Expected procedure name after 'CALL' keyword.");

    if (!valid) {
        freeASTNode(call);
        return NULL;
    }

    CALL.name = parser->current - 1;

    valid &= consume(parser, TOK_LEFT_PAREN, "Expected '(' after procedure name.");

    if (!check(parser, TOK_RIGHT_PAREN)) {
        do {
            ASTNode* arg = expression(parser);
            if (arg == NULL) {
                freeASTNode(call);
                return NULL;
            }

            addASTNode(&CALL.arguments, arg);
        } while (match(parser, TOK_COMMA));
    }

    valid &= consume(parser, TOK_RIGHT_PAREN, "Expected ')' after arguments in CALL statement.");

    valid &= consume(parser, TOK_NEW_LINE, "Expect new line after CALL statement.");

    call->length = (parser->current[-1].start + parser->current[-1].length) - call->start;

    if (!valid) {
        freeASTNode(call);
        return NULL;
    }

    return call;

#undef CALL
}

static ASTNode* inputStatement(Parser* parser) {
    bool valid = true;
    ASTNode* inp = ALLOC_AST;
    inp->type = STMT_INPUT;
    inp->start = parser->current[-1].start;
    inp->line = parser->current[-1].line;
    inp->col = parser->current[-1].col;

    valid &= consume(parser, TOK_IDENTIFIER, "Expect identifier name after 'INPUT' keyword.");

    inp->as.InputStmt.name = parser->current - 1;

    ASTNode* varAccess = symbolExpression(parser);
    if (varAccess == NULL) {
        freeASTNode(inp);
        return NULL;
    }

    inp->as.InputStmt.varAccess = varAccess;

    valid &= consume(parser, TOK_NEW_LINE, "Expect new line after INPUT statement.");

    inp->length = (parser->current[-1].start + parser->current[-1].length) - inp->start;

    if (!valid) {
        freeASTNode(inp);
        return NULL;
    }

    return inp;
}

static ASTNode* outputStatement(Parser* parser) {
    bool valid = true;
    ASTNode* out = ALLOC_AST;
    out->type = STMT_OUTPUT;
    out->start = parser->current[-1].start;
    out->line = parser->current[-1].line;
    out->col = parser->current[-1].col;

#define OUT out->as.OutputStmt

    OUT.expressions.start = NULL;
    OUT.expressions.count = 0;
    OUT.expressions.size = 0;

    do {
        ASTNode* expr = expression(parser);
        if (expr == NULL) {
            freeASTNode(out);
            return NULL;
        }

        addASTNode(&OUT.expressions, expr);
    } while (match(parser, TOK_COMMA));

    valid &= consume(parser, TOK_NEW_LINE, "Expect new line after OUTPUT statement.");

    out->length = (parser->current[-1].start + parser->current[-1].length) - out->start;

    if (!valid) {
        freeASTNode(out);
        return NULL;
    }

    return out;

#undef OUT
}

static ASTNode* openfileStatement(Parser* parser) {
    bool valid = true;
    ASTNode* openfile = ALLOC_AST;
    openfile->type = STMT_OPENFILE;
    openfile->start = parser->current->start;
    openfile->line = parser->current->line;
    openfile->col = parser->current->col;

#define OFILE openfile->as.OpenfileStmt

    OFILE.filename = NULL;
    OFILE.accessType = ACCESS_NONE;

    if (!consume(parser, TOK_STRING_LIT, "Expected string literal referring to file name.")) {
        freeASTNode(openfile);
        return NULL;
    }

    OFILE.filename = parser->current - 1;

    valid &= consume(parser, TOK_FOR, "Expected FOR keyword after file name.");

    switch (parser->current->type) {
        case TOK_READ:
            OFILE.accessType = ACCESS_READ;
            break;
        case TOK_WRITE:
            OFILE.accessType = ACCESS_WRITE;
            break;
        case TOK_APPEND:
            OFILE.accessType = ACCESS_APPEND;
            break;
        default:
            errorAtCurrent(parser, "Not a valid file access specifier.");
            freeASTNode(openfile);
            return NULL;
    }

    advance(parser);

    valid &= consume(parser, TOK_NEW_LINE, "Expect new line after OPENFILE statement.");

    if (!valid) {
        freeASTNode(openfile);
        return NULL;
    }

    openfile->length = (parser->current[-1].start + parser->current[-1].length) - openfile->start;

    return openfile;

#undef OFILE
}

static ASTNode* closefileStatement(Parser* parser) {
    ASTNode* closefile = ALLOC_AST;
    closefile->type = STMT_CLOSEFILE;
    closefile->start = parser->current->start;
    closefile->line = parser->current->line;
    closefile->col = parser->current->col;

#define CFILE closefile->as.ClosefileStmt

    CFILE.filename = NULL;

    if (!consume(parser, TOK_STRING_LIT, "Expected string literal referring to file name.")) {
        freeASTNode(closefile);
        return NULL;
    }

    CFILE.filename = parser->current - 1;

    if (!consume(parser, TOK_NEW_LINE, "Expected new line after CLOSEFILE statement.")) {
        freeASTNode(closefile);
        return NULL;
    }

    closefile->length = (parser->current[-1].start + parser->current[-1].length) - closefile->start;

    return closefile;

#undef CFILE
}

static ASTNode* readfileStatement(Parser* parser) {
    bool valid = true;
    ASTNode* readfile = ALLOC_AST;
    readfile->type = STMT_READFILE;
    readfile->start = parser->current->start;
    readfile->line = parser->current->line;
    readfile->col = parser->current->col;

#define RFILE readfile->as.ReadfileStmt

    RFILE.filename = NULL;
    RFILE.varAccess = NULL;

    if (!consume(parser, TOK_STRING_LIT, "Expected string literal referring to file name.")) {
        freeASTNode(readfile);
        return NULL;
    }

    RFILE.filename = parser->current - 1;

    valid &= consume(parser, TOK_COMMA, "Expected comma separator between file name and target variable access.");

    valid &= consume(parser, TOK_IDENTIFIER, "Expect identifier name after 'INPUT' keyword.");

    ASTNode* varAccess = symbolExpression(parser);
    if (varAccess == NULL) {
        freeASTNode(readfile);
        return NULL;
    }

    RFILE.varAccess = varAccess;

    valid &= consume(parser, TOK_NEW_LINE, "Expected new line after READFILE statement.");

    if (!valid) {
        freeASTNode(readfile);
        return NULL;
    }

    readfile->length = (parser->current[-1].start + parser->current[-1].length) - readfile->start;

    return readfile;

#undef RFILE
}

static ASTNode* writefileStatement(Parser* parser) {
    bool valid = true;
    ASTNode* writefile = ALLOC_AST;
    writefile->type = STMT_WRITEFILE;
    writefile->start = parser->current->start;
    writefile->line = parser->current->line;
    writefile->col = parser->current->col;

#define WFILE writefile->as.WritefileStmt

    WFILE.filename = NULL;
    WFILE.expressions.start = NULL;
    WFILE.expressions.count = 0;
    WFILE.expressions.size = 0;

    if (!consume(parser, TOK_STRING_LIT, "Expected string literal referring to file name.")) {
        freeASTNode(writefile);
        return NULL;
    }

    WFILE.filename = parser->current - 1;

    valid &= consume(parser, TOK_COMMA, "Expected comma separator between file name and expression list.");

    do {
        ASTNode* expr = expression(parser);
        if (expr == NULL) {
            freeASTNode(writefile);
            return NULL;
        }

        addASTNode(&WFILE.expressions, expr);
    } while (match(parser, TOK_COMMA));

    valid &= consume(parser, TOK_NEW_LINE, "Expected new line after WRITEFILE statement.");

    if (!valid) {
        freeASTNode(writefile);
        return NULL;
    }

    writefile->length = (parser->current[-1].start + parser->current[-1].length) - writefile->start;

    return writefile;

#undef WFILE
}

static ASTNode* expressionStatement(Parser* parser) {
    ASTNode* exprStmt = ALLOC_AST;
    exprStmt->type = STMT_EXPR;
    exprStmt->start = parser->current->start;
    exprStmt->line = parser->current->line;
    exprStmt->col = parser->current->col;

#define EXPR exprStmt->as.ExprStmt

    EXPR.expr = NULL;

    ASTNode* expr = expression(parser);
    if (expr == NULL) {
        freeASTNode(exprStmt);
        return NULL;
    }

    EXPR.expr = expr;

    if (!consume(parser, TOK_NEW_LINE, "Expect new line after statement.")) {
        freeASTNode(exprStmt);
        return NULL;
    }

    exprStmt->length = (parser->current[-1].start + parser->current[-1].length) - exprStmt->start;

    return exprStmt;

#undef EXPR
}

static ASTNode* statement(Parser* parser) {
    ASTNode* stmt;

    if (match(parser, TOK_IF)) {
        stmt = ifStatement(parser);
    } else if (match(parser, TOK_CASE)) {
        stmt = caseStatement(parser);
    } else if (match(parser, TOK_FOR)) {
        stmt = forStatement(parser);
    } else if (match(parser, TOK_WHILE)) {
        stmt = whileStatement(parser);
    } else if (match(parser, TOK_REPEAT)) {
        stmt = repeatStatement(parser);
    } else if (match(parser, TOK_RETURN)) {
        stmt = returnStatement(parser);
    } else if (match(parser, TOK_CALL)) {
        stmt = callStatement(parser);
    } else if (match(parser, TOK_INPUT)) {
        stmt = inputStatement(parser);
    } else if (match(parser, TOK_OUTPUT)) {
        stmt = outputStatement(parser);
    } else if (match(parser, TOK_OPENFILE)) {
        stmt = openfileStatement(parser);
    } else if (match(parser, TOK_CLOSEFILE)) {
        stmt = closefileStatement(parser);
    } else if (match(parser, TOK_READFILE)) {
        stmt = readfileStatement(parser);
    } else if (match(parser, TOK_WRITEFILE)) {
        stmt = writefileStatement(parser);
    } else {
        stmt = expressionStatement(parser);
    }

    return stmt;
}

static ASTNode* declaration(Parser* parser) {

    ASTNode* res;

    if (match(parser, TOK_NEW_LINE)) {
        res = declaration(parser);
    } else if (match(parser, TOK_FUNCTION)) {
        res = function(parser);
    } else if (match(parser, TOK_PROCEDURE)) {
        res = procedure(parser);
    } else if (match(parser, TOK_DECLARE)) {
        res = symbolDeclaration(parser);
    } else if (match(parser, TOK_CONSTANT)) {
        res = constantDeclaration(parser);
    } else {
        res = statement(parser);
    }

    return res;
}

bool genAST(Parser* parser) {
    parser->ast.program = ALLOC_AST;
    ASTNode* program = parser->ast.program;
    program->type = STMT_PROGRAM;
    program->start = parser->current->start;
    program->line = parser->current->line;
    program->col = parser->current->col;
    program->length = (long)strlen(parser->current->start);
    program->as.ProgramStmt.body.start = ALLOC_AST_ARRAY(10);
    program->as.ProgramStmt.body.count = 0;
    program->as.ProgramStmt.body.size = 10;

    while (!match(parser, TOK_EOF)) {
        ASTNode* next = declaration(parser);

        if (next == NULL || parser->panicMode) {
            synchronise(parser);
        }

        addASTNode(&program->as.ProgramStmt.body, next);
    }

    return parser->hadError;
}

void printAST(Parser* parser) {
    printASTNode(parser->ast.program, 0);
}