#include "compiler.h"

static bool findSymbol(Compiler* compiler, const char* key, Symbol* symbol) {
    return getTable(compiler->symbolTable, key, symbol);
}

static bool findSymbolInCurrScope(Compiler* compiler, const char* key, Symbol* symbol) {
    return getCurrTable(compiler->symbolTable, key, symbol);
}

static bool initialiseSymbol(Compiler* compiler, const char* key) {
    Symbol* symbol = getTablePointer(compiler->symbolTable, key);

    if (symbol == NULL) return false;

    symbol->initialised = true;

    return true;
}

static bool addSymbol(Compiler* compiler, const char* key, ASTNode* node, SymbolType type, bool isRelative, bool byref, int size) {
    int pos = compiler->symbolTable->nextPos;
    compiler->symbolTable->nextPos += size;
    return setTable(compiler->symbolTable, key, node, type, pos, isRelative, byref);
}

static bool addFile(Compiler* compiler, const char* key, ASTNode* node, SymbolType type, bool isRelative, bool byref, FileAccessType access) {
    int size = 8;
    int pos = compiler->symbolTable->nextPos;
    compiler->symbolTable->nextPos += size;
    return setTableFile(compiler->symbolTable, key, node, type, pos, isRelative, byref, access);
}

static bool addSubroutineSymbol(Compiler* compiler, const char* key, ASTNode* node, int pos) {
    SymbolType type = node->as.SubroutineStmt.subroutineType == TYPE_FUNCTION ? SYMBOL_FUNC : SYMBOL_PROC;

    return setTable(compiler->symbolTable, key, node, type, pos, false, false);
}

static bool createScope(Compiler* compiler, ScopeType type) {
    SymbolTable* newTable = malloc(sizeof(SymbolTable));
    if (newTable == NULL) return false;

    initTable(newTable);
    newTable->scopeType = type;
    newTable->enclosing = compiler->symbolTable;
    compiler->symbolTable = newTable;
    compiler->depth++;
    return true;
}

static bool endScope(Compiler* compiler) {
    SymbolTable* end = compiler->symbolTable;
    if (end == compiler->globalTable) return false;
    compiler->symbolTable = compiler->symbolTable->enclosing;
    end->enclosing = NULL;
    freeTable(end);
    compiler->depth--;
    return true;
}

static void addOp(Compiler* compiler, Instruction op) {
    addInstruction(compiler->bStream, op);
}

static void addByte(Compiler* compiler, byte b) {
    addBytecode(compiler->bStream, b);
}

#define ADD_BYTE(x)  addByte(compiler, *(byte*)&x)
#define ADD_4BYTE(x)  { byte4 temp = *(byte4*)&x; \
                      addByte(compiler, (byte)((temp >> 24) & 0xff));                        \
                      addByte(compiler, (byte)((temp >> 16) & 0xff));                        \
                      addByte(compiler, (byte)((temp >> 8) & 0xff));                        \
                      addByte(compiler, (byte)((temp) & 0xff));}

#define ADD_8BYTE(x)  {byte8 temp = *(byte8*)&x; \
                        addByte(compiler, (byte)((temp >> 56) & 0xff)); \
                        addByte(compiler, (byte)((temp >> 48) & 0xff)); \
                        addByte(compiler, (byte)((temp >> 40) & 0xff)); \
                        addByte(compiler, (byte)((temp >> 32) & 0xff)); \
                        addByte(compiler, (byte)((temp >> 24) & 0xff)); \
                        addByte(compiler, (byte)((temp >> 16) & 0xff)); \
                        addByte(compiler, (byte)((temp >> 8) & 0xff)); \
                        addByte(compiler, (byte)((temp) & 0xff));}

#define ADD_INT(x)  ADD_4BYTE(x)
#define ADD_REAL(x)  ADD_8BYTE(x)
#define ADD_CHAR(x)  ADD_BYTE(x)
#define ADD_BOOL(x)  ADD_BYTE(x)
#define ADD_REF(x)  ADD_8BYTE(x)

