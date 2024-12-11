%{
#define YYPARSER /* distinguishes Yacc output from other code files */

#include "globals.h"
#include "types.h"
#include "ast.h"
#include "util.h"
#include "scan.h"
#include "parse.h"
#include "log.h"

#ifndef _PARSE_H_
#define _PARSE_H_

ASTNode* parse(void);

#endif

static char* savedName; /* for use in assignments */
static int savedLineNo;  /* ditto */
static int scopeId = 0;
static ASTNode* savedTree; /* stores syntax tree for later return */
static int yylex(void);
int yyerror(char *);

%}

%union {
    int val;
    char *name;
    TokenType token;
    ASTNode* node;
    Type type;
}

/* Token declaration */
%token <name> ID
%token <val>  NUM
%token <type> INT VOID
%token IF ELSE WHILE RETURN ERROR
%token PLUS MINUS TIMES OVER
%token LT GT LEQ GEQ EQ NEQ
%token ASSIGN
%token LPAREN RPAREN LBRACE RBRACE LBRACKET RBRACKET
%token SEMI COMMA

/* Type declarations */
%type <token> soma mult relacional
%type <node>  programa declaracao_lista declaracao
%type <node>  var_declaracao fun_declaracao
%type <node>  params param_lista param composto_decl
%type <node>  local_declaracoes statement_lista statement
%type <node>  expressao_decl selecao_decl iteracao_decl retorno_decl
%type <node>  expressao var simples_expressao soma_expressao
%type <node>  termo fator ativacao args arg_lista
%type <type>  tipo_especificador

%% /* Grammar rules for C- */

programa:
    declaracao_lista
        { savedTree = $1; }
    ;

declaracao_lista:
    declaracao_lista declaracao
        {
            ASTNode* t = $1;
            if(t != NULL) {
                while(t->next != NULL)
                    t = t->next;
                t->next = $2;
                $$ = $1;
            } else {
                $$ = $2;
            }
        }
    | declaracao
        { $$ = $1; }
    ;

declaracao:
    var_declaracao
        { $$ = $1; }
    | fun_declaracao
        { $$ = $1; }
    ;

var_declaracao:
    tipo_especificador ID SEMI
        { 
            $$ = createNode(NODE_VARIABLE);
            $$->data.symbol.name = $2;
            $$->data.symbol.type = createType($1);
        }
    | tipo_especificador ID LBRACKET NUM RBRACKET SEMI
        {
            $$ = createNode(NODE_VARIABLE);
            $$->data.symbol.name = $2;
            $$->data.symbol.type = createArrayType($1, $4);

            ASTNode* sizeNode = createNode(NODE_CONSTANT);
            sizeNode->data.constValue = $4;
            $$->children[0] = sizeNode;
        }
    ;

tipo_especificador:
    INT
        { $$ = TYPE_INT; }
    | VOID
        { $$ = TYPE_VOID; }
    ;

fun_declaracao:
    tipo_especificador ID { savedLineNo = lineno; savedName = $2; } LPAREN params RPAREN composto_decl
        {
            $$ = createNode(NODE_FUNCTION);
            $$->data.symbol.name = $2;
            $$->lineNo = savedLineNo;
            $$->data.symbol.type = createFunctionType(createType($1));
            $$->children[0] = $5; // Parameters
            $$->children[1] = $7; // Function body
        }
    ;

params:
    param_lista
        { $$ = $1; }
    | VOID
        { $$ = NULL; }
    ;

param_lista:
    param_lista COMMA param
        {
            ASTNode* t = $1;
            if (t != NULL) {
                while(t->next != NULL)
                    t = t->next;
                t->next = $3;
                $$ = $1;
            } else {
                $$ = $3;
            }
        }
    | param
        { $$ = $1; }
    ;

param:
    tipo_especificador ID
        {
            $$ = createNode(NODE_PARAM);
            $$->data.symbol.name = $2;
            $$->data.symbol.type = createType($1);
        }
    | tipo_especificador ID LBRACKET RBRACKET
        {
            $$ = createNode(NODE_PARAM);
            $$->data.symbol.name = $2;
            $$->data.symbol.type = createArrayType($1, 0);
        }
    ;

composto_decl:
    LBRACE local_declaracoes statement_lista RBRACE
        {
            $$ = createNode(NODE_BLOCK);

            char* scopeName = (char*)malloc(16);
            sprintf(scopeName, "%s", savedName);

            savedName = copyString(scopeName);
            $$->data.symbol.name = savedName;

            ASTNode* t = $2;
            if (t != NULL) {
                while(t->next != NULL)
                    t = t->next;
                t->next = $3;
                $$->children[0] = $2 ? $2 : $3;
            } else {
                $$->children[0] = $3;
            }
        }
    ;

local_declaracoes:
    local_declaracoes var_declaracao
        {
            ASTNode* t = $1;
            if (t != NULL) {
                while(t->next != NULL)
                    t = t->next;
                t->next = $2;
                $$ = $1;
            } else {
                $$ = $2;
            }
        }
    | %empty
        { $$ = NULL; }
    ;

