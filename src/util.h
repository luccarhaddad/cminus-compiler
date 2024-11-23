#ifndef _UTIL_H_
#define _UTIL_H_

#include "ast.h"
#include "globals.h"

/* Procedure printToken prints a token
 * and its lexeme to the listing file
 */
void printToken(TokenType token, const char* tokenString);

/* Function newStmtNode creates a new statement
 * node for syntax tree construction
 */
ASTNode* newStmtNode(int kind);

/* Function newExpNode creates a new expression
 * node for syntax tree construction
 */
ASTNode* newExpNode(int kind);

/* Function copyString allocates and makes a new
 * copy of an existing string
 */
char* copyString(const char* str);

void printLine();

/* procedure printTree prints a syntax tree to the
 * listing file using indentation to indicate subtrees
 */
void printTree(ASTNode*);

#endif
