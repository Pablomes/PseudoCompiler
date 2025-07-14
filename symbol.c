

#include "common.h"
#include "symbol.h"

#define TABLE_MAX_LOAD 0.75

static uint32_t hashString(const char* key) {
    uint32_t hash = 2166136261u;
    for (const char* c = key; *c != '\0'; c++) {
        hash ^= (uint8_t)(*c);
        hash *= 16777619;
    }
    return hash;
}

static Entry* findEntry(Entry* entries, int capacity, const char* key) {
    uint32_t index = hashString(key) % capacity;
    Entry* tombstone = NULL;

    while (true) {
        Entry* entry = &entries[index];

        if (entry->key == NULL) {
            if (entry->symbol == NULL) {
                return tombstone != NULL ? tombstone : entry;
            } else {
                if (tombstone == NULL) tombstone = entry;
            }
        } else if (strcmp(entry->key, key) == 0) {
            return entry;
        }

        index = (index + 1) % capacity;
    }
}

void initTable(SymbolTable* table) {
    table->count = 0;
    table->capacity = 16;
    table->entries = calloc(16, sizeof(Entry));
    table->enclosing = NULL;
    table->scopeType = SCOPE_GLOBAL;
    table->nextPos = 0;
    table->filesOpened = 0;
}

void freeTable(SymbolTable* table) {
    if (table->enclosing != NULL) freeTable(table->enclosing);

    for (int i = 0; i < table->capacity; i++) {
        Entry* entry = &table->entries[i];
        if (entry->key == NULL) continue;
        free(entry->key);
        free(entry->symbol);
    }
    free(table->entries);
    initTable(table);
}

static void adjustCapacity(SymbolTable* table, int capacity) {
    Entry* entries = calloc(capacity, sizeof(Entry));
    for (int i = 0; i < table->capacity; i++) {
        Entry* oldEntry = &table->entries[i];
        if (oldEntry->key == NULL) continue;

        Entry* dest = findEntry(entries, capacity, oldEntry->key);
        dest->key = oldEntry->key;
        dest->symbol = oldEntry->symbol;
    }

    free(table->entries);
    table->entries = entries;
    table->capacity = capacity;
}

bool setTable(SymbolTable* table, const char* key, ASTNode* node, SymbolType type, int pos, bool isRelative, bool byref) {
    setTableFile(table, key, node, type, pos, isRelative, byref, ACCESS_NONE);
}

bool setTableFile(SymbolTable* table, const char* key, ASTNode* node, SymbolType type, int pos, bool isRelative, bool byref, FileAccessType access) {
    if ((table->count + 1) > table->capacity * TABLE_MAX_LOAD) {
        int newCapacity = table->capacity < 8 ? 8 : table->capacity * 2;
        adjustCapacity(table, newCapacity);
    }

    Entry* entry = findEntry(table->entries, table->capacity, key);
    bool isNewKey = entry->key == NULL;

    if (isNewKey && entry->symbol == NULL) table->count++;

    if (isNewKey) {
        entry->key = strdup(key);
    }

    entry->symbol = malloc(sizeof(Symbol));
    entry->symbol->type = type;
    entry->symbol->node = node;
    entry->symbol->initialised = false;
    entry->symbol->pos = pos;
    entry->symbol->isRelative = isRelative;
    entry->symbol->byref = byref;
    entry->symbol->access = access;

    return isNewKey;
}

bool getCurrTable(SymbolTable* table, const char* key, Symbol* outSymbol) {
    SymbolTable* current = table;
    Entry* entry = findEntry(current->entries, current->capacity, key);
    if (entry->key != NULL) {
        *outSymbol = *(entry->symbol);
        return true;
    }

    return false;
}

bool getTable(SymbolTable* table, const char* key, Symbol* outSymbol) {
    SymbolTable* current = table;
    while (current != NULL) {
        Entry* entry = findEntry(current->entries, current->capacity, key);
        if (entry->key != NULL) {
            *outSymbol = *(entry->symbol);
            return true;
        }
        current = current->enclosing;
    }
    return false;
}

Symbol* getTablePointer(SymbolTable* table, const char* key) {
    SymbolTable* current = table;
    while (current != NULL) {
        Entry* entry = findEntry(current->entries, current->capacity, key);
        if (entry->key != NULL) {
            return entry->symbol;
        }
        current = current->enclosing;
    }
    return NULL;
}

bool deleteTable(SymbolTable* table, const char* key) {
    if (table->count == 0) return false;

    Entry* entry = findEntry(table->entries, table->capacity, key);
    if (entry->key == NULL) return false;

    free(entry->key);
    free(entry->symbol);
    entry->key = NULL;
    entry->symbol = (Symbol*)1;  // Tombstone marker
    return true;
}

void copyOverTable(SymbolTable* from, SymbolTable* to) {
    for (int i = 0; i < from->capacity; i++) {
        Entry* entry = &from->entries[i];
        if (entry->key != NULL && entry->symbol != NULL) {
            setTable(to, entry->key, entry->symbol->node, entry->symbol->type, entry->symbol->pos, entry->symbol->isRelative, entry->symbol->byref);
        }
    }

    to->nextPos = from->nextPos;
}
void clearTable(SymbolTable* table) {
    for (int i = 0; i < table->capacity; i++) {
        Entry* entry = &table->entries[i];
        if (entry->key != NULL) {
            free(entry->key);
            free(entry->symbol);
            entry->key = NULL;
            entry->symbol = NULL;
        }
    }
    table->count = 0;
    table->nextPos = 0;
}

void createBuiltin(Builtin* func, int numParams, DataType returnType, int idx) {
    func->builtinIdx = idx;
    func->returnType = returnType;
    func->numParams = numParams;
    func->parameterTypes = (DataType*) malloc(sizeof(DataType) * numParams);

    if (func->parameterTypes == NULL) {
        printf("Error creating builtin function declarations.\n");
        return;
    }
}

void addParamDatatype(Builtin* func, DataType type, int idx) {
    func->parameterTypes[idx] = type;
}