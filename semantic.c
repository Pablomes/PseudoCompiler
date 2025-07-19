
#include "common.h"
#include "semantic.h"


static void semanticErrorMessage(ASTNode* node, const char* message) {
    switch (node->type) {
        // EXPRESSIONS
        case EXPR_BINARY:
            fprintf(stderr, "binary expression:\n");
            break;
        case EXPR_ASSIGN:
            fprintf(stderr, "assignment expression:\n");
            break;
        case EXPR_CALL:
            fprintf(stderr, "call expression:\n");
            break;
        case EXPR_GET:
            fprintf(stderr, "get expression:\n");
            break;
        case EXPR_GROUP:
            fprintf(stderr, "group expression:\n");
            break;
        case EXPR_LITERAL:
            fprintf(stderr, "literal expression:\n");
            break;
        case EXPR_LOGICAL:
            fprintf(stderr, "logical expression:\n");
            break;
        case EXPR_SET:
            fprintf(stderr, "set expression:\n");
            break;
        case EXPR_SUPER:
            fprintf(stderr, "super expression:\n");
            break;
        case EXPR_THIS:
            fprintf(stderr, "this expression:\n");
            break;
        case EXPR_UNARY:
            fprintf(stderr, "unary expression:\n");
            break;
        case EXPR_VARIABLE:
            fprintf(stderr, "variable expression:\n");
            break;
        case EXPR_ARRAY_ACCESS:
            fprintf(stderr, "array access expression:\n");
            break;

            // STATEMENTS
        case STMT_BLOCK:
            fprintf(stderr, "block statement:\n");
            break;
        case STMT_EXPR:
            fprintf(stderr, "expression statement:\n");
            break;
        case STMT_SUBROUTINE:
            if (node->as.SubroutineStmt.subroutineType == TYPE_FUNCTION) {
                fprintf(stderr, "FUNCTION statement:\n");
            } else {
                fprintf(stderr, "PROCEDURE statement:\n");
            }
            break;
        case STMT_IF:
            fprintf(stderr, "IF statement:\n");
            break;
        case STMT_OUTPUT:
            fprintf(stderr, "OUTPUT statement:\n");
            break;
        case STMT_INPUT:
            fprintf(stderr, "INPUT statement:\n");
            break;
        case STMT_RETURN:
            fprintf(stderr, "RETURN statement:\n");
            break;
        case STMT_WHILE:
            fprintf(stderr, "WHILE statement:\n");
            break;
        case STMT_VAR_DECLARE:
            fprintf(stderr, "variable declaration statement:\n");
            break;
        case STMT_CONST_DECLARE:
            fprintf(stderr, "constant declaration statement:\n");
            break;
        case STMT_ARRAY_DECLARE:
            fprintf(stderr, "array declaration statement:\n");
            break;
        case STMT_CASE:
            fprintf(stderr, "CASE statement:\n");
            break;
        case STMT_REPEAT:
            fprintf(stderr, "REPEAT-UNTIL statement:\n");
            break;
        case STMT_CALL:
            fprintf(stderr, "CALL statement:\n");
            break;
        case STMT_CASE_BLOCK:
            fprintf(stderr, "CASE block statement:\n");
            break;
        case STMT_PROGRAM:
            fprintf(stderr, "program statement:\n");
            break;
        case STMT_FOR:
            fprintf(stderr, "FOR statement:\n");
            break;
        case STMT_CASE_LINE:
            fprintf(stderr, "CASE line statement:\n");
            break;

            // MISCELLANEOUS
        case AST_PARAMETER:
            fprintf(stderr, "AST parameter:\n");
            break;

        default:
            fprintf(stderr, "syntax node referring to source:\n");
            break;
    }

    fprintf(stderr, "%.*s\n%s", (int)node->length, node->start, message);
}

static void semanticError(Analyser* analyser, ASTNode* node, const char* message) {
    if (node == NULL) return;
    analyser->hadError = true;

    fprintf(stderr, "\n[line: %d, col: %d] Error in ", node->line, node->col);

    semanticErrorMessage(node, message);
}

static void semanticWarning(ASTNode* node, const char* message) {
    if (node == NULL) return;

    fprintf(stderr, "\n[line: %d, col: %d] Warning in ", node->line, node->col);

    semanticErrorMessage(node, message);
}

static bool findSymbol(Analyser* analyser, const char* key, Symbol* symbol) {
    /*SymbolTable* curr = analyser->symbolTable;
    while (curr != NULL) {
        bool res = getTable(curr, key, symbol);
        if (res) return true;

        curr = curr->enclosing;
    }*/

    return getTable(analyser->symbolTable, key, symbol);
}

static bool findSymbolInCurrScope(Analyser* analyser, const char* key, Symbol* symbol) {
    return getCurrTable(analyser->symbolTable, key, symbol);
}

static bool initialiseSymbol(Analyser* analyser, const char* key) {
    Symbol* symbol = getTablePointer(analyser->symbolTable, key);

    if (symbol == NULL) return false;

    symbol->initialised = true;

    return true;
}

static bool addSymbol(Analyser* analyser, const char* key, ASTNode* node, SymbolType type) {
    return setTable(analyser->symbolTable, key, node, type, 0, false, false);
}

static bool addFile(Analyser* analyser, const char* key, ASTNode* node, SymbolType type, FileAccessType access) {
    bool res = setTableFile(analyser->symbolTable, key, node, type, 0, false, false, access);
    if (res) analyser->symbolTable->filesOpened++;

    return res;
}

static bool removeFile(Analyser* analyser, const char* key) {
    bool res = deleteTable(analyser->symbolTable, key);
    if (res) analyser->symbolTable->filesOpened--;
    return res;
}

static bool createScope(Analyser* analyser, ScopeType type) {
    SymbolTable* newTable = malloc(sizeof(SymbolTable));
    if (newTable == NULL) return false;

    initTable(newTable);
    newTable->scopeType = type;
    newTable->enclosing = analyser->symbolTable;
    analyser->symbolTable = newTable;
    analyser->depth++;
    return true;
}

static bool endScope(Analyser* analyser) {
    SymbolTable* end = analyser->symbolTable;
    if (end == analyser->globalTable) return false;
    analyser->symbolTable = analyser->symbolTable->enclosing;
    end->enclosing = NULL;
    freeTable(end);
    analyser->depth--;
    return true;
}