static void compileNode(Compiler* compiler, ASTNode* node) {
    if (node == NULL) return;

    switch (node->type) {
        case EXPR_LITERAL: {
            switch (node->as.LiteralExpr.resultType) {
                case TYPE_INTEGER: {
                    char* num = malloc(sizeof(char) * (node->as.LiteralExpr.value->length + 1));
                    memcpy(num, node->as.LiteralExpr.value->start, node->as.LiteralExpr.value->length * sizeof(char));
                    num[node->as.LiteralExpr.value->length] = '\0';
                    int n = atoi(num);
                    free(num);
                    addOp(compiler, LOAD_INT);
                    ADD_INT(n);
                    break;
                }
                case TYPE_REAL: {
                    char* num = malloc(sizeof(char) * (node->as.LiteralExpr.value->length + 1));
                    memcpy(num, node->as.LiteralExpr.value->start, node->as.LiteralExpr.value->length * sizeof(char));
                    num[node->as.LiteralExpr.value->length] = '\0';
                    double n = strtod(num, NULL);
                    free(num);
                    addOp(compiler, LOAD_REAL);
                    ADD_REAL(n);
                    break;
                }
                case TYPE_CHAR: {
                    addOp(compiler, LOAD_CHAR);
                    ADD_CHAR(node->as.LiteralExpr.value->start[1]);
                    break;
                }
                case TYPE_BOOLEAN: {
                    bool res = false;
                    if (node->as.LiteralExpr.value->start[0] == 'T') {
                        res = true;
                    }

                    addOp(compiler, LOAD_BOOL);
                    ADD_BOOL(res);
                    break;
                }
                case TYPE_STRING: {
                    addOp(compiler, LOAD_STRING);
                    ADD_INT(node->as.LiteralExpr.value->length - 2);
                    for (int i = 1; i < node->as.LiteralExpr.value->length - 1; i++) {
                        ADD_CHAR(node->as.LiteralExpr.value->start[i]);
                    }
                    break;
                }
                default: break;
            }
            break;
        }
        case EXPR_CALL: {
            char* name = extractNullTerminatedString(node->as.CallExpr.name->start, node->as.CallExpr.name->length);
            Symbol callable;
            bool res = findSymbol(compiler, name, &callable);
            if (!res) {
                printf("Something went wrong. This shouldn't be able to happen.\n");
                break;
            }
            free(name);

            if (callable.type == SYMBOL_BUILTIN_FUNC) {
                for (int i = 0; i < node->as.CallExpr.arguments.count; i++) {
                    compileNode(compiler, node->as.CallExpr.arguments.start[i]);
                }

                int idx = ((Builtin*)callable.node)->builtinIdx;

                addOp(compiler, CALL_BUILTIN);
                ADD_INT(idx);

                break;
            }

            addOp(compiler, CALL_SUB);
            for (int i = 0; i < node->as.CallExpr.arguments.count; i++) {
                if (callable.node->as.SubroutineStmt.parameters.start[i]->as.Parameter.byref) {
                    name = extractNullTerminatedString(node->as.CallExpr.arguments.start[i]->as.VariableExpr.name->start, node->as.CallExpr.arguments.start[i]->as.VariableExpr.name->length);
                    Symbol symbol;
                    res = findSymbol(compiler, name, &symbol);
                    if (!res) {
                        printf("Something went wrong. This shouldn't be able to happen.\n");
                        break;
                    }
                    free(name);

                    addOp(compiler, LOAD_INT);
                    ADD_INT(symbol.pos);

                    if (symbol.isRelative) {
                        addOp(compiler, RGET_REF);
                    } else {
                        addOp(compiler, GET_REF);
                    }
                } else {
                    compileNode(compiler, node->as.CallExpr.arguments.start[i]);
                }
            }

            addOp(compiler, DO_CALL);
            ADD_4BYTE(callable.pos);

            break;
        }
        case EXPR_GROUP: {
            compileNode(compiler, node->as.GroupExpr.subExpr);
            break;
        }
        case EXPR_VARIABLE:{
            char* name = extractNullTerminatedString(node->as.VariableExpr.name->start, node->as.VariableExpr.name->length);
            Symbol var;
            bool res = findSymbol(compiler, name, &var);
            if (!res) {
                printf("Something went wrong. This shouldn't be able to happen.");
                break;
            }
            free(name);

            addOp(compiler, LOAD_INT);
            ADD_INT(var.pos);

            if (node->as.VariableExpr.assigned) {
                switch (var.type) {
                    case SYMBOL_VAR:
                        if (var.byref) {
                            if (var.isRelative) {
                                addOp(compiler, RFETCH_REF);
                            } else {
                                addOp(compiler, FETCH_REF);
                            }
                            break;
                        } else {
                            switch (var.node->as.VarDeclareStmt.type) {
                                case TYPE_INTEGER:
                                    addOp(compiler, var.isRelative ? RSTORE_INT : STORE_INT);
                                    break;
                                case TYPE_REAL:
                                    addOp(compiler, var.isRelative ? RSTORE_REAL : STORE_REAL);
                                    break;
                                case TYPE_CHAR:
                                    addOp(compiler, var.isRelative ? RSTORE_CHAR : STORE_CHAR);
                                    break;
                                case TYPE_BOOLEAN:
                                    addOp(compiler, var.isRelative ? RSTORE_BOOL : STORE_BOOL);
                                    break;
                                case TYPE_STRING:
                                case TYPE_ARRAY:
                                    addOp(compiler, var.isRelative ? RSTORE_REF : STORE_REF);
                                    break;
                                default: break;
                            }
                        }
                        break;
                    case SYMBOL_PARAM:
                        if (var.byref) {
                            if (var.isRelative) {
                                addOp(compiler, RFETCH_REF);
                            } else {
                                addOp(compiler, FETCH_REF);
                            }
                            break;
                        } else if (var.node->as.Parameter.isArray) {
                            addOp(compiler, var.isRelative ? RSTORE_REF : STORE_REF);
                        } else {
                            switch (var.node->as.Parameter.type) {
                                case TYPE_INTEGER:
                                    addOp(compiler, var.isRelative ? RSTORE_INT : STORE_INT);
                                    break;
                                case TYPE_REAL:
                                    addOp(compiler, var.isRelative ? RSTORE_REAL : STORE_REAL);
                                    break;
                                case TYPE_CHAR:
                                    addOp(compiler, var.isRelative ? RSTORE_CHAR : STORE_CHAR);
                                    break;
                                case TYPE_BOOLEAN:
                                    addOp(compiler, var.isRelative ? RSTORE_BOOL : STORE_BOOL);
                                    break;
                                case TYPE_STRING:
                                case TYPE_ARRAY:
                                    addOp(compiler, var.isRelative ? RSTORE_REF : STORE_REF);
                                    break;
                                default: break;
                            }
                        }
                        break;
                    case SYMBOL_FOR_COUNTER:
                        addOp(compiler, var.isRelative ? RSTORE_INT : STORE_INT);
                        break;
                    case SYMBOL_ARRAY:
                        addOp(compiler, var.isRelative ? RSTORE_REF : STORE_REF);break;
                    default: break;
                }

                if (var.byref) {
                    switch (var.type) {
                        case SYMBOL_VAR:
                            switch (var.node->as.VarDeclareStmt.type) {
                                case TYPE_INTEGER:
                                    addOp(compiler, STORE_REF_INT);
                                    break;
                                case TYPE_REAL:
                                    addOp(compiler, STORE_REF_REAL);
                                    break;
                                case TYPE_CHAR:
                                    addOp(compiler, STORE_REF_CHAR);
                                    break;
                                case TYPE_BOOLEAN:
                                    addOp(compiler, STORE_REF_BOOL);
                                    break;
                                default: break;
                            }
                            break;
                        case SYMBOL_PARAM:
                            switch (var.node->as.Parameter.type) {
                                case TYPE_INTEGER:
                                    addOp(compiler, STORE_REF_INT);
                                    break;
                                case TYPE_REAL:
                                    addOp(compiler, STORE_REF_REAL);
                                    break;
                                case TYPE_CHAR:
                                    addOp(compiler, STORE_REF_CHAR);
                                    break;
                                case TYPE_BOOLEAN:
                                    addOp(compiler, STORE_REF_BOOL);
                                    break;
                                default: break;
                            }
                        default:break;
                    }
                }
                break;
            }

            switch (var.type) {
                case SYMBOL_FILE: {
                    if (var.isRelative) {
                        addOp(compiler, RFETCH_REF);
                    } else {
                        addOp(compiler, FETCH_REF);
                    }
                    break;
                }
                case SYMBOL_VAR:
                    if (var.byref) {
                        if (var.isRelative) {
                            addOp(compiler, RFETCH_REF);
                        } else {
                            addOp(compiler, FETCH_REF);
                        }
                        break;
                    } else {
                        switch (var.node->as.VarDeclareStmt.type) {
                            case TYPE_INTEGER:
                                addOp(compiler, var.isRelative ? RFETCH_INT : FETCH_INT);
                                break;
                            case TYPE_REAL:
                                addOp(compiler, var.isRelative ? RFETCH_REAL : FETCH_REAL);
                                break;
                            case TYPE_CHAR:
                                addOp(compiler, var.isRelative ? RFETCH_CHAR : FETCH_CHAR);
                                break;
                            case TYPE_BOOLEAN:
                                addOp(compiler, var.isRelative ? RFETCH_BOOL : FETCH_BOOL);
                                break;
                            case TYPE_STRING:
                            case TYPE_ARRAY:
                                addOp(compiler, var.isRelative ? RFETCH_REF : FETCH_REF);
                                break;
                            default: break;
                        }
                    }
                    break;
                case SYMBOL_PARAM:
                    if (var.byref) {
                        if (var.isRelative) {
                            addOp(compiler, RFETCH_REF);
                        } else {
                            addOp(compiler, FETCH_REF);
                        }
                        break;
                    } else if (var.node->as.Parameter.isArray) {
                        addOp(compiler, var.isRelative ? RFETCH_REF : FETCH_REF);
                    } else {
                        switch (var.node->as.Parameter.type) {
                            case TYPE_INTEGER:
                                addOp(compiler, var.isRelative ? RFETCH_INT : FETCH_INT);
                                break;
                            case TYPE_REAL:
                                addOp(compiler, var.isRelative ? RFETCH_REAL : FETCH_REAL);
                                break;
                            case TYPE_CHAR:
                                addOp(compiler, var.isRelative ? RFETCH_CHAR : FETCH_CHAR);
                                break;
                            case TYPE_BOOLEAN:
                                addOp(compiler, var.isRelative ? RFETCH_BOOL : FETCH_BOOL);
                                break;
                            case TYPE_STRING:
                            case TYPE_ARRAY:
                                addOp(compiler, var.isRelative ? RFETCH_REF : FETCH_REF);
                                break;
                            default: break;
                        }
                    }
                    break;
                case SYMBOL_CONST:
                    switch (var.node->as.ConstDeclareStmt.type) {
                        case TYPE_INTEGER:
                            addOp(compiler, var.isRelative ? RFETCH_INT : FETCH_INT);
                            break;
                        case TYPE_REAL:
                            addOp(compiler, var.isRelative ? RFETCH_REAL : FETCH_REAL);
                            break;
                        case TYPE_CHAR:
                            addOp(compiler, var.isRelative ? RFETCH_CHAR : FETCH_CHAR);
                            break;
                        case TYPE_BOOLEAN:
                            addOp(compiler, var.isRelative ? RFETCH_BOOL : FETCH_BOOL);
                            break;
                        case TYPE_STRING:
                            addOp(compiler, var.isRelative ? RFETCH_REF : FETCH_REF);
                            break;
                        default: break;
                    }
                    break;
                case SYMBOL_FOR_COUNTER:
                    addOp(compiler, var.isRelative ? RFETCH_INT : FETCH_INT);
                    break;
                case SYMBOL_ARRAY:
                    addOp(compiler, var.isRelative ? RFETCH_REF : FETCH_REF);break;
                default: break;
            }

            if (var.byref) {
                switch (var.type) {
                    case SYMBOL_VAR:
                        switch (var.node->as.VarDeclareStmt.type) {
                            case TYPE_INTEGER:
                                addOp(compiler, FETCH_REF_INT);
                                break;
                            case TYPE_REAL:
                                addOp(compiler, FETCH_REF_REAL);
                                break;
                            case TYPE_CHAR:
                                addOp(compiler, FETCH_REF_CHAR);
                                break;
                            case TYPE_BOOLEAN:
                                addOp(compiler, FETCH_REF_BOOL);
                                break;
                            default: break;
                        }
                    case SYMBOL_PARAM:
                        switch (var.node->as.Parameter.type) {
                            case TYPE_INTEGER:
                                addOp(compiler, FETCH_REF_INT);
                                break;
                            case TYPE_REAL:
                                addOp(compiler, FETCH_REF_REAL);
                                break;
                            case TYPE_CHAR:
                                addOp(compiler, FETCH_REF_CHAR);
                                break;
                            case TYPE_BOOLEAN:
                                addOp(compiler, FETCH_REF_BOOL);
                                break;
                            default: break;
                        }
                    default:break;
                }
            }
            break;
        }
        case EXPR_ARRAY_ACCESS: {
            char* name = extractNullTerminatedString(node->as.ArrayAccessExpr.name->start, node->as.ArrayAccessExpr.name->length);
            Symbol array;
            bool res = findSymbol(compiler, name, &array);
            if (!res) {
                printf("This shouldn't be able to happen.\n");
                break;
            }

            free(name);

            addOp(compiler, LOAD_INT);
            ADD_INT(array.pos);

            if (array.isRelative) {
                addOp(compiler, RFETCH_REF);
            } else {
                addOp(compiler, FETCH_REF);
            }

            compileNode(compiler, node->as.ArrayAccessExpr.indices[0]);
            compileNode(compiler, node->as.ArrayAccessExpr.indices[1]);
            if (node->as.ArrayAccessExpr.indices[1] == NULL) {
                int zero = 0;
                addOp(compiler, LOAD_INT);
                ADD_INT(zero);
            }

            if (node->as.ArrayAccessExpr.assigned) {
                addOp(compiler, STORE_ARRAY_ELEM);
            } else {
                addOp(compiler, FETCH_ARRAY_ELEM);
            }

            break;
        }
        case EXPR_UNARY: {
            compileNode(compiler, node->as.UnaryExpr.right);
            switch (node->as.UnaryExpr.op) {
                case UNARY_NOT:
                    addOp(compiler, NOT);
                    break;
                case UNARY_NEG:
                    if (node->as.UnaryExpr.resultType == TYPE_INTEGER) {
                        addOp(compiler, NEG_INT);
                    } else if (node->as.UnaryExpr.resultType == TYPE_REAL) {
                        addOp(compiler, NEG_REAL);
                    } else {
                        printf("This should be inaccessible.\n");
                    }
                    break;
                default: break;
            }
            break;
        }
        case EXPR_BINARY: {
            compileNode(compiler, node->as.BinaryExpr.left);
            if (node->as.BinaryExpr.leftType == TYPE_INTEGER && node->as.BinaryExpr.rightType == TYPE_REAL) {
                addOp(compiler, CAST_INT_REAL);
            } else if (node->as.BinaryExpr.leftType == TYPE_CHAR) {
                addOp(compiler, CAST_CHAR_INT);
            }
            compileNode(compiler, node->as.BinaryExpr.right);
            if (node->as.BinaryExpr.leftType == TYPE_REAL && node->as.BinaryExpr.rightType == TYPE_INTEGER) {
                addOp(compiler, CAST_INT_REAL);
            } else if (node->as.BinaryExpr.rightType == TYPE_CHAR) {
                addOp(compiler, CAST_CHAR_INT);
            }

            DataType type = node->as.BinaryExpr.leftType;
            if (type == TYPE_INTEGER && node->as.BinaryExpr.rightType == TYPE_REAL) {
                type = TYPE_REAL;
            } else if (type == TYPE_CHAR) {
                type = TYPE_INTEGER;
            }

            switch (node->as.BinaryExpr.op) {
                case BIN_CONCAT:
                    addOp(compiler, CONCAT);
                    break;
                case LOGIC_EQUAL: {
                    switch (type) {
                        case TYPE_INTEGER:
                            addOp(compiler, EQ_INT);
                            break;
                        case TYPE_REAL:
                            addOp(compiler, EQ_REAL);
                            break;
                        case TYPE_BOOLEAN:
                            addOp(compiler, EQ_BOOL);
                            break;
                        case TYPE_STRING:
                            addOp(compiler, EQ_STRING);
                            break;
                        case TYPE_ARRAY:
                            addOp(compiler, EQ_REF);
                            break;
                        default:
                            break;
                    }
                    break;
                }
                case LOGIC_NOT_EQUAL: {
                    switch (type) {
                        case TYPE_INTEGER:
                            addOp(compiler, NEQ_INT);
                            break;
                        case TYPE_REAL:
                            addOp(compiler, NEQ_REAL);
                            break;
                        case TYPE_BOOLEAN:
                            addOp(compiler, NEQ_BOOL);
                            break;
                        case TYPE_STRING:
                            addOp(compiler, NEQ_STRING);
                            break;
                        case TYPE_ARRAY:
                            addOp(compiler, NEQ_REF);
                            break;
                        default:
                            break;
                    }
                    break;
                }
                case LOGIC_LESS: {
                    switch (type) {
                        case TYPE_INTEGER:
                            addOp(compiler, LESS_INT);
                            break;
                        case TYPE_REAL:
                            addOp(compiler, LESS_REAL);
                            break;
                        case TYPE_BOOLEAN:
                            addOp(compiler, LESS_BOOL);
                            break;
                        case TYPE_STRING:
                            addOp(compiler, LESS_STRING);
                            break;
                        case TYPE_ARRAY:
                            addOp(compiler, LESS_REF);
                            break;
                        default:
                            break;
                    }
                    break;
                }
                case LOGIC_LESS_EQUAL: {
                    switch (type) {
                        case TYPE_INTEGER:
                            addOp(compiler, LESS_EQ_INT);
                            break;
                        case TYPE_REAL:
                            addOp(compiler, LESS_EQ_REAL);
                            break;
                        case TYPE_BOOLEAN:
                            addOp(compiler, LESS_EQ_BOOL);
                            break;
                        case TYPE_STRING:
                            addOp(compiler, LESS_EQ_STRING);
                            break;
                        case TYPE_ARRAY:
                            addOp(compiler, LESS_EQ_REF);
                            break;
                        default:
                            break;
                    }
                    break;
                }
                case LOGIC_GREATER: {
                    switch (type) {
                        case TYPE_INTEGER:
                            addOp(compiler, GREATER_INT);
                            break;
                        case TYPE_REAL:
                            addOp(compiler, GREATER_REAL);
                            break;
                        case TYPE_BOOLEAN:
                            addOp(compiler, GREATER_BOOL);
                            break;
                        case TYPE_STRING:
                            addOp(compiler, GREATER_STRING);
                            break;
                        case TYPE_ARRAY:
                            addOp(compiler, GREATER_REF);
                            break;
                        default:
                            break;
                    }
                    break;
                }
                case LOGIC_GREATER_EQUAL: {
                    switch (type) {
                        case TYPE_INTEGER:
                            addOp(compiler, GREATER_EQ_INT);
                            break;
                        case TYPE_REAL:
                            addOp(compiler, GREATER_EQ_REAL);
                            break;
                        case TYPE_BOOLEAN:
                            addOp(compiler, GREATER_EQ_BOOL);
                            break;
                        case TYPE_STRING:
                            addOp(compiler, GREATER_EQ_STRING);
                            break;
                        case TYPE_ARRAY:
                            addOp(compiler, GREATER_EQ_REF);
                            break;
                        default:
                            break;
                    }
                    break;
                }
                case LOGIC_OR: {
                    addOp(compiler, OR);
                    break;
                }
                case LOGIC_AND: {
                    addOp(compiler, AND);
                    break;
                }
                case BIN_ADD: {
                    addOp(compiler, type == TYPE_REAL ? ADD_REAL : ADD_INT);
                    break;
                }
                case BIN_MINUS: {
                    addOp(compiler, type == TYPE_REAL ? MINUS_REAL : MINUS_INT);
                    break;
                }
                case BIN_MULT: {
                    addOp(compiler, type == TYPE_REAL ? MULT_REAL : MULT_INT);
                    break;
                }
                case BIN_DIV: {
                    addOp(compiler, type == TYPE_REAL ? DIV_REAL : DIV_INT);
                    break;
                }
                case BIN_MOD: {
                    addOp(compiler, type == TYPE_REAL ? MOD_REAL : MOD_INT);
                    break;
                }
                case BIN_FDIV: {
                    addOp(compiler, type == TYPE_REAL ? FDIV_REAL : FDIV_INT);
                    break;
                }
                case BIN_POWER: {
                    addOp(compiler, type == TYPE_REAL ? POW_REAL : POW_INT);
                }
                default: break;
            }
            break;
        }
        case EXPR_ASSIGN: {
            compileNode(compiler, node->as.AssignmentExpr.right);
            compileNode(compiler, node->as.AssignmentExpr.left);
            break;
        }
        case STMT_BLOCK: {
            for (int i = 0; i < node->as.BlockStmt.body.count; i++) {
                compileNode(compiler, node->as.BlockStmt.body.start[i]);
            }
            break;
        }
        case STMT_EXPR: {
            compileNode(compiler, node->as.ExprStmt.expr);

            switch (node->as.ExprStmt.resultType) {
                case TYPE_INTEGER:
                    addOp(compiler, POP_4B);
                    break;
                case TYPE_REAL:
                case TYPE_STRING:
                case TYPE_ARRAY:
                    addOp(compiler, POP_8B);
                    break;
                case TYPE_BOOLEAN:
                case TYPE_CHAR:
                    addOp(compiler, POP_1B);
                    break;
                default: break;
            }
            break;
        }
        case STMT_SUBROUTINE: {
            char* name = extractNullTerminatedString(node->as.SubroutineStmt.name->start, node->as.SubroutineStmt.name->length);
            addOp(compiler, BRANCH);
            int branchPos = getNextPos(compiler->bStream);
            int zero = 0;
            ADD_INT(zero);

            addSubroutineSymbol(compiler, name, node, getNextPos(compiler->bStream));
            initialiseSymbol(compiler, name);

            createScope(compiler, node->as.SubroutineStmt.subroutineType == TYPE_FUNCTION ? SCOPE_FUNCTION : SCOPE_PROCEDURE);

            for (int i = 0; i < node->as.SubroutineStmt.parameters.count; i++) {
                compileNode(compiler, node->as.SubroutineStmt.parameters.start[i]);
            }

            compileNode(compiler, node->as.SubroutineStmt.body);

            if (node->as.SubroutineStmt.subroutineType == TYPE_PROCEDURE) {
                addOp(compiler, RETURN_NIL);
            }

            int jumpPos = getNextPos(compiler->bStream);
            insertAtPos(compiler->bStream, (jumpPos >> 24) & 0xff, branchPos);
            insertAtPos(compiler->bStream, (jumpPos >> 16) & 0xff, branchPos + 1);
            insertAtPos(compiler->bStream, (jumpPos >> 8) & 0xff, branchPos + 2);
            insertAtPos(compiler->bStream, (jumpPos) & 0xff, branchPos + 3);

            endScope(compiler);

            break;
        }
        case STMT_IF: {
            compileNode(compiler, node->as.IfStmt.condition);
            addOp(compiler, B_FALSE);
            int zero = 0;
            int elseJumpPos = getNextPos(compiler->bStream);
            ADD_INT(zero);

            compileNode(compiler, node->as.IfStmt.thenBranch);

            addOp(compiler, BRANCH);
            int endThenJumpPos = getNextPos(compiler->bStream);
            ADD_INT(zero);
            int elseJumpTargetPos = getNextPos(compiler->bStream);
            insertAtPos(compiler->bStream, (elseJumpTargetPos >> 24) & 0xff, elseJumpPos);
            insertAtPos(compiler->bStream, (elseJumpTargetPos >> 16) & 0xff, elseJumpPos + 1);
            insertAtPos(compiler->bStream, (elseJumpTargetPos >> 8) & 0xff, elseJumpPos + 2);
            insertAtPos(compiler->bStream, (elseJumpTargetPos) & 0xff, elseJumpPos + 3);

            if (node->as.IfStmt.elseBranch != NULL) {
                compileNode(compiler, node->as.IfStmt.elseBranch);
            }
            int endThenTargetPos = getNextPos(compiler->bStream);
            insertAtPos(compiler->bStream, (endThenTargetPos >> 24) & 0xff, endThenJumpPos);
            insertAtPos(compiler->bStream, (endThenTargetPos >> 16) & 0xff, endThenJumpPos + 1);
            insertAtPos(compiler->bStream, (endThenTargetPos >> 8) & 0xff, endThenJumpPos + 2);
            insertAtPos(compiler->bStream, (endThenTargetPos) & 0xff, endThenJumpPos + 3);

            break;
        }
        case STMT_OUTPUT: {
            for (int i = 0; i < node->as.OutputStmt.expressions.count; i++) {
                compileNode(compiler, node->as.OutputStmt.expressions.start[i]);

                switch (node->as.OutputStmt.expressions.start[i]->as.Expr.resultType) {
                    case TYPE_INTEGER:
                        addOp(compiler, OUTPUT_INT);
                        break;
                    case TYPE_REAL:
                        addOp(compiler, OUTPUT_REAL);
                        break;
                    case TYPE_CHAR:
                        addOp(compiler, OUTPUT_CHAR);
                        break;
                    case TYPE_BOOLEAN:
                        addOp(compiler, OUTPUT_BOOL);
                        break;
                    case TYPE_ARRAY:
                        addOp(compiler, OUTPUT_REF);
                        break;
                    case TYPE_STRING:
                        addOp(compiler, OUTPUT_STRING);
                        break;
                    default: break;
                }
            }

            addOp(compiler, OUTPUT_NL);

            break;
        }
        case STMT_INPUT: {
            /*char* name = extractNullTerminatedString(node->as.InputStmt.name->start, node->as.InputStmt.name->length);
            Symbol var;
            bool res = findSymbol(compiler, name, &var);

            if (!res) {
                printf("This should not be able to happen.\n");
                break;
            }

            free(name);*/

            DataType type = node->as.InputStmt.expectedType;

            switch (type) {
                case TYPE_INTEGER:
                    addOp(compiler, INPUT_INT);
                    break;
                case TYPE_REAL:
                    addOp(compiler, INPUT_REAL);
                    break;
                case TYPE_CHAR:
                    addOp(compiler, INPUT_CHAR);
                    break;
                case TYPE_BOOLEAN:
                    addOp(compiler, INPUT_BOOL);
                    break;
                case TYPE_STRING:
                    addOp(compiler, INPUT_STRING);
                    break;
                default: break;
            }

            compileNode(compiler, node->as.InputStmt.varAccess);

            /*addOp(compiler, LOAD_INT);
            ADD_INT(var.pos);

            if (var.byref) {
                if (var.isRelative) {
                    addOp(compiler, RFETCH_REF);
                } else {
                    addOp(compiler, FETCH_REF);
                }

                switch (type) {
                    case TYPE_INTEGER:
                        addOp(compiler, STORE_REF_INT);
                        break;
                    case TYPE_REAL:
                        addOp(compiler, STORE_REF_REAL);
                        break;
                    case TYPE_CHAR:
                        addOp(compiler, STORE_REF_CHAR);
                        break;
                    case TYPE_BOOLEAN:
                        addOp(compiler, STORE_REF_BOOL);
                        break;
                    default: break;
                }
            } else {
                switch (type) {
                    case TYPE_INTEGER:
                        addOp(compiler, var.isRelative ? RSTORE_INT : STORE_INT);
                        break;
                    case TYPE_REAL:
                        addOp(compiler, var.isRelative ? RSTORE_REAL : STORE_REAL);
                        break;
                    case TYPE_CHAR:
                        addOp(compiler, var.isRelative ? RSTORE_CHAR : STORE_CHAR);
                        break;
                    case TYPE_BOOLEAN:
                        addOp(compiler, var.isRelative ? RSTORE_BOOL : STORE_BOOL);
                        break;
                    case TYPE_STRING:
                        addOp(compiler, var.isRelative ? RSTORE_REF : STORE_REF);
                        break;
                    default: break;
                }
            }*/

            switch (node->as.InputStmt.expectedType) {
                case TYPE_INTEGER:
                    addOp(compiler, POP_4B);
                    break;
                case TYPE_REAL:
                case TYPE_STRING:
                case TYPE_ARRAY:
                    addOp(compiler, POP_8B);
                    break;
                case TYPE_CHAR:
                case TYPE_BOOLEAN:
                    addOp(compiler, POP_1B);
                    break;
                default: break;
            }
            break;
        }
        case STMT_RETURN: {
            compileNode(compiler, node->as.ReturnStmt.expr);

            addOp(compiler, RETURN);

            int size = 0;
            switch (node->as.ReturnStmt.returnType) {
                case TYPE_INTEGER:
                    size = 4;
                    break;
                case TYPE_REAL:
                case TYPE_STRING:
                case TYPE_ARRAY:
                    size = 8;
                    break;
                case TYPE_BOOLEAN:
                case TYPE_CHAR:
                    size = 1;
                    break;
                default: break;
            }

            ADD_BYTE(size);

            break;
        }
        case STMT_WHILE: {
            int condStartPos = getNextPos(compiler->bStream);
            compileNode(compiler, node->as.WhileStmt.condition);
            addOp(compiler, B_FALSE);
            int zero = 0;
            int falseJump = getNextPos(compiler->bStream);
            ADD_INT(zero);
            compileNode(compiler, node->as.WhileStmt.body);
            addOp(compiler, BRANCH);
            ADD_INT(condStartPos);
            int endPos = getNextPos(compiler->bStream);

            insertAtPos(compiler->bStream, (endPos >> 24) & 0xff, falseJump);
            insertAtPos(compiler->bStream, (endPos >> 16) & 0xff, falseJump + 1);
            insertAtPos(compiler->bStream, (endPos >> 8) & 0xff, falseJump + 2);
            insertAtPos(compiler->bStream, (endPos) & 0xff, falseJump + 3);

            break;
        }
        case STMT_VAR_DECLARE: {
            char* name = extractNullTerminatedString(node->as.VarDeclareStmt.name->start, node->as.VarDeclareStmt.name->length);

            int size = 0;
            int zero = 0;
            switch (node->as.VarDeclareStmt.type) {
                case TYPE_INTEGER:
                    size = 4;
                    addOp(compiler, LOAD_INT);
                    ADD_INT(zero);
                    break;
                case TYPE_REAL:
                case TYPE_STRING:
                case TYPE_ARRAY:
                    addOp(compiler, LOAD_REAL);
                    ADD_REAL(zero);
                    size = 8;
                    break;
                case TYPE_BOOLEAN:
                case TYPE_CHAR:
                    addOp(compiler, LOAD_CHAR);
                    ADD_CHAR(zero);
                    size = 1;
                    break;
                default: break;
            }

            addSymbol(compiler, name, node, SYMBOL_VAR, compiler->depth > 0, false, size);

            free(name);

            break;
        }
        case STMT_CONST_DECLARE: {
            char* name = extractNullTerminatedString(node->as.ConstDeclareStmt.name->start, node->as.ConstDeclareStmt.name->length);

            int size = 0;
            switch (node->as.ConstDeclareStmt.type) {
                case TYPE_INTEGER:
                    size = 4;
                    break;
                case TYPE_REAL:
                case TYPE_STRING:
                case TYPE_ARRAY:
                    size = 8;
                    break;
                case TYPE_BOOLEAN:
                case TYPE_CHAR:
                    size = 1;
                    break;
                default: break;
            }

            addSymbol(compiler, name, node, SYMBOL_CONST, compiler->depth > 0, false, size);

            free(name);

            switch (node->as.ConstDeclareStmt.type) {
                case TYPE_INTEGER: {
                    char *numStr = extractNullTerminatedString(node->as.ConstDeclareStmt.value->start,
                                                               node->as.ConstDeclareStmt.value->length);
                    int n = atoi(numStr);
                    free(numStr);

                    addOp(compiler, LOAD_INT);
                    ADD_INT(n);

                    /*addOp(compiler, LOAD_INT);
                    int pos = compiler->symbolTable->nextPos - size;
                    ADD_INT(pos);

                    if (compiler->depth > 0) {
                        addOp(compiler, RSTORE_INT);
                    } else {
                        addOp(compiler, STORE_INT);
                    }*/
                    break;
                }
                case TYPE_REAL: {
                    char *numStr = extractNullTerminatedString(node->as.ConstDeclareStmt.value->start,
                                                               node->as.ConstDeclareStmt.value->length);
                    double n = strtod(numStr, NULL);
                    free(numStr);

                    addOp(compiler, LOAD_REAL);
                    ADD_REAL(n);

                    /*addOp(compiler, LOAD_INT);
                    int pos = compiler->symbolTable->nextPos - size;
                    ADD_INT(pos);

                    if (compiler->depth > 0) {
                        addOp(compiler, RSTORE_REAL);
                    } else {
                        addOp(compiler, STORE_REAL);
                    }*/
                    break;
                }
                case TYPE_STRING: {
                    addOp(compiler, LOAD_STRING);
                    ADD_INT(node->as.LiteralExpr.value->length - 2);
                    for (int i = 1; i < node->as.LiteralExpr.value->length - 1; i++) {
                        ADD_CHAR(node->as.LiteralExpr.value->start[i]);
                    }

                    /*addOp(compiler, LOAD_INT);
                    int pos = compiler->symbolTable->nextPos - size;
                    ADD_INT(pos);

                    if (compiler->depth > 0) {
                        addOp(compiler, RSTORE_REF);
                    } else {
                        addOp(compiler, STORE_REF);
                    }*/
                    break;
                }
                case TYPE_ARRAY: break;
                case TYPE_BOOLEAN: {
                    bool res = false;
                    if (node->as.LiteralExpr.value->start[0] == 'T') {
                        res = true;
                    }

                    addOp(compiler, LOAD_BOOL);
                    ADD_BOOL(res);

                    /*addOp(compiler, LOAD_INT);
                    int pos = compiler->symbolTable->nextPos - size;
                    ADD_INT(pos);

                    if (compiler->depth > 0) {
                        addOp(compiler, RSTORE_BOOL);
                    } else {
                        addOp(compiler, STORE_BOOL);
                    }*/

                    break;
                }
                case TYPE_CHAR: {
                    addOp(compiler, LOAD_CHAR);
                    ADD_CHAR(node->as.LiteralExpr.value->start[1]);

                    /*addOp(compiler, LOAD_INT);
                    int pos = compiler->symbolTable->nextPos - size;
                    ADD_INT(pos);

                    if (compiler->depth > 0) {
                        addOp(compiler, RSTORE_CHAR);
                    } else {
                        addOp(compiler, STORE_CHAR);
                    }*/

                    break;
                }
                default: break;
            }

            /*switch (node->as.ConstDeclareStmt.type) {
                case TYPE_INTEGER:
                    addOp(compiler, POP_4B);
                    break;
                case TYPE_REAL:
                case TYPE_STRING:
                case TYPE_ARRAY:
                    addOp(compiler, POP_8B);
                    break;
                case TYPE_CHAR:
                case TYPE_BOOLEAN:
                    addOp(compiler, POP_1B);
                    break;
                default: break;
            }*/

            break;
        }
        case STMT_ARRAY_DECLARE: {
            char* name = extractNullTerminatedString(node->as.ArrayDeclareStmt.name->start, node->as.ArrayDeclareStmt.name->length);

            addSymbol(compiler, name, node, SYMBOL_ARRAY, compiler->depth > 0, false, 8);

            int pos = compiler->symbolTable->nextPos - 8;
            int zero = 0;

            for (int i = 0; i < 4; i++) {
                ASTNode* dim = node->as.ArrayDeclareStmt.dimensions[i];
                if (dim == NULL) {
                    addOp(compiler, LOAD_INT);
                    ADD_INT(zero);
                } else {
                    compileNode(compiler, dim);
                }
            }

            int size = 0;

            switch (node->as.ArrayDeclareStmt.type) {
                case TYPE_INTEGER:
                    size = 4;
                    break;
                case TYPE_REAL:
                case TYPE_STRING:
                case TYPE_ARRAY:
                    size = 8;
                    break;
                case TYPE_BOOLEAN:
                case TYPE_CHAR:
                    size = 1;
                    break;
                default: break;
            }
            addOp(compiler, LOAD_INT);
            ADD_INT(size);

            addOp(compiler, CREATE_ARRAY);

            /*addOp(compiler, LOAD_INT);
            ADD_INT(pos);

            if (compiler->depth > 0) {
                addOp(compiler, RSTORE_REF);
            } else {
                addOp(compiler, STORE_REF);
            }
            addOp(compiler, POP_8B);*/
            break;
        }
        case STMT_CASE: {
            compiler->lastCaseJumpPos = -1;

            compileNode(compiler, node->as.CaseStmt.expr);
            if (node->as.CaseStmt.exprType == TYPE_CHAR) {
                addOp(compiler, CAST_CHAR_INT);
            }

            compileNode(compiler, node->as.CaseStmt.body);

            compiler->lastCaseJumpPos = -1;
            break;
        }
        case STMT_REPEAT: {
            int first = getNextPos(compiler->bStream);

            compileNode(compiler, node->as.RepeatStmt.body);

            compileNode(compiler, node->as.RepeatStmt.condition);

            addOp(compiler, B_FALSE);
            ADD_INT(first);

            break;
        }
        case STMT_FOR: {
            char* name = extractNullTerminatedString(node->as.ForStmt.counterName->start, node->as.ForStmt.counterName->length);
            Symbol counter;
            bool res = findSymbol(compiler, name, &counter);

            int pos;
            bool isRel;
            bool byref;

            SymbolTable symbolTable;
            initTable(&symbolTable);
            copyOverTable(compiler->symbolTable, &symbolTable);

            if (res) {
                pos = counter.pos;
                isRel = counter.isRelative;
                byref = counter.byref;
            } else {
                pos = compiler->symbolTable->nextPos;
                isRel = compiler->depth > 0;
                byref = false;
                addSymbol(compiler, name, node, SYMBOL_FOR_COUNTER,isRel, byref, 4);
                int zero = 0;
                addOp(compiler, LOAD_INT);
                ADD_INT(zero);
            }

            compileNode(compiler, node->as.ForStmt.init);
            addOp(compiler, LOAD_INT);
            ADD_INT(pos);
            if (byref) {
                if (isRel) {
                    addOp(compiler, RFETCH_REF);
                } else {
                    addOp(compiler, FETCH_REF);
                }

                addOp(compiler, STORE_REF_INT);
            } else {
                if (isRel) {
                    addOp(compiler,  RSTORE_INT);
                } else {
                    addOp(compiler, STORE_INT);
                }
            }
            addOp(compiler, POP_4B);

            int step = 1;

            if (node->as.ForStmt.step != NULL) {
                char* stepStr = extractNullTerminatedString(node->as.ForStmt.step->as.LiteralExpr.value->start, node->as.ForStmt.step->as.LiteralExpr.value->length);
                step = atoi(stepStr);
                free(stepStr);
            }

            //

            int condStartPos = getNextPos(compiler->bStream);

            addOp(compiler, LOAD_INT);
            ADD_INT(pos);
            if (byref) {
                if (isRel) {
                    addOp(compiler, RFETCH_REF);
                } else {
                    addOp(compiler, FETCH_REF);
                }

                addOp(compiler, FETCH_REF_INT);
            } else {
                if (isRel) {
                    addOp(compiler,  RFETCH_INT);
                } else {
                    addOp(compiler, FETCH_INT);
                }
            }

            compileNode(compiler, node->as.ForStmt.end);

            if (step < 0) {
                addOp(compiler, GREATER_EQ_INT);
            } else {
                addOp(compiler, LESS_EQ_INT);
            }

            addOp(compiler, B_FALSE);
            int zero = 0;
            int falseJump = getNextPos(compiler->bStream);
            ADD_INT(zero);

            compileNode(compiler, node->as.ForStmt.body);

            addOp(compiler, LOAD_INT);
            ADD_INT(pos);
            if (byref) {
                if (isRel) {
                    addOp(compiler, RFETCH_REF);
                } else {
                    addOp(compiler, FETCH_REF);
                }

                addOp(compiler, FETCH_REF_INT);
            } else {
                if (isRel) {
                    addOp(compiler,  RFETCH_INT);
                } else {
                    addOp(compiler, FETCH_INT);
                }
            }

            addOp(compiler, LOAD_INT);
            ADD_INT(step);

            addOp(compiler, ADD_INT);

            addOp(compiler, LOAD_INT);
            ADD_INT(pos);
            if (byref) {
                if (isRel) {
                    addOp(compiler, RFETCH_REF);
                } else {
                    addOp(compiler, FETCH_REF);
                }

                addOp(compiler, STORE_REF_INT);
            } else {
                if (isRel) {
                    addOp(compiler,  RSTORE_INT);
                } else {
                    addOp(compiler, STORE_INT);
                }
            }

            addOp(compiler, POP_4B);

            addOp(compiler, BRANCH);
            ADD_INT(condStartPos);
            int endPos = getNextPos(compiler->bStream);

            insertAtPos(compiler->bStream, (endPos >> 24) & 0xff, falseJump);
            insertAtPos(compiler->bStream, (endPos >> 16) & 0xff, falseJump + 1);
            insertAtPos(compiler->bStream, (endPos >> 8) & 0xff, falseJump + 2);
            insertAtPos(compiler->bStream, (endPos) & 0xff, falseJump + 3);
            //

            if (!res) {
                addOp(compiler, POP_4B);
            }
            clearTable(compiler->symbolTable);
            copyOverTable(&symbolTable, compiler->symbolTable);
            freeTable(&symbolTable);
            free(name);
            break;
        }
        case STMT_CALL: {
            char* name = extractNullTerminatedString(node->as.CallStmt.name->start, node->as.CallStmt.name->length);
            Symbol callable;
            bool res = findSymbol(compiler, name, &callable);
            if (!res) {
                printf("Something went wrong. This shouldn't be able to happen.\n");
                break;
            }
            free(name);
            addOp(compiler, CALL_SUB);
            for (int i = 0; i < node->as.CallStmt.arguments.count; i++) {
                if (callable.node->as.SubroutineStmt.parameters.start[i]->as.Parameter.byref) {
                    name = extractNullTerminatedString(node->as.CallStmt.arguments.start[i]->as.VariableExpr.name->start, node->as.CallStmt.arguments.start[i]->as.VariableExpr.name->length);
                    Symbol symbol;
                    res = findSymbol(compiler, name, &symbol);
                    if (!res) {
                        printf("Something went wrong. This shouldn't be able to happen.\n");
                        break;
                    }
                    free(name);

                    addOp(compiler, LOAD_INT);
                    ADD_INT(symbol.pos);

                    if (symbol.isRelative) {
                        addOp(compiler, RGET_REF);
                    } else {
                        addOp(compiler, GET_REF);
                    }
                } else {
                    compileNode(compiler, node->as.CallStmt.arguments.start[i]);
                }
            }

            addOp(compiler, DO_CALL);
            ADD_4BYTE(callable.pos);

            break;
        }
        case STMT_CASE_LINE: {
            if (node->as.CaseLineStmt.value == NULL) {
                addOp(compiler, POP_4B);
                compileNode(compiler, node->as.CaseLineStmt.result);

                if (compiler->lastCaseJumpPos >= 0) {
                    int pos = getNextPos(compiler->bStream);
                    insertAtPos(compiler->bStream, (pos >> 24) & 0xff, compiler->lastCaseJumpPos);
                    insertAtPos(compiler->bStream, (pos >> 16) & 0xff, compiler->lastCaseJumpPos + 1);
                    insertAtPos(compiler->bStream, (pos >> 8) & 0xff, compiler->lastCaseJumpPos + 2);
                    insertAtPos(compiler->bStream, (pos) & 0xff, compiler->lastCaseJumpPos + 3);
                }
            } else {
                addOp(compiler, COPY_INT);
                compileNode(compiler, node->as.CaseLineStmt.value);
                if (node->as.CaseLineStmt.value->as.Expr.resultType == TYPE_CHAR) {
                    addOp(compiler, CAST_CHAR_INT);
                }

                addOp(compiler, EQ_INT);

                addOp(compiler, B_FALSE);
                int falseJumpPos = getNextPos(compiler->bStream);
                int zero = 0;
                ADD_INT(zero);

                addOp(compiler, POP_4B);
                compileNode(compiler, node->as.CaseLineStmt.result);

                if (compiler->lastCaseJumpPos >= 0) {
                    int pos = getNextPos(compiler->bStream);
                    insertAtPos(compiler->bStream, (pos >> 24) & 0xff, compiler->lastCaseJumpPos);
                    insertAtPos(compiler->bStream, (pos >> 16) & 0xff, compiler->lastCaseJumpPos + 1);
                    insertAtPos(compiler->bStream, (pos >> 8) & 0xff, compiler->lastCaseJumpPos + 2);
                    insertAtPos(compiler->bStream, (pos) & 0xff, compiler->lastCaseJumpPos + 3);
                }

                addOp(compiler, BRANCH);

                compiler->lastCaseJumpPos = getNextPos(compiler->bStream);
                ADD_INT(zero);

                int pos = getNextPos(compiler->bStream);
                insertAtPos(compiler->bStream, (pos >> 24) & 0xff, falseJumpPos);
                insertAtPos(compiler->bStream, (pos >> 16) & 0xff, falseJumpPos + 1);
                insertAtPos(compiler->bStream, (pos >> 8) & 0xff, falseJumpPos + 2);
                insertAtPos(compiler->bStream, (pos) & 0xff, falseJumpPos + 3);
            }
            break;
        }
        case STMT_CASE_BLOCK: {
            compileNode(compiler, node->as.CaseBlockStmt.body);
            break;
        }
        case STMT_OPENFILE: {
            char* filename = extractNullTerminatedString(node->as.OpenfileStmt.filename->start, node->as.OpenfileStmt.filename->length);

            addFile(compiler, filename, node, SYMBOL_FILE, compiler->depth > 0, false, node->as.OpenfileStmt.accessType);

            free(filename);

            addOp(compiler, LOAD_STRING);
            ADD_INT(node->as.OpenfileStmt.filename->length - 2);
            for (int i = 1; i < node->as.OpenfileStmt.filename->length - 1; i++) {
                ADD_CHAR(node->as.OpenfileStmt.filename->start[i]);
            }

            addOp(compiler, LOAD_INT);
            ADD_INT(node->as.OpenfileStmt.accessType);

            addOp(compiler, OPENFILE);

            break;
        }
        case STMT_CLOSEFILE: {
            char* filename = extractNullTerminatedString(node->as.ClosefileStmt.filename->start, node->as.ClosefileStmt.filename->length);

            Symbol file;
            int res = findSymbol(compiler, filename, &file);
            if (!res) {
                printf("Something went wrong. This shouldn't be able to happen.\n");
                break;
            }
            free(filename);

            addOp(compiler, LOAD_INT);
            ADD_INT(file.pos);

            if (file.isRelative) {
                addOp(compiler, RFETCH_REF);
            } else {
                addOp(compiler, FETCH_REF);
            }

            deleteTable(compiler->symbolTable, filename);

            addOp(compiler, CLOSEFILE);

            break;
        }
        case STMT_READFILE: {
            char* filename = extractNullTerminatedString(node->as.ReadfileStmt.filename->start, node->as.ReadfileStmt.filename->length);

            Symbol file;
            int res = findSymbol(compiler, filename, &file);
            if (!res) {
                printf("Something went wrong. This shouldn't be able to happen.\n");
                break;
            }
            free(filename);

            addOp(compiler, LOAD_INT);
            ADD_INT(file.pos);

            if (file.isRelative) {
                addOp(compiler, RFETCH_REF);
            } else {
                addOp(compiler, FETCH_REF);
            }

            addOp(compiler, READ_LINE);
            char* name;

            if (node->as.ReadfileStmt.varAccess->type == EXPR_VARIABLE) {
                name = extractNullTerminatedString(node->as.ReadfileStmt.varAccess->as.VariableExpr.name->start, node->as.ReadfileStmt.varAccess->as.VariableExpr.name->length);
            } else if (node->as.InputStmt.varAccess->type == EXPR_ARRAY_ACCESS) {
                name = extractNullTerminatedString(node->as.ReadfileStmt.varAccess->as.ArrayAccessExpr.name->start, node->as.ReadfileStmt.varAccess->as.ArrayAccessExpr.name->length);
            } else {
                printf("This shouldn't be able to happen.\n");
                break;
            }

            Symbol var;
            res = findSymbol(compiler, name, &var);
            if (!res) {
                printf("This shouldn't be able to happen.\n");
                break;
            }

            addOp(compiler, LOAD_INT);
            ADD_INT(var.pos);

            if (var.isRelative) {
                addOp(compiler, RSTORE_REF);
            } else {
                addOp(compiler, STORE_REF);
            }

            free(name);

            break;
        }
        case STMT_WRITEFILE: {
            char* filename = extractNullTerminatedString(node->as.WritefileStmt.filename->start, node->as.WritefileStmt.filename->length);

            Symbol file;
            int res = findSymbol(compiler, filename, &file);
            if (!res) {
                printf("Something went wrong. This shouldn't be able to happen.\n");
                break;
            }
            free(filename);


            for (int i = 0; i < node->as.WritefileStmt.expressions.count; i++) {
                compileNode(compiler, node->as.WritefileStmt.expressions.start[i]);
                addOp(compiler, LOAD_INT);
                ADD_INT(file.pos);

                if (file.isRelative) {
                    addOp(compiler, RFETCH_REF);
                } else {
                    addOp(compiler, FETCH_REF);
                }

                switch (node->as.WritefileStmt.expressions.start[i]->as.Expr.resultType) {
                    case TYPE_INTEGER:
                        addOp(compiler, WRITE_INT);
                        break;
                    case TYPE_REAL:
                        addOp(compiler, WRITE_REAL);
                        break;
                    case TYPE_CHAR:
                        addOp(compiler, WRITE_CHAR);
                        break;
                    case TYPE_BOOLEAN:
                        addOp(compiler, WRITE_BOOL);
                        break;
                    case TYPE_ARRAY:
                        addOp(compiler, WRITE_REF);
                        break;
                    case TYPE_STRING:
                        addOp(compiler, WRITE_STRING);
                        break;
                    default: break;
                }
            }

            addOp(compiler, LOAD_INT);
            ADD_INT(file.pos);

            if (file.isRelative) {
                addOp(compiler, RFETCH_REF);
            } else {
                addOp(compiler, FETCH_REF);
            }

            addOp(compiler, WRITE_NL);

            break;
        }
        case STMT_PROGRAM: {
            for (int i = 0; i < node->as.ProgramStmt.body.count; i++) {
                compileNode(compiler, node->as.ProgramStmt.body.start[i]);
            }
            addOp(compiler, EXIT);
            break;
        }
        case AST_PARAMETER: {
            bool byref = node->as.Parameter.byref;

            int size = 0;

            if (node->as.Parameter.isArray || node->as.Parameter.byref) {
                size = 8;
            } else {
                switch (node->as.Parameter.type) {
                    case TYPE_INTEGER:
                        size = 4;
                        break;
                    case TYPE_REAL:
                    case TYPE_ARRAY:
                    case TYPE_STRING:
                        size = 8;
                        break;
                    case TYPE_CHAR:
                    case TYPE_BOOLEAN:
                        size = 1;
                        break;
                    default: break;
                }
            }

            char* name = extractNullTerminatedString(node->as.Parameter.name->start, node->as.Parameter.name->length);
            addSymbol(compiler, name, node, SYMBOL_PARAM, true, byref, size);

            free(name);

            break;
        }
        default: break;
    }
}

