#include "analyze.h"
#include "globals.h"
#include "symtab.h"
#include <log.h>
#include <stdbool.h>
#include <stdlib.h>

static Scope* currentScope = NULL;
static Scope* globalScope = NULL;
static TypeInfo* currentFunctionType = NULL;

static void enterScope(const char* name) {
	if (currentScope) {
		for (int i = 0; i < currentScope->childCount; i++) {
			if (strcmp(currentScope->children[i]->name, name) == 0) {
				currentScope = currentScope->children[i];
				return;
			}
		}
	}

	Scope* newScope = createScope(name, currentScope);

	if (currentScope) {
		if (!currentScope->children) {
			currentScope->children = (Scope**)malloc(sizeof(Scope*));
			currentScope->childCount = 0;
		} else {
			currentScope->children = (Scope**)realloc(
				currentScope->children,
				sizeof(Scope*) * (currentScope->childCount + 1)
			);
		}

		currentScope->children[currentScope->childCount++] = newScope;
	}

	currentScope = newScope;
	if (!globalScope)
		globalScope = currentScope;
}

static void leaveScope(void) {
	if (currentScope) {
		currentScope = currentScope->parent;
	}
}

static void typeError(const ASTNode* t, const char* message){
	pce("Type error at line %d: %s\n", t->lineNo, message);
	Error = TRUE;
}

static bool checkBinaryOperands(const ASTNode* t, const TypeInfo* expectedType) {
	if (!t || !expectedType) return false;

	// Check the left operand
	const ASTNode* leftNode = t->children[0];
	if (!leftNode) {
		typeError(t, "Missing left operand");
		return false;
	}

	TypeInfo* leftType = NULL;
	switch (leftNode->kind) {
		case NODE_VARIABLE:
		case NODE_IDENTIFIER:
			leftType = leftNode->data.symbol.type;
		break;
		case NODE_CONSTANT:
			leftType = createType(TYPE_INT);
		break;
		default:
			typeError(t, "Invalid left operand type");
		return false;
	}

	if (!leftType) {
		typeError(t, "Left operand has no type information");
		return false;
	}

	if (!areTypesCompatible(leftType, expectedType)) {
		typeError(t, "Left operand type does not match expected type");
		return false;
	}

	const ASTNode* rightNode = t->children[1];
	if (!rightNode) {
		typeError(t, "Missing right operand");
		return false;
	}

	TypeInfo* rightType = NULL;
	switch (rightNode->kind) {
		case NODE_VARIABLE:
		case NODE_IDENTIFIER:
			rightType = rightNode->data.symbol.type;
		break;
		case NODE_CONSTANT:
			rightType = createType(TYPE_INT);
		break;
		default:
			typeError(t, "Invalid right operand type");
		return false;
	}

	if (!rightType) {
		typeError(t, "Right operand has no type information");
		return false;
	}

	if (!areTypesCompatible(rightType, expectedType)) {
		typeError(t, "Left operand type does not match expected type");
		return false;
	}

	return true;
}

static void traverse(ASTNode* t, void (*preProc)(ASTNode*), void (*postProc)(ASTNode*)) {
	if (t != NULL) {
		preProc(t);
		for (int i = 0; i < MAXCHILDREN; i++)
			traverse(t->children[i], preProc, postProc);
		postProc(t);
		traverse(t->next, preProc, postProc);
	}
}

static void nullProc(ASTNode* t) { }

static void insertNode(ASTNode* t) {
	if (!t || !t->data.symbol.name)
		return; // No action if node or name is invalid

	Symbol* symbol = NULL;

	switch (t->kind) {
		case NODE_FUNCTION:
			// Check if the function is already declared
			if (findSymbol(currentScope, t->data.symbol.name)) {
				typeError(t, "Function already declared in this scope");
				return;
			}

			// Create and add function symbol
			symbol = createSymbol(t->data.symbol.name, SYMBOL_FUNCTION, t->data.symbol.type);
			symbol->sourceInfo.definedAt = t->lineNo;
			addSymbol(currentScope, symbol);
			addReference(symbol, t->lineNo);

			// Enter function scope
			enterScope(t->data.symbol.name);
			currentFunctionType = t->data.symbol.type->returnType;
			break;

		case NODE_VARIABLE:
			// Check if the variable is already declared in the current scope
			if (findSymbol(currentScope, t->data.symbol.name)) {
				typeError(t, "Variable already declared in this scope");
				return;
			}

			// Create and add variable/array symbol
			symbol = createSymbol(
				t->data.symbol.name,
				(t->data.symbol.type->baseType == TYPE_ARRAY) ? SYMBOL_ARRAY : SYMBOL_VARIABLE,
				t->data.symbol.type);
			symbol->sourceInfo.definedAt = t->lineNo;
			addSymbol(currentScope, symbol);
			addReference(symbol, t->lineNo);
			break;

		case NODE_PARAM:
			// Parameters should only be added within a function scope
			if (!currentScope->parent) {
				typeError(t, "Parameters should belong to a function scope");
				return;
			}

			// Create and add parameter symbol
			symbol = createSymbol(t->data.symbol.name, SYMBOL_PARAMETER, t->data.symbol.type);
			symbol->sourceInfo.definedAt = t->lineNo;
			addSymbol(currentScope, symbol);
			addReference(symbol, t->lineNo);
			break;

		case NODE_IDENTIFIER:
		case NODE_ASSIGN:
		case NODE_CALL:
			// Look up the symbol in the current scope
			symbol = findSymbol(currentScope, t->data.symbol.name);
			if (!symbol) {
				typeError(t, "Undeclared identifier");
				return;
			}

			// Add reference to the symbol
			addReference(symbol, t->lineNo);
			addSymbol(currentScope, symbol);
			t->data.symbol.type = symbol->type;
			break;

		default:
			break;
	}
}