static bool staticSymbolCheck(Analyser* analyser, ASTNode* program) {
    bool valid = true;

    for (int i = 0; i < program->as.ProgramStmt.body.count; i++) {
        ASTNode* curr = program->as.ProgramStmt.body.start[i];

        if (curr == NULL) continue;

        if (curr->type == STMT_SUBROUTINE) {
#define SUBROUTINE curr->as.SubroutineStmt
            SymbolType type = SYMBOL_NONE;

            if (SUBROUTINE.subroutineType == TYPE_FUNCTION) {
                type = SYMBOL_FUNC;
            } else if (SUBROUTINE.subroutineType == TYPE_PROCEDURE) {
                type = SYMBOL_PROC;
            } else {
                valid = false;
            }

            char* key = extractNullTerminatedString(SUBROUTINE.name->start, SUBROUTINE.name->length);

            valid &= setTable(analyser->symbolTable, key, curr, type, 0, false, false);

            valid &= initialiseSymbol(analyser, key);

            free(key);
#undef SUBROUTINE
        }
    }

    return valid;
}

static DataType getArrayBaseType(Analyser* analyser, ASTNode* node, ASTNodeType type) {
    switch (type) {
        case EXPR_VARIABLE: {
            char* name = extractNullTerminatedString(node->as.VariableExpr.name->start, node->as.VariableExpr.name->length);
            Symbol sym;
            bool res = findSymbol(analyser, name, &sym);
            if (!res) {
                return TYPE_ERROR;
            }

            free(name);

            return sym.node->as.ArrayDeclareStmt.type;
        }
        case EXPR_GROUP:
            return getArrayBaseType(analyser, node->as.GroupExpr.subExpr, node->as.GroupExpr.subExpr->type);
        case EXPR_ASSIGN:
            return getArrayBaseType(analyser, node->as.AssignmentExpr.right, node->as.AssignmentExpr.right->type);
        default:
            return TYPE_ERROR;
    }
}

