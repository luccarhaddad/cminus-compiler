#include "util.h"
#include "globals.h"

#include <log.h>
#include <stdlib.h>
#include <string.h>
#include <parser.h>

/* Procedure printToken prints a token
 * and its lexeme to the listing file
 */
void printToken(const TokenType token, const char* tokenString) {
	switch (token) {
		case IF:
		case ELSE:
		case INT:
		case RETURN:
		case VOID:
		case WHILE:
			pc("reserved word: %s\n", tokenString);
			break;
		case ASSIGN:
			pc("=\n");
			break;
		case OP_EQ:
			pc("==\n");
			break;
		case OP_NEQ:
			pc("!=\n");
			break;
		case OP_LT:
			pc("<\n");
			break;
		case OP_LEQ:
			pc("<=\n");
			break;
		case OP_GT:
			pc(">\n");
			break;
		case OP_GEQ:
			pc(">=\n");
			break;
		case LPAREN:
			pc("(\n");
			break;
		case RPAREN:
			pc(")\n");
			break;
		case LBRACKET:
			pc("[\n");
			break;
		case RBRACKET:
			pc("]\n");
			break;
		case LBRACE:
			pc("{\n");
			break;
		case RBRACE:
			pc("}\n");
			break;
		case SEMI:
			pc(";\n");
			break;
		case COMMA:
			pc(",\n");
			break;
		case PLUS:
			pc("+\n");
			break;
		case MINUS:
			pc("-\n");
			break;
		case TIMES:
			pc("*\n");
			break;
		case OVER:
			pc("/\n");
			break;
		case ENDFILE:
			pc("EOF\n");
			break;
		case NUM:
			pc("NUM, val= %s\n", tokenString);
			break;
		case ID:
			pc("ID, name= %s\n", tokenString);
			break;
		case ERROR:
			pce("ERROR: %s\n", tokenString);
			break;
		default:
			pce("Unknown token: %d\n", token);
	}
}

/* Function newStmtNode creates a new statement
 * node for syntax tree construction
 */
ASTNode* newStmtNode(const int kind) {
	ASTNode* node = createNode(kind);
	if (!node) {
		fprintf(stderr, "Out of memory error at line %d\n", lineno);
		return NULL;
	}
	node->lineNo = lineno;
	return node;
}

ASTNode* newExpNode(const int kind) {
	ASTNode* node = createNode(kind);
	if (!node) {
		fprintf(stderr, "Out of memory error at line %d\n", lineno);
		return NULL;
	}
	node->lineNo = lineno;
	return node;
}

/* Function copyString allocates and makes a new
 * copy of an existing string
 */
char* copyString(const char* s) {
	if (s == NULL) return NULL;
	const unsigned int n = strlen(s) + 1;
	char*     t = malloc(n);
	if (t == NULL)
		pce("Out of memory error at line %d\n", lineno);
	else
		strcpy(t, s);
	return t;
}

/* Variable indentno is used by printTree to
 * store current number of spaces to indent
 */
static int indentno = 0;

/* macros to increase/decrease indentation */
#define INDENT indentno += 4
#define UNINDENT indentno -= 4

/* printSpaces indents by printing spaces */
static void printSpaces(void) {
	for (int i = 0; i < indentno; i++)
		pc(" ");
}
/* Procedure printLine prints a full line
 * of the source code, with its number
 * reduntand_source is ANOTHER instance
 * of file pointer opened with the source code.
 */
void printLine() {
	static int currentLine = 0;
	static int firstCall   = 1;
	char       line[1024];

	if (firstCall) {
		rewind(redundant_source); // Restart reading from the beginning
		firstCall = 0;
	}

	const char* ret = fgets(line, sizeof(line), redundant_source);
	if (ret) {
		currentLine++;
		pc("%d: %s", currentLine, line);

		// Handle EOF condition
		if (feof(redundant_source)) {
			// If the line doesn't end with a newline, add one
			if (line[strlen(line) - 1] != '\n') {
				pc("\n");
			}
		}
	}
}

const char* ExpTypeToString(const TypeInfo* type) {
	switch (type->baseType) {
		case TYPE_VOID:    return "void";
		case TYPE_INT:     return "int";
		case TYPE_BOOLEAN: return "boolean";
		case TYPE_ARRAY:   return "array";
		default:           return "unknown";
	}
}

/* procedure printTree prints a syntax tree to the
 * listing file using indentation to indicate subtrees
 */
void printTree(const ASTNode* tree) {
	while (tree != NULL) {
		printSpaces();

		if (tree->kind) {
			switch (tree->kind) {
				case NODE_PROGRAM:
					pc("Program\n");
				break;
				case NODE_FUNCTION:
					pc("Declare function (return type \"%s\"): %s\n",
					   ExpTypeToString(tree->data.symbol.type->returnType), tree->data.symbol.name);
				break;
				case NODE_IF:
					pc("Conditional selection\n");
				break;
				case NODE_WHILE:
					pc("Iteration (loop)\n");
				break;
				case NODE_ASSIGN: {
					ASTNode* target = tree->children[0];

					if (target == NULL) {
						pc("Assign to: (unknown)\n");
						break;
					}

					if (target->kind == NODE_IDENTIFIER) {
						if (target->children[0] != NULL) {
							pc("Assign to array: %s\n", target->data.symbol.name);
							INDENT;
							printTree(target->children[0]);  // Print array index
							UNINDENT;
						} else {
							pc("Assign to var: %s\n", target->data.symbol.name);
						}
						// Only print the value being assigned, not the target identifier again
						INDENT;
						printTree(tree->children[1]);  // Print the value being assigned
						UNINDENT;
						tree = tree->next;
						continue;
					}
					break;
				}
				case NODE_PARAM:
					pc("Function param (%s %s): %s\n",
						ExpTypeToString(tree->data.symbol.type),
						(tree->data.symbol.type->arraySize >= 0 ? "array" : "var"),
						tree->data.symbol.name);
					break;
				case NODE_VARIABLE:
					pc("Declare %s %s: %s\n", ExpTypeToString(tree->data.symbol.type),
					   (tree->data.symbol.type->arraySize >= 0 ? "array" : "var"), tree->data.symbol.name);
					break;
				case NODE_OPERATOR:
					pc("Op: ");
					printToken(tree->data.operator, "\0");
				break;
				case NODE_CONSTANT:
					pc("Const: %d\n", tree->data.constValue);
				break;
				case NODE_IDENTIFIER:
					pc("Id: %s\n", tree->data.symbol.name);
				break;
				case NODE_CALL:
					pc("Function call: %s\n", tree->data.symbol.name);
				break;
				case NODE_RETURN:
					pc("Return\n");
				break;
				default:
					pce("Unknown ExpNode kind\n");
				break;
			}

			if (tree->kind != NODE_ASSIGN) {
				for (int i = 0; i < MAXCHILDREN; i++) {
					INDENT;
					printTree(tree->children[i]);
					UNINDENT;
				}
			}
			tree = tree->next;
		}
	}
}