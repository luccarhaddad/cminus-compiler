#include "analyze.h"
#include "globals.h"
#include "symtab.h"
#include <log.h>
#include <stdbool.h>
#include <stdlib.h>

static Scope*    currentScope        = NULL;
static Scope*    globalScope         = NULL;
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
			currentScope->children   = (Scope**) malloc(sizeof(Scope*));
			currentScope->childCount = 0;
		} else {
			currentScope->children = (Scope**) realloc(
			    currentScope->children, sizeof(Scope*) * (currentScope->childCount + 1));
		}
		currentScope->children[currentScope->childCount++] = newScope;
	}

	currentScope = newScope;
	if (!globalScope) globalScope = currentScope;
}

static void leaveScope(void) {
	if (currentScope) currentScope = currentScope->parent;
}

static void typeError(const ASTNode* t, const char* message) {
	pce("Type error at line %d: %s\n", t->lineNo, message);
	Error = TRUE;
}

static bool checkBinaryOperands(const ASTNode* t, const TypeInfo* expectedType) {
	if (!t || !expectedType) return false;

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
		case NODE_OPERATOR:
			leftType = createType(leftNode->resultType->baseType);
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
		case NODE_OPERATOR:
			rightType = createType(rightNode->resultType->baseType);
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
		typeError(t, "Right operand type does not match expected type");
		return false;
	}

	return true;
}


static void traverse(ASTNode* t, void (*preProc)(ASTNode*), void (*postProc)(ASTNode*)) {
	if (t) {
		preProc(t);
		for (int i = 0; i < MAXCHILDREN; i++) traverse(t->children[i], preProc, postProc);
		postProc(t);
		traverse(t->next, preProc, postProc);
	}
}

static void nullProc(ASTNode* t) {
	if (t->kind == SYMBOL_FUNCTION) enterScope(t->data.symbol.name);
}

static void insertNode(ASTNode* t) {
	if (!t || !t->data.symbol.name) return;

	Symbol* symbol = NULL;

	switch (t->kind) {
		case NODE_FUNCTION:
			if (findSymbol(currentScope, t->data.symbol.name)) {
				typeError(t, "Function already declared in this scope");
				return;
			}
			symbol = createSymbol(t->data.symbol.name, SYMBOL_FUNCTION, t->data.symbol.type);
			symbol->sourceInfo.definedAt = t->lineNo;
			addSymbol(currentScope, symbol);
			addReference(symbol, t->lineNo);
			enterScope(t->data.symbol.name);
			currentFunctionType = t->data.symbol.type->returnType;
			break;

		case NODE_VARIABLE:
			if (findSymbol(currentScope, t->data.symbol.name)) {
				typeError(t, "Variable already declared in this scope");
				return;
			}
			symbol =
			    createSymbol(t->data.symbol.name,
			                 t->data.symbol.type->arraySize >= 0 ? SYMBOL_ARRAY : SYMBOL_VARIABLE,
			                 t->data.symbol.type);
			symbol->sourceInfo.definedAt = t->lineNo;
			addSymbol(currentScope, symbol);
			addReference(symbol, t->lineNo);
			break;

		case NODE_PARAM:
			if (!currentScope->parent) {
				typeError(t, "Parameters should belong to a function scope");
				return;
			}
			symbol = createSymbol(t->data.symbol.name, SYMBOL_PARAMETER, t->data.symbol.type);
			symbol->sourceInfo.definedAt = t->lineNo;
			addSymbol(currentScope, symbol);
			addReference(symbol, t->lineNo);
			break;

		case NODE_IDENTIFIER:
		case NODE_ASSIGN:
		case NODE_CALL:
			symbol = findSymbol(currentScope, t->data.symbol.name);
			if (!symbol) {
				typeError(t, "Undeclared identifier");
				return;
			}
			addReference(symbol, t->lineNo);
			addSymbol(currentScope, symbol);
			t->data.symbol.type = symbol->type;
			break;

		default:
			break;
	}
}