statement_lista:
    statement_lista statement
        {
            ASTNode* t = $1;
            if (t != NULL) {
                while(t->next != NULL)
                    t = t->next;
                t->next = $2;
                $$ = $1;
            } else {
                $$ = $2;
            }
        }
    | %empty
        { $$ = NULL; }
    ;

statement:
    expressao_decl
        { $$ = $1; }
    | composto_decl
        { $$ = $1; }
    | selecao_decl
        { $$ = $1; }
    | iteracao_decl
        { $$ = $1; }
    | retorno_decl
        { $$ = $1; }
    ;

expressao_decl:
    expressao SEMI
        { $$ = $1; }
    | SEMI
        { $$ = NULL; }
    ;

selecao_decl:
    IF LPAREN expressao RPAREN statement
        { 
            $$ = createNode(NODE_IF);
            $$->children[0] = $3;
            $$->children[1] = $5;
        }
    | IF LPAREN expressao RPAREN statement ELSE statement
        {
            $$ = createNode(NODE_IF);
            $$->children[0] = $3;
            $$->children[1] = $5;
            $$->children[2] = $7;
        }
    ;

iteracao_decl:
    WHILE LPAREN expressao RPAREN statement
        {
            $$ = createNode(NODE_WHILE);
            $$->children[0] = $3;
            $$->children[1] = $5;
        }
    ;

retorno_decl:
    RETURN SEMI
        { $$ = createNode(NODE_RETURN); }
    | RETURN expressao SEMI
        { 
            $$ = createNode(NODE_RETURN);
            $$->children[0] = $2;
        }
    ;

expressao:
    var ASSIGN expressao
        {
            $$ = createNode(NODE_ASSIGN);
            $$->children[0] = $1;
            $$->children[1] = $3;
        }
    | simples_expressao
        { $$ = $1; }
    ;

var:
    ID
        {
            $$ = createNode(NODE_IDENTIFIER);
            $$->data.symbol.name = $1;
        }
    | ID LBRACKET expressao RBRACKET
        {
            $$ = createNode(NODE_IDENTIFIER);
            $$->data.symbol.name = $1;
            $$->children[0] = $3;
        }
    ;

simples_expressao:
    soma_expressao relacional soma_expressao
        { 
            $$ = createNode(NODE_OPERATOR);
            $$->children[0] = $1;
            $$->children[1] = $3;
            $$->data.operator = $2;
        }
    | soma_expressao
        { $$ = $1; }
    ;

relacional:
    LEQ { $$ = OP_LEQ; }
    | LT { $$ = OP_LT; }
    | GT { $$ = OP_GT; }
    | GEQ { $$ = OP_GEQ; }
    | EQ { $$ = OP_EQ; }
    | NEQ { $$ = OP_NEQ; }
    ;

soma_expressao:
    soma_expressao soma termo
        {
            $$ = createNode(NODE_OPERATOR);
            $$->children[0] = $1;
            $$->children[1] = $3;
            $$->data.operator = $2;
        }
    | termo
        { $$ = $1; }
    ;

soma:
    PLUS
        { $$ = OP_PLUS; }
    | MINUS
        { $$ = OP_MINUS; }
    ;

termo:
    termo mult fator
        { 
            $$ = createNode(NODE_OPERATOR);
            $$->children[0] = $1;
            $$->children[1] = $3;
            $$->data.operator = $2;
        }
    | fator
        { $$ = $1; }
    ;

mult:
    TIMES
        { $$ = OP_TIMES; }
    | OVER
        { $$ = OP_OVER; }
    ;

fator:
    LPAREN expressao RPAREN
        { $$ = $2; }
    | var
        { $$ = $1; }
    | ativacao
        { $$ = $1; }
    | NUM
        { 
            $$ = createNode(NODE_CONSTANT);
            $$->data.constValue = $1;
        }
    ;

ativacao:
    ID LPAREN args RPAREN
        { 
            $$ = createNode(NODE_CALL);
            $$->data.symbol.name = $1;
            $$->children[0] = $3;
        }
    ;

args:
    arg_lista
        { $$ = $1; }
    | %empty
        { $$ = NULL; }
    ;

arg_lista:
    arg_lista COMMA expressao
        {
            ASTNode* t = $1;
            if (t != NULL) {
                while(t->next != NULL)
                    t = t->next;
                t->next = $3;
                $$ = $1;
            } else {
                $$ = $3;
            }
        }
    | expressao
        { $$ = $1; }
    ;

%%

int yyerror(char * message)
{ pce("Syntax error at line %d: %s\n",lineno,message);
  pce("Current token: ");
  printToken(yychar,tokenString);
  Error = TRUE;
  return 0;
}

/* yylex calls getToken to make Yacc/Bison output
 * compatible with ealier versions of the TINY scanner
 */
static int yylex(void)
{ return getToken(); }

ASTNode* parse(void)
{ yyparse();
  return savedTree;
}

