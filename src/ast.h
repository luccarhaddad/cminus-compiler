//
// Created by Lucca Haddad on 22/11/24.
//

/* ast.h = Abstract Syntax Tree declarations */
#ifndef AST_H
#define AST_H
#include <symtab.h>
#include <types.h>

typedef struct ASTNode {
	enum {
		NODE_PROGRAM    = 1,
		NODE_FUNCTION   = 2,
		NODE_VARIABLE   = 3,
		NODE_IF         = 4,
		NODE_WHILE      = 5,
		NODE_RETURN     = 6,
		NODE_ASSIGN     = 7,
		NODE_CALL       = 8,
		NODE_OPERATOR   = 9,
		NODE_CONSTANT   = 10,
		NODE_IDENTIFIER = 11,
		NODE_PARAM      = 12,
		NODE_BLOCK      = 13
	} kind;

	union {
		struct {
			const char* name;
			TypeInfo*   type;
		} symbol;

		struct {
			enum {
				OP_PLUS  = 267,
				OP_MINUS = 268,
				OP_TIMES = 269,
				OP_OVER  = 270,
				OP_LT    = 271,
				OP_GT    = 272,
				OP_LEQ   = 273,
				OP_GEQ   = 274,
				OP_EQ    = 275,
				OP_NEQ   = 276
			} operator;

			int constValue;
		};
	} data;

	struct ASTNode* children[3];
	struct ASTNode* next;

	TypeInfo* resultType;
	Symbol*   symbol;
	int       lineNo;
} ASTNode;

/* AST functions */
ASTNode* createNode(int kind);
void     destroyNode(ASTNode* node);
void     addChild(ASTNode* parent, ASTNode* child);
void     addSibling(ASTNode* node, ASTNode* sibling);

#endif // AST_H
