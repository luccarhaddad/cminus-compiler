#ifndef _ANALYZE_H_
#define _ANALYZE_H_

#define MAX_MEMORY 1023

#include "ast.h"
#include <string.h>

extern Scope* globalScope;
extern Scope* currentScope;

/* Function buildSymtab constructs the symbol
 * table by preorder traversal of the syntax tree
 */
void buildSymTab(ASTNode* syntaxTree);

/* Procedure typeCheck performs type checking
 * by a postorder syntax tree traversal
 */
void typeCheck(ASTNode* syntaxTree);

void enterScope(const char* name);
void leaveScope(ASTNode* t);

#endif
