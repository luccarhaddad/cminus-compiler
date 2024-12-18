#ifndef _GLOBALS_H_
#define _GLOBALS_H_

#include <stdio.h>

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

/* MAXRESERVED = the number of reserved words */
#define MAXRESERVED 6

// typedef enum {
//     // Keywords
//     ENDFILE, IF, ELSE, INT, RETURN, VOID, WHILE,
//     // Special symbols
//     ASSIGN, EQ, NEQ, LT, LEQ, GT, GEQ,
//     LPAREN, RPAREN, LBRACKET, RBRACKET, LBRACE, RBRACE,
//     SEMI, COMMA, PLUS, MINUS, TIMES, OVER,
//     // Other tokens
//     NUM, ID, ERROR
// } TokenType;

typedef int TokenType;

extern FILE* source;           /* source code text file */
extern FILE* redundant_source; /* source code text file */
extern FILE* listing;          /* listing output text file */
extern FILE* code;             /* code text file for TM simulator */

extern int lineno; /* source line number for listing */

/**************************************************/
/***********   Syntax tree for parsing ************/
/**************************************************/

#define MAXCHILDREN 3
//
// typedef struct treeNode {
// 	struct treeNode* child[MAXCHILDREN];
// 	struct treeNode* sibling;
// 	int              lineno;
// 	NodeKind         nodekind;
// 	union {
// 		StmtKind stmt;
// 		ExpKind  exp;
// 	} kind;
// 	union {
// 		TokenType op;
// 		int       val;
// 		char*     name;
// 	} attr;
// 	ExpType type; /* for type checking of exps */
// 	int     isArray;
// } TreeNode;

/**************************************************/
/***********   Flags for tracing       ************/
/**************************************************/

/* EchoSource = TRUE causes the source program to
 * be echoed to the listing file with line numbers
 * during parsing
 */
extern int EchoSource;

/* TraceScan = TRUE causes token information to be
 * printed to the listing file as each token is
 * recognized by the scanner
 */
extern int TraceScan;

/* TraceParse = TRUE causes the syntax tree to be
 * printed to the listing file in linearized form
 * (using indents for children)
 */
extern int TraceParse;

/* TraceAnalyze = TRUE causes symbol table inserts
 * and lookups to be reported to the listing file
 */
extern int TraceAnalyze;

/* TraceCode = TRUE causes comments to be written
 * to the TM code file as code is generated
 */
extern int TraceCode;

/* Error = TRUE prevents further passes if an error occurs */
extern int Error;

#ifndef YYPARSER
#define ENDFILE 0
#endif

#endif
