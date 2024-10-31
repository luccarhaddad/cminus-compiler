%{
#define YYPARSER /* distinguishes Yacc output from other code files */

#include "globals.h"
#include "util.h"
#include "scan.h"
#include "parse.h"

#ifndef _PARSE_H_
#define _PARSE_H_

TreeNode* parse(void);

#endif

static char* savedName; /* for use in assignments */
static int savedLineNo;  /* ditto */
static TreeNode* savedTree; /* stores syntax tree for later return */
static int yylex(void);
int yyerror(char *);

%}

%union {
    int val;
    char *name;
    TokenType token;
    TreeNode* node;
    ExpType type;
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
            TreeNode* t = $1;
            if(t != NULL) {
                while(t->sibling != NULL)
                    t = t->sibling;
                t->sibling = $2;
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
            $$ = newStmtNode(VarK);
            $$->attr.name = $2;
            $$->type = $1;
        }
    | tipo_especificador ID LBRACKET NUM RBRACKET SEMI
        {
            $$ = newStmtNode(VarK);
            $$->attr.name = $2;
            $$->type = $1;
            $$->child[0] = newExpNode(ConstK);
            $$->child[0]->attr.val = $4;
        }
    ;

tipo_especificador:
    INT
        { $$ = Integer; }
    | VOID
        { $$ = Void; }
    ;

fun_declaracao:
    tipo_especificador ID LPAREN params RPAREN composto_decl
        {
            $$ = newStmtNode(FuncK);
            $$->attr.name = $2;
            $$->type = $1;
            $$->child[0] = $4;
            $$->child[1] = $6;
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
            TreeNode* t = $1;
            if (t != NULL) {
                while(t->sibling != NULL)
                    t = t->sibling;
                t->sibling = $3;
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
            $$ = newStmtNode(ParamK);
            $$->attr.name = $2;
            $$->type = $1;
            $$->isArray = 0;
        }
    | tipo_especificador ID LBRACKET RBRACKET
        {
            $$ = newStmtNode(ParamK);
            $$->attr.name = $2;
            $$->type = $1;
            $$->isArray = 1;
        }
    ;

composto_decl:
    LBRACE local_declaracoes statement_lista RBRACE
        {
            TreeNode* t = $2;
            if (t != NULL) {
                while(t->sibling != NULL)
                    t = t->sibling;
                t->sibling = $3;
                $$ = $2;
            } else {
                $$ = $3;
            }
        }
    ;

local_declaracoes:
    local_declaracoes var_declaracao
        {
            TreeNode* t = $1;
            if (t != NULL) {
                while(t->sibling != NULL)
                    t = t->sibling;
                t->sibling = $2;
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
            TreeNode* t = $1;
            if (t != NULL) {
                while(t->sibling != NULL)
                    t = t->sibling;
                t->sibling = $2;
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
            $$ = newStmtNode(IfK);
            $$->child[0] = $3;
            $$->child[1] = $5;
        }
    | IF LPAREN expressao RPAREN statement ELSE statement
        {
            $$ = newStmtNode(IfK);
            $$->child[0] = $3;
            $$->child[1] = $5;
            $$->child[2] = $7;
        }
    ;

iteracao_decl:
    WHILE LPAREN expressao RPAREN statement
        {
            $$ = newStmtNode(WhileK);
            $$->child[0] = $3;
            $$->child[1] = $5;
        }
    ;

retorno_decl:
    RETURN SEMI
        { $$ = newStmtNode(ReturnK); }
    | RETURN expressao SEMI
        { 
            $$ = newStmtNode(ReturnK);
            $$->child[0] = $2;
        }
    ;

expressao:
    var ASSIGN expressao
        {
            $$ = newStmtNode(AssignK);
            $$->child[0] = $1;
            $$->child[1] = $3;
        }
    | simples_expressao
        { $$ = $1; }
    ;

var:
    ID
        {
            $$ = newExpNode(IdK);
            $$->attr.name = $1;
        }
    | ID LBRACKET expressao RBRACKET
        {
            $$ = newExpNode(IdK);
            $$->attr.name = $1;
            $$->child[0] = $3;
        }
    ;

simples_expressao:
    soma_expressao relacional soma_expressao
        { 
            $$ = newExpNode(OpK);
            $$->child[0] = $1;
            $$->child[1] = $3;
            $$->attr.op = $2;
        }
    | soma_expressao
        { $$ = $1; }
    ;

relacional:
    LEQ
        { $$ = LEQ; }
    | LT
        { $$ = LT; }
    | GT
        { $$ = GT; }
    | GEQ
        { $$ = GEQ; }
    | EQ
        { $$ = EQ; }
    | NEQ
        { $$ = NEQ; }
    ;

soma_expressao:
    soma_expressao soma termo
        {
            $$ = newExpNode(OpK);
            $$->child[0] = $1;
            $$->child[1] = $3;
            $$->attr.op = $2;
        }
    | termo
        { $$ = $1; }
    ;

soma:
    PLUS
        { $$ = PLUS; }
    | MINUS
        { $$ = MINUS; }
    ;

termo:
    termo mult fator
        { 
            $$ = newExpNode(OpK);
            $$->child[0] = $1;
            $$->child[1] = $3;
            $$->attr.op = $2;
        }
    | fator
        { $$ = $1; }
    ;

mult:
    TIMES
        { $$ = TIMES; }
    | OVER
        { $$ = OVER; }
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
            $$ = newExpNode(ConstK);
            $$->attr.val = $1;
        }
    ;

ativacao:
    ID LPAREN args RPAREN
        { 
            $$ = newExpNode(CallK);
            $$->attr.name = $1;
            $$->child[0] = $3;
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
            TreeNode* t = $1;
            if (t != NULL) {
                while(t->sibling != NULL)
                    t = t->sibling;
                t->sibling = $3;
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

TreeNode * parse(void)
{ yyparse();
  return savedTree;
}