static DataType semanticCheck(Analyser* analyser, ASTNode* node) {
    if (node == NULL) { return TYPE_NONE; }

    switch (node->type) {
        case EXPR_LITERAL:
            return node->as.LiteralExpr.resultType;
        case EXPR_CALL: {
            char* name = extractNullTerminatedString(node->as.CallExpr.name->start, node->as.CallExpr.name->length);
            Symbol callable;
            bool res = findSymbol(analyser, name, &callable);
            if (!res) {
                semanticError(analyser, node, "Callable symbol ");
                fprintf(stderr, "'%s' not in scope.", name);
                return TYPE_ERROR;
            }
            if (callable.type != SYMBOL_FUNC && callable.type != SYMBOL_BUILTIN_FUNC) {
                semanticError(analyser, node, "Expected function in call expression, but got other.");
                return TYPE_ERROR;
            }

            if (callable.type == SYMBOL_BUILTIN_FUNC) {
                if (node->as.CallExpr.arguments.count != ((Builtin*)callable.node)->numParams) {
                    semanticError(analyser, node, "Expected ");
                    fprintf(stderr, "%d arguments as per definition but got %d.", callable.node->as.SubroutineStmt.parameters.count,
                            node->as.CallExpr.arguments.count);
                    return TYPE_ERROR;
                }

                free(name);

                if (((Builtin*)callable.node)->builtinIdx == 7) { // EOF()
                    if (node->as.CallExpr.arguments.start[0]->type != EXPR_LITERAL && semanticCheck(analyser, node->as.CallExpr.arguments.start[0]) != TYPE_STRING) {
                        semanticError(analyser, node, "Argument number 1 is not correct type.");
                        return TYPE_ERROR;
                    }

                    node->as.CallExpr.arguments.start[0]->type = EXPR_VARIABLE;
                    node->as.CallExpr.arguments.start[0]->as.VariableExpr.assigned = false;

                    if (semanticCheck(analyser, node->as.CallExpr.arguments.start[0]) != TYPE_FILE) {
                        semanticError(analyser, node, "Not a valid file path.");
                        return TYPE_ERROR;
                    }

                    node->as.CallExpr.resultType = ((Builtin*)callable.node)->returnType;

                    return TYPE_BOOLEAN;
                }

                for (int i = 0; i < node->as.CallExpr.arguments.count; i++) {
                    DataType type = semanticCheck(analyser, node->as.CallExpr.arguments.start[i]);
                    if (type == TYPE_ERROR) continue;
                    if (type != ((Builtin*)callable.node)->parameterTypes[i]) {
                        semanticError(analyser, node, "Argument number ");
                        fprintf(stderr, "%d is not correct type.", i + 1);
                        return TYPE_ERROR;
                    }
                }

                node->as.CallExpr.resultType = ((Builtin*)callable.node)->returnType;

                return node->as.CallExpr.resultType;
            }

            if (!callable.initialised) {
                semanticError(analyser, node, "Symbol ");
                fprintf(stderr, "'%s' is not initialised previously, and therefore, cannot be used.", name);
                return TYPE_ERROR;
            }
            if (callable.node->as.SubroutineStmt.parameters.count != node->as.CallExpr.arguments.count) {
                semanticError(analyser, node, "Expected ");
                fprintf(stderr, "%d arguments as per definition but got %d.", callable.node->as.SubroutineStmt.parameters.count,
                        node->as.CallExpr.arguments.count);
                return TYPE_ERROR;
            }

            free(name);

            for (int i = 0; i < node->as.CallExpr.arguments.count; i++) {
                DataType type = semanticCheck(analyser, node->as.CallExpr.arguments.start[i]);
                if (type == TYPE_ERROR) continue;
                if (type != callable.node->as.SubroutineStmt.parameters.start[i]->as.Parameter.type && !(type == TYPE_ARRAY && callable.node->as.SubroutineStmt.parameters.start[i]->as.Parameter.isArray)) {
                    semanticError(analyser, node, "Argument number ");
                    fprintf(stderr, "%d is not correct type.", i + 1);
                    return TYPE_ERROR;
                }

                if (callable.node->as.SubroutineStmt.parameters.start[i]->as.Parameter.byref && node->as.CallExpr.arguments.start[i]->type != EXPR_VARIABLE) {
                    semanticError(analyser, node, "Function expects reference to a variable, so argument must be a variable expression.\n");
                    return TYPE_ERROR;
                }

                if (type == TYPE_ARRAY) {
                    DataType baseType = getArrayBaseType(analyser, node->as.CallExpr.arguments.start[i], node->as.CallExpr.arguments.start[i]->type);
                    if (baseType != callable.node->as.SubroutineStmt.parameters.start[i]->as.Parameter.type) {
                        semanticError(analyser, node, "Argument number ");
                        fprintf(stderr, "%d is an array but not of the correct type.", i + 1);
                        return TYPE_ERROR;
                    }
                }
            }

            node->as.CallExpr.resultType = callable.node->as.SubroutineStmt.returnValue;
            return node->as.CallExpr.resultType;
        }
        case EXPR_GROUP:
            node->as.GroupExpr.resultType = semanticCheck(analyser, node->as.GroupExpr.subExpr);
            return node->as.GroupExpr.resultType;
        case EXPR_VARIABLE: {
            node->as.VariableExpr.assigned = analyser->assigning;
            char* name = extractNullTerminatedString(node->as.VariableExpr.name->start, node->as.VariableExpr.name->length);
            Symbol var;
            bool res = findSymbol(analyser, name, &var);
            if (!res) {
                semanticError(analyser, node, "Symbol ");
                fprintf(stderr, "%s not in scope.", name);
                return TYPE_ERROR;
            }
            if (var.type != SYMBOL_VAR && var.type != SYMBOL_PARAM && var.type != SYMBOL_CONST && var.type != SYMBOL_FOR_COUNTER && var.type != SYMBOL_ARRAY && var.type != SYMBOL_FILE) {
                semanticError(analyser, node, "Expect variable. Subroutines are NOT first class.");
                return TYPE_ERROR;
            }

            if (var.type == SYMBOL_CONST && analyser->assigning) {
                semanticError(analyser, node, "Can't assign to constant ");
                fprintf(stderr, "'%s'.", name);
                return TYPE_ERROR;
            }

            if (var.type == SYMBOL_FOR_COUNTER && analyser->assigning) {
                semanticError(analyser, node, "Can't assign to FOR loop counter ");
                fprintf(stderr, "'%s'.", name);
                return TYPE_ERROR;
            }

            if (analyser->assigning) {
                initialiseSymbol(analyser, name);
                var.initialised = true;
            }

            if (!var.initialised) {
                semanticError(analyser, node, "Symbol ");
                fprintf(stderr, "'%s' is not initialised previously and therefore cannot be used.", name);
                return TYPE_ERROR;
            }

            free(name);

            if (var.type == SYMBOL_VAR) {
                node->as.VariableExpr.resultType = var.node->as.VarDeclareStmt.type;
            } else if (var.type == SYMBOL_PARAM) {
                if (var.node->as.Parameter.isArray) {
                    node->as.VariableExpr.resultType = TYPE_ARRAY;
                } else {
                    node->as.VariableExpr.resultType = var.node->as.Parameter.type;
                }
            } else if (var.type == SYMBOL_CONST) {
                node->as.VariableExpr.resultType = var.node->as.ConstDeclareStmt.type;
            } else if (var.type == SYMBOL_FOR_COUNTER) {
                node->as.VariableExpr.resultType = TYPE_INTEGER;
            } else if (var.type == SYMBOL_FILE) {
                node->as.VariableExpr.resultType = TYPE_FILE;
            } else {
                node->as.VariableExpr.resultType = TYPE_ARRAY;
            }

            return node->as.VariableExpr.resultType;
        }
        case EXPR_ARRAY_ACCESS: {
            node->as.ArrayAccessExpr.assigned = analyser->assigning;
            char* name = extractNullTerminatedString(node->as.ArrayAccessExpr.name->start, node->as.ArrayAccessExpr.name->length);
            Symbol array;
            bool res = findSymbol(analyser, name, &array);
            if (!res) {
                semanticError(analyser, node, "Array ");
                fprintf(stderr, "%s is not in scope.", name);
                return TYPE_ERROR;
            }
            if (array.type != SYMBOL_ARRAY && !(array.type == SYMBOL_PARAM && array.node->as.Parameter.isArray)) {
                semanticError(analyser, node, "Expected array. Other symbols are NOT indexable.");
                return TYPE_ERROR;
            }
            free(name);
            bool wasAssigning = analyser->assigning;
            analyser->assigning = false;
            DataType index1 = semanticCheck(analyser, node->as.ArrayAccessExpr.indices[0]);
            DataType index2 = semanticCheck(analyser, node->as.ArrayAccessExpr.indices[1]);
            analyser->assigning = wasAssigning;

            if (index1 != TYPE_INTEGER && index1 != TYPE_ERROR) {
                semanticError(analyser, node, "First index should be an INTEGER value.");
                return TYPE_ERROR;
            }

            bool is2D = false;

            if (array.type == SYMBOL_ARRAY) {
                is2D = array.node->as.ArrayDeclareStmt.is2D;
            } else {
                is2D = array.node->as.Parameter.is2D;
            }

            if ((index2 != TYPE_NONE && index2 != TYPE_ERROR) ^ is2D) {
                semanticError(analyser, node, "Wrong array dimensions. Expected ");
                if (array.node->as.ArrayDeclareStmt.is2D) {
                    fprintf(stderr, "two indices but got one.");
                } else {
                    fprintf(stderr, "one index but got two.");
                }
                return TYPE_ERROR;
            }

            if (index2 != TYPE_NONE && index2 != TYPE_INTEGER && index2 != TYPE_ERROR) {
                semanticError(analyser, node, "Second index should be an INTEGER value.");
                return TYPE_ERROR;
            }

            if (index2 == TYPE_NONE) {
                node->as.ArrayAccessExpr.indices[1] = 0;
            }

            node->as.ArrayAccessExpr.resultType = array.node->as.ArrayDeclareStmt.type;
            return node->as.ArrayAccessExpr.resultType;
        }
        case EXPR_UNARY: {
            DataType subType = semanticCheck(analyser, node->as.UnaryExpr.right);
            if (subType == TYPE_ERROR) return subType;
            if (node->as.UnaryExpr.op == UNARY_NOT) {
                if (subType != TYPE_BOOLEAN) {
                    semanticError(analyser, node, "NOT unary operator expects BOOLEAN expression.");
                    return TYPE_ERROR;
                }
            } else {
                if (subType != TYPE_REAL && subType != TYPE_INTEGER) {
                    semanticError(analyser, node, "Operator expects either INTEGER or REAL expression.");
                    return TYPE_ERROR;
                }
            }
            node->as.UnaryExpr.resultType = subType;
            return node->as.UnaryExpr.resultType;
        }
        case EXPR_BINARY: {
            DataType leftType = semanticCheck(analyser, node->as.BinaryExpr.left);
            DataType rightType = semanticCheck(analyser, node->as.BinaryExpr.right);
            node->as.BinaryExpr.leftType = leftType;
            node->as.BinaryExpr.rightType = rightType;

            if (leftType == TYPE_ERROR || rightType == TYPE_ERROR) {
                return TYPE_ERROR;
            }

            if (node->as.BinaryExpr.op == BIN_CONCAT) {
                if (leftType != TYPE_STRING || rightType != TYPE_STRING) {
                    semanticError(analyser, node, "Concatenation operator '&' expects two string operands.");
                    return TYPE_ERROR;
                }
                node->as.BinaryExpr.resultType = TYPE_STRING;
            } else if (node->as.BinaryExpr.op == LOGIC_EQUAL || node->as.BinaryExpr.op == LOGIC_NOT_EQUAL ||
                       node->as.BinaryExpr.op == LOGIC_LESS || node->as.BinaryExpr.op == LOGIC_LESS_EQUAL ||
                       node->as.BinaryExpr.op == LOGIC_GREATER || node->as.BinaryExpr.op == LOGIC_GREATER_EQUAL) {

                if (leftType != rightType && !(leftType == TYPE_REAL && rightType == TYPE_INTEGER) && !(leftType == TYPE_INTEGER && rightType == TYPE_REAL)) {
                    semanticWarning(node, "Operands are of different types. This comparison always results in FALSE.");
                }

                node->as.BinaryExpr.resultType = TYPE_BOOLEAN;

            } else if (node->as.BinaryExpr.op == LOGIC_AND || node->as.BinaryExpr.op == LOGIC_OR) {
                if (leftType != TYPE_BOOLEAN || rightType != TYPE_BOOLEAN) {
                    semanticError(analyser, node, "Logical operators require BOOLEAN operands.");
                    return TYPE_ERROR;
                }
                node->as.BinaryExpr.resultType = TYPE_BOOLEAN;
            } else {
                if ((leftType != TYPE_INTEGER && leftType != TYPE_REAL) || (rightType != TYPE_INTEGER && rightType != TYPE_REAL)) {
                    semanticError(analyser, node, "Arithmetic binary operations expect two INTEGER or REAL operands.");
                    return TYPE_ERROR;
                }

                if (leftType == TYPE_REAL || rightType == TYPE_REAL || node->as.BinaryExpr.op == BIN_DIV || node->as.BinaryExpr.op == BIN_POWER) {
                    node->as.BinaryExpr.resultType = TYPE_REAL;
                } else {
                    node->as.BinaryExpr.resultType = TYPE_INTEGER;
                }

                if (node->as.BinaryExpr.op == BIN_FDIV) {
                    node->as.BinaryExpr.resultType = TYPE_INTEGER;
                }
            }

            return node->as.BinaryExpr.resultType;
        }
        case EXPR_ASSIGN: {
            analyser->assigning = true;

            DataType leftType = semanticCheck(analyser, node->as.AssignmentExpr.left);

            analyser->assigning = false;

            DataType rightType = semanticCheck(analyser, node->as.AssignmentExpr.right);

            if (leftType == TYPE_ERROR || rightType == TYPE_ERROR) {
                return TYPE_ERROR;
            }

            if (leftType != rightType && !(leftType == TYPE_REAL && rightType == TYPE_INTEGER)) {
                semanticError(analyser, node, "Assignment expression expects both sides of '<-' to be the same type.");
                return TYPE_ERROR;
            }

            node->as.AssignmentExpr.resultType = rightType;
            return node->as.AssignmentExpr.resultType;
        }
        case STMT_BLOCK: {
            for (int i = 0; i < node->as.BlockStmt.body.count; i++) {
                semanticCheck(analyser, node->as.BlockStmt.body.start[i]);
            }
            return TYPE_NONE;
        }
        case STMT_EXPR:
            node->as.ExprStmt.resultType = semanticCheck(analyser, node->as.ExprStmt.expr);

            if (node->as.ExprStmt.resultType == TYPE_ERROR) {
                return TYPE_ERROR;
            }

            return TYPE_NONE;
        case STMT_SUBROUTINE: {
            if (analyser->depth != 0) {
                semanticError(analyser, node, "Subroutines are only allowed at top level as they live in global namespace");
                return TYPE_ERROR;
            }

            ASTNode* prevFunc = analyser->currentFunction;
            analyser->currentFunction = NULL;
            char* name = extractNullTerminatedString(node->as.SubroutineStmt.name->start, node->as.SubroutineStmt.name->length);
            Symbol func;
            bool res = findSymbol(analyser, name, &func);
            if (res) {
                semanticError(analyser, node, "Symbol ");
                fprintf(stderr, "'%s' already exists.", name);
                return TYPE_ERROR;
            }

            SymbolType type;
            ScopeType scopeType = SCOPE_GLOBAL;
            if (node->as.SubroutineStmt.subroutineType == TYPE_FUNCTION) {
                scopeType = SCOPE_FUNCTION;
                type = SYMBOL_FUNC;
                analyser->currentFunction = node;
            } else {
                scopeType = SCOPE_PROCEDURE;
                type = SYMBOL_PROC;
            }

            addSymbol(analyser, name, node, type);
            initialiseSymbol(analyser, name);

            analyser->hasReturned = false;
            createScope(analyser, scopeType);

            for (int i = 0; i < node->as.SubroutineStmt.parameters.count; i++) {
                semanticCheck(analyser, node->as.SubroutineStmt.parameters.start[i]); // ADDS TO SCOPE
            }

            semanticCheck(analyser, node->as.SubroutineStmt.body);

            if (analyser->symbolTable->filesOpened > 0) {
                semanticWarning(node, "Not all file streams have been closed properly in this scope.");
            }
            endScope(analyser);

            analyser->currentFunction = prevFunc;

            if (!analyser->hasReturned && type == SYMBOL_FUNC) {
                semanticError(analyser, node, "Not all FUNCTION paths RETURN a value.");
                return TYPE_ERROR;
            }

            analyser->hasReturned = false;

            return TYPE_NONE;
        }
        case STMT_IF: {
            DataType conditionType = semanticCheck(analyser, node->as.IfStmt.condition);

            if (conditionType != TYPE_BOOLEAN) {
                semanticError(analyser, node, "IF statement condition must result in BOOLEAN.");
                return TYPE_ERROR;
            }

            bool origHasReturned = analyser->hasReturned;
            analyser->hasReturned = false;

            createScope(analyser, SCOPE_CONDITIONAL);

            semanticCheck(analyser, node->as.IfStmt.thenBranch);

            if (analyser->symbolTable->filesOpened > 0) {
                semanticWarning(node, "Not all file streams have been closed properly in this scope.");
            }
            endScope(analyser);

            bool thenReturns = analyser->hasReturned;

            bool elseReturns = false;
            if (node->as.IfStmt.elseBranch != NULL) {
                analyser->hasReturned = false;
                createScope(analyser, SCOPE_CONDITIONAL);
                semanticCheck(analyser, node->as.IfStmt.elseBranch);
                if (analyser->symbolTable->filesOpened > 0) {
                    semanticWarning(node, "Not all file streams have been closed properly in this scope.");
                }
                endScope(analyser);
                elseReturns = analyser->hasReturned;
            }

            analyser->hasReturned = origHasReturned;

            if (thenReturns && elseReturns) {
                analyser->hasReturned = true;
            }

            return TYPE_NONE;
        }
        case STMT_OUTPUT: {
            for (int i = 0; i < node->as.OutputStmt.expressions.count; i++) {
                semanticCheck(analyser, node->as.OutputStmt.expressions.start[i]);
            }

            return TYPE_NONE;
        }
        case STMT_INPUT: {
            char* name;

            if (node->as.InputStmt.varAccess->type == EXPR_CALL) {
                semanticError(analyser, node, "Call expression ");
                fprintf(stderr, "%.*s is not assignable.", (int)node->as.InputStmt.varAccess->length, node->as.InputStmt.varAccess->start);
                return TYPE_ERROR;
            }

            if (node->as.InputStmt.varAccess->type == EXPR_VARIABLE) {
                name = extractNullTerminatedString(node->as.InputStmt.varAccess->as.VariableExpr.name->start, node->as.InputStmt.varAccess->as.VariableExpr.name->length);
            } else if (node->as.InputStmt.varAccess->type == EXPR_ARRAY_ACCESS) {
                name = extractNullTerminatedString(node->as.InputStmt.varAccess->as.ArrayAccessExpr.name->start, node->as.InputStmt.varAccess->as.ArrayAccessExpr.name->length);
            } else {
                semanticError(analyser, node, "Expression not assignable.");
                return TYPE_ERROR;
            }

            //char* name = extractNullTerminatedString(node->as.InputStmt.name->start, node->as.InputStmt.name->length);
            Symbol var;
            bool res = findSymbol(analyser, name, &var);

            if (!res) {
                semanticError(analyser, node, "Target variable ");
                fprintf(stderr, "'%s' not in scope.", name);
                return TYPE_ERROR;
            }

            if (var.type == SYMBOL_FOR_COUNTER) {
                semanticError(analyser, node, "Symbol ");
                fprintf(stderr, "'%s' is a counter in a FOR loop. It can't be inputted to.", name);
                return TYPE_ERROR;
            }

            if (var.type != SYMBOL_VAR && var.type != SYMBOL_PARAM && !(var.type == SYMBOL_ARRAY && node->as.InputStmt.varAccess->type == EXPR_ARRAY_ACCESS)) {
                semanticError(analyser, node, "Symbol ");
                fprintf(stderr, "'%s' is not a variable. Array references, constants and subroutines can't be inputted to.", name);
                return TYPE_ERROR;
            }

            if (var.type == SYMBOL_PARAM && var.node->as.Parameter.isArray && node->as.InputStmt.varAccess->type != EXPR_ARRAY_ACCESS) {
                semanticError(analyser, node, "Symbol ");
                fprintf(stderr, "'%s' is not a variable, but an ARRAY reference. It can't be inputted to.", name);
                return TYPE_ERROR;
            }

            initialiseSymbol(analyser, name);

            /*if (var.type == SYMBOL_VAR) {
                node->as.InputStmt.expectedType = var.node->as.VarDeclareStmt.type;
            } else {
                node->as.InputStmt.expectedType = var.node->as.Parameter.type;
            }*/

            analyser->assigning = true;

            node->as.InputStmt.expectedType = semanticCheck(analyser, node->as.InputStmt.varAccess);

            analyser->assigning = false;

            initialiseSymbol(analyser, name);

            free(name);

            return TYPE_NONE;
        }
        case STMT_RETURN: {
            analyser->hasReturned = true;
            if (analyser->currentFunction == NULL || analyser->currentFunction->type != STMT_SUBROUTINE || analyser->currentFunction->as.SubroutineStmt.subroutineType != TYPE_FUNCTION) {
                semanticError(analyser, node, "RETURN statement may only be used in FUNCTION context.");
                return TYPE_ERROR;
            }

            DataType type = semanticCheck(analyser, node->as.ReturnStmt.expr);
            node->as.ReturnStmt.returnType = type;

            if (type != TYPE_ERROR && type != analyser->currentFunction->as.SubroutineStmt.returnValue && !(type == TYPE_INTEGER && analyser->currentFunction->as.SubroutineStmt.returnValue == TYPE_REAL)) {
                semanticError(analyser, node, "Unexpected return type.");
                return TYPE_ERROR;
            }

            return TYPE_NONE;
        }
        case STMT_WHILE: {
            DataType conditionType = semanticCheck(analyser, node->as.WhileStmt.condition);

            if (conditionType != TYPE_BOOLEAN) {
                semanticError(analyser, node, "WHILE loop condition must result in a boolean.");
                return TYPE_ERROR;
            }

            bool origReturn = analyser->hasReturned;

            createScope(analyser, SCOPE_LOOP);

            semanticCheck(analyser, node->as.WhileStmt.body);

            if (analyser->symbolTable->filesOpened > 0) {
                semanticWarning(node, "Not all file streams have been closed properly in this scope.");
            }
            endScope(analyser);

            analyser->hasReturned = origReturn;

            return TYPE_NONE;
        }
        case STMT_VAR_DECLARE: {
            char* name = extractNullTerminatedString(node->as.VarDeclareStmt.name->start, node->as.VarDeclareStmt.name->length);
            Symbol var;
            bool res = findSymbolInCurrScope(analyser, name, &var);
            if (res) {
                semanticError(analyser, node, "Symbol ");
                fprintf(stderr, "'%s' already exists.", name);
                return TYPE_ERROR;
            }

            res = findSymbol(analyser, name, &var);
            if (res) {
                semanticWarning(node, "Symbol ");
                fprintf(stderr, "'%s' redeclaration in inner scope shadows outer definition.", name);
            }

            addSymbol(analyser, name, node, SYMBOL_VAR);

            free(name);
            return TYPE_NONE;
        }
        case STMT_CONST_DECLARE: {
            char* name = extractNullTerminatedString(node->as.ConstDeclareStmt.name->start, node->as.ConstDeclareStmt.name->length);

            Symbol var;
            bool res = findSymbolInCurrScope(analyser, name, &var);
            if (res) {
                semanticError(analyser, node, "Symbol ");
                fprintf(stderr, "'%s' already exists.", name);
                return TYPE_ERROR;
            }

            res = findSymbol(analyser, name, &var);
            if (res) {
                semanticWarning(node, "Symbol ");
                fprintf(stderr, "'%s' redeclaration in inner scope shadows outer definition.", name);
            }

            addSymbol(analyser, name, node, SYMBOL_CONST);
            initialiseSymbol(analyser, name);
            free(name);
            return TYPE_NONE;
        }
        case STMT_ARRAY_DECLARE: {
            char* name = extractNullTerminatedString(node->as.ArrayDeclareStmt.name->start, node->as.ArrayDeclareStmt.name->length);

            Symbol var;
            bool res = findSymbolInCurrScope(analyser, name, &var);
            if (res) {
                semanticError(analyser, node, "Symbol ");
                fprintf(stderr, "'%s' already exists.", name);
                return TYPE_ERROR;
            }

            res = findSymbol(analyser, name, &var);
            if (res) {
                semanticWarning(node, "Symbol ");
                fprintf(stderr, "'%s' redeclaration in inner scope shadows outer definition.", name);
            }

            for (int i = 0; i < 4; i++) {
                ASTNode* dim = node->as.ArrayDeclareStmt.dimensions[i];
                if (dim == NULL) continue;
                DataType semRes = semanticCheck(analyser, dim);
                if (semRes != TYPE_INTEGER) {
                    semanticError(analyser, node, "Dimensions of array must be INTEGER values.");
                    return TYPE_ERROR;
                }
            }

            addSymbol(analyser, name, node, SYMBOL_ARRAY);
            initialiseSymbol(analyser, name);
            free(name);
            return TYPE_NONE;
        }
        case STMT_CASE: {
            DataType type = semanticCheck(analyser, node->as.CaseStmt.expr);

            node->as.CaseStmt.exprType = type;

            if (type != TYPE_INTEGER && type != TYPE_CHAR && type != TYPE_ERROR) {
                semanticError(analyser, node, "CASE statements may only handle expressions that result in INTEGER or CHAR values.");
                return TYPE_ERROR;
            }

            analyser->caseReturns = true;

            createScope(analyser, SCOPE_CONDITIONAL);
            analyser->hasDefault = false;

            semanticCheck(analyser, node->as.CaseStmt.body);

            if (!analyser->hasDefault) {
                semanticWarning(node, "There exists unhandled cases as CASE block doesn't have OTHERWISE.");
                analyser->caseReturns = false;
            }

            if (analyser->symbolTable->filesOpened > 0) {
                semanticWarning(node, "Not all file streams have been closed properly in this scope.");
            }
            endScope(analyser);

            if (analyser->caseReturns) {
                analyser->hasReturned = true;
            }

            return TYPE_NONE;
        }
        case STMT_REPEAT: {
            DataType conditionType = semanticCheck(analyser, node->as.RepeatStmt.condition);

            if (conditionType != TYPE_BOOLEAN) {
                semanticError(analyser, node, "REPEAT-UNTIL condition must result in a BOOLEAN.");
                return TYPE_ERROR;
            }

            bool origReturn = analyser->hasReturned;

            createScope(analyser, SCOPE_LOOP);

            semanticCheck(analyser, node->as.RepeatStmt.body);

            if (analyser->symbolTable->filesOpened > 0) {
                semanticWarning(node, "Not all file streams have been closed properly in this scope.");
            }
            endScope(analyser);

            analyser->hasReturned = origReturn;

            return TYPE_NONE;
        }
        case STMT_FOR: {
            if (semanticCheck(analyser, node->as.ForStmt.init) != TYPE_INTEGER) {
                semanticError(analyser, node, "Initial value in FOR loop must be of type INTEGER.");
                return TYPE_ERROR;
            }
            if (semanticCheck(analyser, node->as.ForStmt.end) != TYPE_INTEGER) {
                semanticError(analyser, node, "Final value in FOR loop must be of type INTEGER.");
                return TYPE_ERROR;
            }
            if (node->as.ForStmt.step != NULL && (node->as.ForStmt.step->type != EXPR_LITERAL || semanticCheck(analyser, node->as.ForStmt.step) != TYPE_INTEGER)) {
                semanticError(analyser, node, "STEP value in FOR loop must be an INTEGER literal.");
                return TYPE_ERROR;
            }

            char* name = extractNullTerminatedString(node->as.ForStmt.counterName->start, node->as.ForStmt.counterName->length);

            Symbol counter;
            bool res = findSymbol(analyser, name, &counter);

            if (res) {
                if (counter.type != SYMBOL_VAR && counter.type != SYMBOL_PARAM && counter.type != SYMBOL_FOR_COUNTER) {
                    semanticError(analyser, node, "Symbol ");
                    fprintf(stderr, "'%s' already exists and is not a valid counter symbol.", name);
                    return TYPE_ERROR;
                }

                if (counter.type == SYMBOL_FOR_COUNTER) {
                    semanticError(analyser, node, "");
                    fprintf(stderr, "'%s' is already a counter variable for another FOR loop. It can't be used.", name);
                    return TYPE_ERROR;
                }

                DataType type;

                if (counter.type == SYMBOL_PARAM) {
                    if (counter.node->as.Parameter.isArray) {
                        semanticError(analyser, node, "Symbol ");
                        fprintf(stderr, "'%s' already exists and is not a valid counter type.", name);
                        return TYPE_ERROR;
                    }
                }

                if (counter.type == SYMBOL_VAR) {
                    type = counter.node->as.VarDeclareStmt.type;
                } else {
                    type = counter.node->as.Parameter.type;
                }

                if (type != TYPE_INTEGER) {
                    semanticError(analyser, node, "Symbol ");
                    fprintf(stderr, "'%s' already exists and is not type INTEGER.", name);
                    return TYPE_ERROR;
                }

                semanticWarning(node, "Symbol ");
                fprintf(stderr, "'%s' already exists. It is not recommended as it may lead to infinite loops, as only FOR loop variable counters are protected from assignment.", name);
                createScope(analyser, SCOPE_LOOP);
            } else {
                createScope(analyser, SCOPE_LOOP);
                addSymbol(analyser, name, node, SYMBOL_FOR_COUNTER);
                initialiseSymbol(analyser, name);
            }

            bool origReturn = analyser->hasReturned;

            semanticCheck(analyser, node->as.ForStmt.body);

            if (analyser->symbolTable->filesOpened > 0) {
                semanticWarning(node, "Not all file streams have been closed properly in this scope.");
            }
            endScope(analyser);

            analyser->hasReturned = origReturn;

            return TYPE_NONE;
        }
        case STMT_CALL: {
            char* name = extractNullTerminatedString(node->as.CallStmt.name->start, node->as.CallStmt.name->length);
            Symbol callable;
            bool res = findSymbol(analyser, name, &callable);
            if (!res) {
                semanticError(analyser, node, "Callable symbol ");
                fprintf(stderr, "'%s' not in scope.", name);
                return TYPE_ERROR;
            }
            if (callable.type != SYMBOL_PROC) {
                semanticError(analyser, node, "Expected procedure in CALL statement, but got other.");
                return TYPE_ERROR;
            }
            if (!callable.initialised) {
                semanticError(analyser, node, "Symbol ");
                fprintf(stderr, "'%s' is not initialised previously and therefore cannot be used.", name);
                return TYPE_ERROR;
            }
            if (callable.node->as.SubroutineStmt.parameters.count != node->as.CallStmt.arguments.count) {
                semanticError(analyser, node, "Expected ");
                fprintf(stderr, "%d arguments as per definition but got %d.", callable.node->as.SubroutineStmt.parameters.count,
                        node->as.CallStmt.arguments.count);
                return TYPE_ERROR;
            }

            free(name);

            for (int i = 0; i < node->as.CallStmt.arguments.count; i++) {
                DataType type = semanticCheck(analyser, node->as.CallStmt.arguments.start[i]);
                if (type == TYPE_ERROR) continue;
                if (type != callable.node->as.SubroutineStmt.parameters.start[i]->as.Parameter.type && !(type == TYPE_ARRAY && callable.node->as.SubroutineStmt.parameters.start[i]->as.Parameter.isArray)) {
                    semanticError(analyser, node, "Argument number ");
                    fprintf(stderr, "%d is not correct type.", i + 1);
                    return TYPE_ERROR;
                }

                if (callable.node->as.SubroutineStmt.parameters.start[i]->as.Parameter.byref && node->as.CallStmt.arguments.start[i]->type != EXPR_VARIABLE) {
                    semanticError(analyser, node, "Procedure expects reference to a variable, so argument must be a variable expression.\n");
                    return TYPE_ERROR;
                }

                if (type == TYPE_ARRAY) {
                    DataType baseType = getArrayBaseType(analyser, node->as.CallStmt.arguments.start[i], node->as.CallStmt.arguments.start[i]->type);
                    if (baseType != callable.node->as.SubroutineStmt.parameters.start[i]->as.Parameter.type) {
                        semanticError(analyser, node, "Argument number ");
                        fprintf(stderr, "%d is an array but not of the correct type.", i + 1);
                        return TYPE_ERROR;
                    }
                }
            }

            return TYPE_NONE;
        }
        case STMT_CASE_LINE: {
            if (analyser->hasDefault) {
                semanticError(analyser, node, "CASE line after OTHERWISE. Place it before OTHERWISE.");
                return TYPE_ERROR;
            }

            if (node->as.CaseLineStmt.value == NULL) {
                analyser->hasDefault = true;
            } else {
                if (node->as.CaseLineStmt.value->as.Expr.resultType != TYPE_INTEGER && node->as.CaseLineStmt.value->as.Expr.resultType != TYPE_CHAR) {
                    semanticError(analyser, node, "Value in CASE line must result in INTEGER or CHAR value.");
                    return TYPE_ERROR;
                }
            }

            bool origReturns = analyser->hasReturned;
            analyser->hasReturned = false;

            semanticCheck(analyser, node->as.CaseLineStmt.result);

            analyser->caseReturns &= analyser->hasReturned;

            analyser->hasReturned = origReturns;

            return TYPE_NONE;
        }
        case STMT_CASE_BLOCK: {
            semanticCheck(analyser, node->as.CaseBlockStmt.body);
            return TYPE_NONE;
        }
        case STMT_OPENFILE: {
            char* filename = extractNullTerminatedString(node->as.OpenfileStmt.filename->start, node->as.OpenfileStmt.filename->length);
            Symbol file;
            bool res = findSymbol(analyser, filename, &file);
            if (res) {
                semanticError(analyser, node, "File ");
                fprintf(stderr, "%s is already open.", filename);
                return TYPE_ERROR;
            }

            bool added = addFile(analyser, filename, node, SYMBOL_FILE, node->as.OpenfileStmt.accessType);
            initialiseSymbol(analyser, filename);

            free(filename);

            return TYPE_NONE;
        }
        case STMT_CLOSEFILE: {
            char* filename = extractNullTerminatedString(node->as.ClosefileStmt.filename->start, node->as.ClosefileStmt.filename->length);
            Symbol file;
            bool res = findSymbolInCurrScope(analyser, filename, &file);
            if (!res) {
                semanticError(analyser, node, "File ");
                fprintf(stderr, "%s not found in current scope. Files may only be closed in the same scope in which they were opened.", filename);
                return TYPE_ERROR;
            }

            removeFile(analyser, filename);
            free(filename);

            return TYPE_NONE;
        }
        case STMT_READFILE: {
            char* filename = extractNullTerminatedString(node->as.ReadfileStmt.filename->start, node->as.ReadfileStmt.filename->length);
            Symbol file;
            bool res = findSymbol(analyser, filename, &file);
            if (!res) {
                semanticError(analyser, node, "File ");
                fprintf(stderr, "%s is not open and therefore can't be read.", filename);
                return TYPE_ERROR;
            }

            if (file.type != SYMBOL_FILE) {
                semanticError(analyser, node, "Symbol ");
                fprintf(stderr, "%s is not a file.", filename);
                return TYPE_ERROR;
            }

            if (file.access != ACCESS_READ) {
                semanticError(analyser, node, "File ");
                fprintf(stderr, "%s is not open for READ, therefore it can't be read.", filename);
                return TYPE_ERROR;
            }

            free(filename);

            char* name;

            if (node->as.ReadfileStmt.varAccess->type == EXPR_CALL) {
                semanticError(analyser, node, "Call expression ");
                fprintf(stderr, "%.*s is not assignable.", (int)node->as.ReadfileStmt.varAccess->length, node->as.ReadfileStmt.varAccess->start);
                return TYPE_ERROR;
            }

            if (node->as.ReadfileStmt.varAccess->type == EXPR_VARIABLE) {
                name = extractNullTerminatedString(node->as.ReadfileStmt.varAccess->as.VariableExpr.name->start, node->as.ReadfileStmt.varAccess->as.VariableExpr.name->length);
            } else if (node->as.InputStmt.varAccess->type == EXPR_ARRAY_ACCESS) {
                name = extractNullTerminatedString(node->as.ReadfileStmt.varAccess->as.ArrayAccessExpr.name->start, node->as.ReadfileStmt.varAccess->as.ArrayAccessExpr.name->length);
            } else {
                semanticError(analyser, node, "Expression not assignable.");
                return TYPE_ERROR;
            }

            //char* name = extractNullTerminatedString(node->as.InputStmt.name->start, node->as.InputStmt.name->length);
            Symbol var;
            bool res2 = findSymbol(analyser, name, &var);

            if (!res) {
                semanticError(analyser, node, "Target variable ");
                fprintf(stderr, "'%s' not in scope.", name);
                return TYPE_ERROR;
            }

            if (var.type == SYMBOL_FOR_COUNTER) {
                semanticError(analyser, node, "Symbol ");
                fprintf(stderr, "'%s' is a counter in a FOR loop. It can't be inputted to.", name);
                return TYPE_ERROR;
            }

            if (var.type != SYMBOL_VAR && var.type != SYMBOL_PARAM && !(var.type == SYMBOL_ARRAY && node->as.InputStmt.varAccess->type == EXPR_ARRAY_ACCESS)) {
                semanticError(analyser, node, "Symbol ");
                fprintf(stderr, "'%s' is not a variable. Array references, constants and subroutines can't be inputted to.", name);
                return TYPE_ERROR;
            }

            if (var.type == SYMBOL_PARAM && var.node->as.Parameter.isArray && node->as.InputStmt.varAccess->type != EXPR_ARRAY_ACCESS) {
                semanticError(analyser, node, "Symbol ");
                fprintf(stderr, "'%s' is not a variable, but an ARRAY reference. It can't be inputted to.", name);
                return TYPE_ERROR;
            }

            /*if (var.type == SYMBOL_VAR) {
                node->as.InputStmt.expectedType = var.node->as.VarDeclareStmt.type;
            } else {
                node->as.InputStmt.expectedType = var.node->as.Parameter.type;
            }*/

            analyser->assigning = true;

            if (semanticCheck(analyser, node->as.InputStmt.varAccess) != TYPE_STRING) {
                semanticError(analyser, node, "Target variable in READFILE must be of type STRING.");
                return TYPE_ERROR;
            }

            analyser->assigning = false;

            initialiseSymbol(analyser, name);

            free(name);

            return TYPE_NONE;
        }
        case STMT_WRITEFILE: {
            char* filename = extractNullTerminatedString(node->as.WritefileStmt.filename->start, node->as.WritefileStmt.filename->length);
            Symbol file;

            bool res = findSymbol(analyser, filename, &file);
            if (!res) {
                semanticError(analyser, node, "File ");
                fprintf(stderr, "%s is not open and therefore can't be written to.", filename);
                return TYPE_ERROR;
            }

            if (file.type != SYMBOL_FILE) {
                semanticError(analyser, node, "Symbol ");
                fprintf(stderr, "%s is not a file.", filename);
                return TYPE_ERROR;
            }

            if (file.access != ACCESS_WRITE && file.access != ACCESS_APPEND) {
                semanticError(analyser, node, "File ");
                fprintf(stderr, "%s is not open for WRITE nor APPEND, therefore it can't be written to.", filename);
                return TYPE_ERROR;
            }

            free(filename);

            for (int i = 0; i < node->as.WritefileStmt.expressions.count; i++) {
                semanticCheck(analyser, node->as.WritefileStmt.expressions.start[i]);
            }

            return TYPE_NONE;

        }
        case STMT_PROGRAM: {
            for (int i = 0; i < node->as.ProgramStmt.body.count; i++) {
                semanticCheck(analyser, node->as.ProgramStmt.body.start[i]);
            }
            return TYPE_NONE;
        }
        case AST_PARAMETER: {
            if (node->as.Parameter.byref) {
                if (node->as.Parameter.type != TYPE_INTEGER && node->as.Parameter.type != TYPE_REAL && node->as.Parameter.type != TYPE_CHAR && node->as.Parameter.type != TYPE_BOOLEAN) {
                    semanticError(analyser, node, "Parameter may only be passed BYREF it it expects a primitive type.\n");
                    return TYPE_ERROR;
                }
            }

            char* name = extractNullTerminatedString(node->as.Parameter.name->start, node->as.Parameter.name->length);
            addSymbol(analyser, name, node, SYMBOL_PARAM);
            initialiseSymbol(analyser, name);
            free(name);
            return TYPE_NONE;
        }
        default: return TYPE_NONE;
    }
}

