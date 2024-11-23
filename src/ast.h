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
		NODE_PROGRAM,
		NODE_FUNCTION,
		NODE_VARIABLE,
		NODE_IF,
		NODE_WHILE,
		NODE_RETURN,
		NODE_ASSIGN,
		NODE_CALL,
		NODE_OPERATOR,
		NODE_CONSTANT,
		NODE_IDENTIFIER,
		NODE_PARAM
	} kind;

	union {
		struct {
			const char* name;
			TypeInfo*   type;
		} symbol;

		struct {
			enum {
				OP_PLUS,
				OP_MINUS,
				OP_TIMES,
				OP_OVER,
				OP_LT,
				OP_GT,
				OP_LEQ,
				OP_GEQ,
				OP_EQ,
				OP_NEQ
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
