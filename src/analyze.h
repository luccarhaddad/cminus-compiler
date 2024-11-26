#ifndef _ANALYZE_H_
#define _ANALYZE_H_

#include "ast.h"
#include <string.h>

/* Function buildSymtab constructs the symbol
 * table by preorder traversal of the syntax tree
 */
void buildSymTab(ASTNode* syntaxTree);

/* Procedure typeCheck performs type checking
 * by a postorder syntax tree traversal
 */
void typeCheck(ASTNode* syntaxTree);

#endif