void buildSymTab(ASTNode* syntaxTree) {
	globalScope = createScope("global", NULL);
	currentScope = globalScope;

	traverse(syntaxTree, insertNode, nullProc);

	currentScope = globalScope;
	if (TraceAnalyze)
		printSymbolTable(globalScope);
}

static void checkNode(ASTNode* t) {
	if (!t) return;

	Symbol* symbol = NULL;

	switch (t->kind) {
		case NODE_OPERATOR:
			// Check binary operations
			switch (t->data.operator) {
				case OP_PLUS:
				case OP_MINUS:
				case OP_TIMES:
				case OP_OVER:
					if (!checkBinaryOperands(t, createType(TYPE_INT))) {
						typeError(t, "Incompatible types for arithmetic operation");
					}
					// t->data.symbol.type->baseType = TYPE_INT;
					break;
				case OP_LT:
				case OP_GT:
				case OP_LEQ:
				case OP_GEQ:
				case OP_EQ:
				case OP_NEQ:
					if (!checkBinaryOperands(t, createType(TYPE_INT))) {
						typeError(t, "Incompatible types for relational operation");
					}
					// t->data.symbol.type->baseType = TYPE_BOOLEAN;
					break;

				default:
					break;
			}
			break;

		case NODE_FUNCTION:
			// Check if the function is already declared
				if (findSymbol(currentScope, t->data.symbol.name)) {
					typeError(t, "Function already declared in this scope");
					return;
				}

		// Create and add function symbol
		Symbol* functionSymbol = createSymbol(t->data.symbol.name, SYMBOL_FUNCTION, t->data.symbol.type);
		functionSymbol->sourceInfo.definedAt = t->lineNo;
		addSymbol(currentScope, functionSymbol);
		addReference(functionSymbol, t->lineNo);

		// Enter function scope
		enterScope(t->data.symbol.name);
		currentFunctionType = t->data.symbol.type->returnType;
		break;

		case NODE_IDENTIFIER:
			// Check if identifier exists in the current scope
			symbol = findSymbol(currentScope, t->data.symbol.name);
			if (!symbol) {
				typeError(t, "Undeclared identifier");
				return;
			}
			// Sync node type with symbol type
			t->data.symbol.type = symbol->type;
			addReference(symbol, t->lineNo);
			break;

		case NODE_CALL:
			// Check if function is declared
			symbol = findSymbol(currentScope, t->data.symbol.name);
			if (!symbol) {
				typeError(t, "Undeclared function");
				return;
			}
			if (symbol->kind != SYMBOL_FUNCTION) {
				typeError(t, "Attempting to call a non-function");
				return;
			}
			t->data.symbol.type = symbol->type;
			addReference(symbol, t->lineNo);
			break;

		case NODE_IF:
		case NODE_WHILE:
			// Ensure the condition node exists
			if (t->children[0]) {
				// Ensure the type is not NULL before accessing baseType
				if (t->children[0]->data.symbol.type) {
					if (t->children[0]->data.symbol.type->baseType != TYPE_BOOLEAN) {
						typeError(t, "Condition must be a boolean expression");
					}
				} else {
					typeError(t, "Condition has no valid type");
				}
			}
			break;

		case NODE_ASSIGN:
			// Check assignment compatibility
			if (t->children[0] && t->children[1]) {
				if (!checkBinaryOperands(t->children[0], t->children[1])) {
					typeError(t, "Type mismatch in assignment");
				}
			}
			break;

		case NODE_RETURN:
			// Check return type compatibility
			if (t->children[0] && currentFunctionType) {
				if (!areTypesCompatible(
						t->children[0]->data.symbol.type,
						currentFunctionType)) {
					typeError(t, "Return type does not match function declaration");
						}
			}
			break;

		default:
			break;
	}
}

void typeCheck(ASTNode* syntaxTree) {
	currentScope = globalScope;
	traverse(syntaxTree, nullProc, checkNode);
}