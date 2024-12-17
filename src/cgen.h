#ifndef _CGEN_H_
#define _CGEN_H_
#include "ast.h"

/* Procedure codeGen generates code to a code
 * file by traversal of the syntax tree.
 */
void codeGen(ASTNode* syntaxTree);

#endif
