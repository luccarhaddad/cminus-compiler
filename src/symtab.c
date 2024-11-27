#include "symtab.h"

#include <log.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HASH_SIZE 211
#define SHIFT 4
#define REF_CAPACITY 10

static unsigned int hash(const char* key) {
	int temp = 0;
	int i    = 0;
	while (key[i] != '\0') {
		temp = ((temp << SHIFT) + key[i]) % HASH_SIZE;
		++i;
	}
	return temp;
}

Symbol* createSymbol(const char* name, const SymbolKind kind, TypeInfo* type) {
	Symbol* symbol = malloc(sizeof(Symbol));
	if (!symbol) return NULL;

	symbol->name   = strdup(name);
	symbol->kind   = kind;
	symbol->type   = type;
	symbol->offset = 0;
	symbol->next   = NULL;

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

	const unsigned int h = hash(name);
	while (scope) {
		if (scope->symbols) {
			Symbol* current = scope->symbols[h];
			while (current) {
				if (strcmp(current->name, name) == 0) return current;
				current = current->next;
			}
		}
		scope = scope->parent;
	}

	return NULL;
}

bool findReference(Symbol* symbol, const int lineNo) {
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

static const char* symbolKindToStr(const SymbolKind kind) {
	switch (kind) {
		case SYMBOL_VARIABLE:
			return "var";
		case SYMBOL_FUNCTION:
			return "fun";
		case SYMBOL_PARAMETER:
			return "param";
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
	pc("%-8s ", symbolKindToStr(symbol->kind));
	pc("%-10s ", getTypeName(symbol->type));

	for (int i = 0; i < symbol->sourceInfo.refCount; i++)
		pc("%d ", symbol->sourceInfo.references[i]);

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

void printSymbolTable(Scope* globalScope) {
	if (!globalScope) return;

	pc("\nSymbol table:\n\n");
	pc("Variable Name  Scope     ID Type  Data Type  Line Numbers\n");
	pc("-------------  --------  -------  ---------  -------------------------\n");

	printScopeSymbols(globalScope, 0);
}