void initCompiler(Compiler* compiler, BytecodeStream* bStream) {
    compiler->symbolTable = malloc(sizeof(SymbolTable));
    initTable(compiler->symbolTable);
    compiler->globalTable = compiler->symbolTable;
    compiler->depth = 0;
    compiler->bStream = bStream;
    compiler->stackPos = 0;
    compiler->lastCaseJumpPos = -1;
}

void freeCompiler(Compiler* compiler) {
    freeTable(compiler->symbolTable);
    freeTable(compiler->globalTable);
}

bool compile(Compiler* compiler, AST* ast) {
    ASTNode* program = ast->program;

    //
    Builtin substring;
    createBuiltin(&substring, 3, TYPE_STRING, 0);
    addParamDatatype(&substring, TYPE_STRING, 0);
    addParamDatatype(&substring, TYPE_INTEGER, 1);
    addParamDatatype(&substring, TYPE_INTEGER, 2);
    addSymbol(compiler, "SUBSTRING", (ASTNode*)&substring, SYMBOL_BUILTIN_FUNC, false, false, 0);

    Builtin length;
    createBuiltin(&length, 1, TYPE_INTEGER, 1);
    addParamDatatype(&length, TYPE_STRING, 0);
    addSymbol(compiler, "LENGTH", (ASTNode*)&length, SYMBOL_BUILTIN_FUNC, false, false, 0);

    Builtin lcase;
    createBuiltin(&lcase, 1, TYPE_STRING, 2);
    addParamDatatype(&lcase, TYPE_STRING, 0);
    addSymbol(compiler, "LCASE", (ASTNode*)&lcase, SYMBOL_BUILTIN_FUNC, false, false, 0);

    Builtin ucase;
    createBuiltin(&ucase, 1, TYPE_STRING, 3);
    addParamDatatype(&ucase, TYPE_STRING, 0);
    addSymbol(compiler, "UCASE", (ASTNode*)&lcase, SYMBOL_BUILTIN_FUNC, false, false, 0);

    Builtin randomBetween;
    createBuiltin(&randomBetween, 2, TYPE_INTEGER, 4);
    addParamDatatype(&randomBetween, TYPE_INTEGER, 0);
    addParamDatatype(&randomBetween, TYPE_INTEGER, 1);
    addSymbol(compiler, "RANDOMBETWEEN", (ASTNode*)&randomBetween, SYMBOL_BUILTIN_FUNC, false, false, 0);

    Builtin rnd;
    createBuiltin(&rnd, 0, TYPE_REAL, 5);
    addSymbol(compiler, "RND", (ASTNode*)&rnd, SYMBOL_BUILTIN_FUNC, false, false, 0);

    Builtin integer;
    createBuiltin(&integer, 1, TYPE_INTEGER, 6);
    addParamDatatype(&integer, TYPE_REAL, 0);
    addSymbol(compiler, "INT", (ASTNode*)&integer, SYMBOL_BUILTIN_FUNC, false, false, 0);

    Builtin eof;
    createBuiltin(&eof, 1, TYPE_BOOLEAN, 7);
    addParamDatatype(&eof, TYPE_STRING, 0);
    addSymbol(compiler, "EOF", (ASTNode*)&eof, SYMBOL_BUILTIN_FUNC, false, false, 0);

    Builtin charAt;
    createBuiltin(&charAt, 2, TYPE_CHAR, 8);
    addParamDatatype(&charAt, TYPE_STRING, 0);
    addParamDatatype(&charAt, TYPE_INTEGER, 1);
    addSymbol(compiler, "CHARAT", (ASTNode*)&charAt, SYMBOL_BUILTIN_FUNC, false, false, 0);
    //

    initBytecodeStream(compiler->bStream);

    compileNode(compiler, program);

    return true;
}

void printCompileResult(Compiler* compiler) {
    printBytestream(compiler->bStream);
}