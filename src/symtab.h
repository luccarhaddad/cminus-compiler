#ifndef _SYMTAB_H_
#define _SYMTAB_H_

#include "types.h"

typedef enum { SYMBOL_VARIABLE, SYMBOL_FUNCTION, SYMBOL_PARAMETER } SymbolKind;

typedef struct Symbol {
	const char* name;
	SymbolKind  kind;
	TypeInfo*   type;
	int         offset; // Memory location/offset

	struct {
		int  definedAt;  // Line where symbol was defined
		int* references; // Array of line numbers where symbol is used
		int  refCount;   // Number of references
	} sourceInfo;

	struct Symbol* next; // For hash table chaining
} Symbol;

typedef struct Scope {
	const char*   name;
	int           level; // Nesting level
	struct Scope* parent;
	Symbol**      symbols;     // Hash table of symbols
	int           symbolCount; // Number of symbols in this scope
} Scope;

/* Symbol table functions */
Symbol* createSymbol(const char* name, SymbolKind kind, TypeInfo* type);
void    addSymbol(Scope* scope, Symbol* symbol);
Symbol* findSymbol(Scope* scope, const char* name);
void    addReference(Symbol* symbol, int lineNo);
void    destroySymbol(Symbol* symbol);

/* Scope functions */
Scope* createScope(const char* name, Scope* parent);
void   destroyScope(Scope* scope);

#endif