void initAnalyser(Analyser* analyser) {
    analyser->symbolTable = malloc(sizeof(SymbolTable));
    initTable(analyser->symbolTable);
    analyser->globalTable = analyser->symbolTable;
    analyser->depth = 0;
    analyser->currentFunction = NULL;
    analyser->hasReturned = false;
    analyser->assigning = false;
    analyser->hasDefault = false;
    analyser->hadError = false;
}

void freeAnalyser(Analyser* analyser) {
    freeTable(analyser->symbolTable);
    freeTable(analyser->globalTable);
    //free(analyser);
}

bool semanticAnalysis(Analyser* analyser, AST* ast) {
    ASTNode* program = ast->program;

    //
    Builtin substring;
    createBuiltin(&substring, 3, TYPE_STRING, 0);
    addParamDatatype(&substring, TYPE_STRING, 0);
    addParamDatatype(&substring, TYPE_INTEGER, 1);
    addParamDatatype(&substring, TYPE_INTEGER, 2);
    addSymbol(analyser, "SUBSTRING", (ASTNode*)&substring, SYMBOL_BUILTIN_FUNC);

    Builtin length;
    createBuiltin(&length, 1, TYPE_INTEGER, 1);
    addParamDatatype(&length, TYPE_STRING, 0);
    addSymbol(analyser, "LENGTH", (ASTNode*)&length, SYMBOL_BUILTIN_FUNC);

    Builtin lcase;
    createBuiltin(&lcase, 1, TYPE_STRING, 2);
    addParamDatatype(&lcase, TYPE_STRING, 0);
    addSymbol(analyser, "LCASE", (ASTNode*)&lcase, SYMBOL_BUILTIN_FUNC);

    Builtin ucase;
    createBuiltin(&ucase, 1, TYPE_STRING, 3);
    addParamDatatype(&ucase, TYPE_STRING, 0);
    addSymbol(analyser, "UCASE", (ASTNode*)&lcase, SYMBOL_BUILTIN_FUNC);

    Builtin randomBetween;
    createBuiltin(&randomBetween, 2, TYPE_INTEGER, 4);
    addParamDatatype(&randomBetween, TYPE_INTEGER, 0);
    addParamDatatype(&randomBetween, TYPE_INTEGER, 1);
    addSymbol(analyser, "RANDOMBETWEEN", (ASTNode*)&randomBetween, SYMBOL_BUILTIN_FUNC);

    Builtin rnd;
    createBuiltin(&rnd, 0, TYPE_REAL, 5);
    addSymbol(analyser, "RND", (ASTNode*)&rnd, SYMBOL_BUILTIN_FUNC);

    Builtin integer;
    createBuiltin(&integer, 1, TYPE_INTEGER, 6);
    addParamDatatype(&integer, TYPE_REAL, 0);
    addSymbol(analyser, "INT", (ASTNode*)&integer, SYMBOL_BUILTIN_FUNC);

    Builtin eof;
    createBuiltin(&eof, 1, TYPE_BOOLEAN, 7);
    addParamDatatype(&eof, TYPE_STRING, 0);
    addSymbol(analyser, "EOF", (ASTNode*)&eof, SYMBOL_BUILTIN_FUNC);

    Builtin charAt;
    createBuiltin(&charAt, 2, TYPE_CHAR, 8);
    addParamDatatype(&charAt, TYPE_STRING, 0);
    addParamDatatype(&charAt, TYPE_INTEGER, 1);
    addSymbol(analyser, "CHARAT", (ASTNode*)&charAt, SYMBOL_BUILTIN_FUNC);
    //

    // SUBROUTINES ARE NO LONGER STATIC, MUST BE DECLARED BEFORE USE
    //bool staticRes = staticSymbolCheck(analyser, program); // CHECKS FOR SUBROUTINES IN THE TOP LEVEL SO THAT THEY ARE STATIC SYMBOLS

    /*if (!staticRes) {
        fprintf(stderr, "\n\nSomething failed in static analysis.\n\n");
    }*/

    semanticCheck(analyser, program);

    return analyser->hadError;
}