void buildSymTab(ASTNode* syntaxTree) {
	globalScope  = createScope("global", NULL);
	currentScope = globalScope;
	addSymbol(globalScope, createSymbol("input", SYMBOL_FUNCTION, createType(TYPE_INT)));
	addSymbol(globalScope, createSymbol("output", SYMBOL_FUNCTION, createType(TYPE_VOID)));

	traverse(syntaxTree, insertNode, nullProc);

	currentScope = globalScope;
	if (TraceAnalyze) printSymbolTable(globalScope);
}

static void checkNode(ASTNode* t) {
	if (!t) return;

	Symbol* symbol = NULL;

	switch (t->kind) {
		case NODE_OPERATOR:
			switch (t->data.operator) {
				case OP_PLUS:
				case OP_MINUS:
				case OP_TIMES:
				case OP_OVER:
					if (!checkBinaryOperands(t, createType(TYPE_INT))) {
						typeError(t, "Incompatible types for arithmetic operation");
					}
					t->resultType = createType(TYPE_INT);
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
					t->resultType = createType(TYPE_BOOLEAN);
					break;

				default:
					break;
			}
			break;

		case NODE_FUNCTION:
			symbol = findSymbol(globalScope, t->data.symbol.name);
			if (!symbol) {
				typeError(t, "Function not declared");
				return;
			}
			if (currentScope != globalScope) leaveScope();
			enterScope(t->data.symbol.name);
			currentFunctionType = t->data.symbol.type->returnType;
			addReference(symbol, t->lineNo);
			break;

		case NODE_IDENTIFIER:
			symbol = findSymbol(currentScope, t->data.symbol.name);
			if (!symbol) {
				typeError(t, "Undeclared identifier");
				return;
			}
			t->data.symbol.type = symbol->type;
			addReference(symbol, t->lineNo);
			break;

		case NODE_CALL:
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
			if (t->children[0]) {
				if (t->children[0]->resultType->baseType != TYPE_BOOLEAN)
					typeError(t, "Condition must be a boolean expression");
			}
			break;

		case NODE_ASSIGN:
			if (t->children[0] && t->children[1]) {
				const ASTNode* leftNode  = t->children[0];
				const ASTNode* rightNode = t->children[1];

				const TypeInfo* leftType  = NULL;
				const TypeInfo* rightType = NULL;
				switch (leftNode->kind) {
					case NODE_CONSTANT:
						typeError(t, "Cannot assign to a constant");
						break;
					case NODE_VARIABLE:
					case NODE_IDENTIFIER:
						leftType = leftNode->data.symbol.type;
						break;
					default:
						typeError(t, "Invalid left-hand side in assignment");
						break;
				}

				switch (rightNode->kind) {
					case NODE_CONSTANT:
						rightType = createType(TYPE_INT);
						break;
					case NODE_VARIABLE:
					case NODE_IDENTIFIER:
						rightType = rightNode->data.symbol.type;
						break;
					case NODE_OPERATOR:
						switch (rightNode->data.operator) {
							case OP_PLUS:
							case OP_MINUS:
							case OP_TIMES:
							case OP_OVER:
								rightType = createType(rightNode->resultType->baseType);
								break;
							default:
								typeError(t, "Invalid right-side operator");
								break;
						}
						break;
					default:
						typeError(t, "Invalid right-side expression");
						break;
				}

				if (!leftType || !rightType || leftType->baseType != rightType->baseType) {
					typeError(t, "Incompatible types in assignment");
				}
			}
			break;

		case NODE_RETURN:
			if (t->children[0] && currentFunctionType) {
				if (!areTypesCompatible(t->children[0]->data.symbol.type, currentFunctionType)) {
					typeError(t, "Return type does not match function declaration");
				}
			}
			leaveScope();
			break;

		default:
			break;
	}
}

void typeCheck(ASTNode* syntaxTree) {
	currentScope        = globalScope;
	currentFunctionType = NULL;
	traverse(syntaxTree, nullProc, checkNode);
	currentScope = globalScope;
}