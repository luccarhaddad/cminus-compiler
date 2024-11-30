#include "symtab.h"
#include "util.h"
#include <log.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define HASH_SIZE 211
#define SHIFT 4
#define REF_CAPACITY 10

static unsigned int hash(const char* name) {
	unsigned int h = 0;
	while (*name) {
		h = h * 31 + *name++;
	}
	return h % HASH_SIZE;
}

Symbol* createSymbol(const char* name, const SymbolKind kind, TypeInfo* type) {
	Symbol* symbol = malloc(sizeof(Symbol));
	if (!symbol) return NULL;

	symbol->name   = strdup(name);
	symbol->kind   = kind;
	symbol->type   = type;
	symbol->offset = 0;
	symbol->next   = NULL;

	if (symbol->kind == SYMBOL_FUNCTION)
		symbol->type->returnType = createType(type->baseType);

	symbol->sourceInfo.definedAt  = 0;
	symbol->sourceInfo.references = (int*) malloc(REF_CAPACITY * sizeof(int));
	symbol->sourceInfo.refCount   = 0;

	return symbol;
}

void addSymbol(Scope* scope, Symbol* symbol) {
	if (!scope || !symbol) return;

	const unsigned int h = hash(symbol->name);
	if (!scope->symbols) scope->symbols = (Symbol**) calloc(HASH_SIZE, sizeof(Symbol*));

	Symbol* current = scope->symbols[h];
	while (current) {
		if (strcmp(current->name, symbol->name) == 0) return;
		current = current->next;
	}

	symbol->next      = scope->symbols[h];
	scope->symbols[h] = symbol;
	scope->symbolCount++;
}

Symbol* findSymbol(Scope* scope, const char* name) {
	if (!scope || !name) return NULL;

	Symbol* current = NULL;
	const unsigned int h = hash(name);
	while (scope) {
		if (scope->symbols) {
			current = scope->symbols[h];
			while (current) {
				if (strcmp(current->name, name) == 0) {
					return current;
				}
				current = current->next;
			}
		}
		scope = scope->parent;
	}
	return current;
}

Symbol* findSymbolInScope(Scope* scope, const char* name) {
	if (!scope || !name) return NULL;

	Symbol* current = NULL;
	const unsigned int h = hash(name);
	if (scope->symbols) {
		current = scope->symbols[h];
		while (current) {
			if (strcmp(current->name, name) == 0) {
				return current;
			}
			current = current->next;
		}
	}
	return current;
}

static bool findReference(Symbol* symbol, const int lineNo) {
	if (!symbol) return false;

	for (int i = 0; i < symbol->sourceInfo.refCount; i++) {
		if (symbol->sourceInfo.references[i] == lineNo) {
			return true;
		}
	}
	return false;
}

void addReference(Symbol* symbol, const int lineNo) {
	if (!symbol) return;

	if (findReference(symbol, lineNo)) {
		return;
	}
	if (symbol->sourceInfo.refCount % REF_CAPACITY == 0) {
		const int newSize = (symbol->sourceInfo.refCount + REF_CAPACITY) * sizeof(int);
		int*      newRefs = realloc(symbol->sourceInfo.references, newSize);
		if (!newRefs) return;
		symbol->sourceInfo.references = newRefs;
	}
	symbol->sourceInfo.references[symbol->sourceInfo.refCount++] = lineNo;
}

void destroySymbol(Symbol* symbol) {
	if (!symbol) return;

	free((void*) symbol->name);
	free(symbol->sourceInfo.references);
	free(symbol);
}

Scope* createScope(const char* name, Scope* parent) {
	Scope* scope = (Scope*) malloc(sizeof(Scope));
	if (!scope) return NULL;

	scope->name        = strdup(name);
	scope->parent      = parent;
	scope->level       = parent ? parent->level + 1 : 0;
	scope->symbols     = NULL;
	scope->symbolCount = 0;
	scope->children    = NULL;
	scope->childCount  = 0;

	return scope;
}

void destroyScope(Scope* scope) {
	if (!scope) return;

	for (int i = 0; i < scope->childCount; i++) {
		destroyScope(scope->children[i]);
	}
	free(scope->children);

	if (scope->symbols) {
		for (int i = 0; i < HASH_SIZE; i++) {
			Symbol* current = scope->symbols[i];
			while (current) {
				Symbol* next = current->next;
				destroySymbol(current);
				current = next;
			}
		}
		free(scope->symbols);
	}

	free((void*) scope->name);
	free(scope);
}

static const char* getTypeName(const TypeInfo* type) {
	if (!type) return "unknown";
	switch (type->baseType) {
		case TYPE_VOID:
			return "void";
		case TYPE_ARRAY:
		case TYPE_INT:
			return "int";
		default:
			return "unknown";
	}
}

static const char* symbolKindToStr(const Symbol* symbol) {
	switch (symbol->kind) {
		case SYMBOL_PARAMETER:
			if (symbol->type->arraySize >= 0) return "array";
			return "var";
		case SYMBOL_VARIABLE:
			return "var";
		case SYMBOL_FUNCTION:
			return "fun";
		case SYMBOL_ARRAY:
			return "array";
		default:
			return "unknown";
	}
}

static void printSymbol(Symbol* symbol, const char* scopeName) {
	if (!symbol) return;

	pc("%-14s ", symbol->name);
	pc("%-9s ", strcmp(scopeName, "global") == 0 ? "" : scopeName);
	pc("%-8s ", symbolKindToStr(symbol));
	pc("%-9s ", getTypeName(symbol->kind != SYMBOL_FUNCTION ? symbol->type : symbol->type->returnType));
	pc(" ");
	for (int i = 0; i < symbol->sourceInfo.refCount; i++)
		pc("%2d ", symbol->sourceInfo.references[i]);

	pc("\n");
}

static void printScopeSymbols(Scope* scope, int level) {
	if (!scope) return;

	if (scope->symbols) {
		for (int i = 0; i < HASH_SIZE; i++) {
			Symbol* current = scope->symbols[i];
			while (current) {
				if (current->kind != SYMBOL_FUNCTION || level == 0) {
					printSymbol(current, scope->name);
				}
				current = current->next;
			}
		}
	}

	for (int i = 0; i < scope->childCount; i++) {
		if (scope->children[i]) printScopeSymbols(scope->children[i], level + 1);
	}
}

void printSymbolTable(Scope* globalScope, bool declaredMain) {
	if (!globalScope) return;

	pc("\nSymbol table:\n\n");
	pc("Variable Name  Scope     ID Type  Data Type  Line Numbers\n");
	pc("-------------  --------  -------  ---------  -------------------------\n");

	printScopeSymbols(globalScope, 0);
	if (!declaredMain) {
		pc("Semantic error: undefined reference to 'main'\n");
	}
}
