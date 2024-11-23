/****************************************************/
/* File: cminus.l                                   */
/* Lex specification for C-                         */
/* Project for CES41: Compiladores                  */
/****************************************************/

%option noyywrap 
/* opção noyywrap pode ser necessária para novas versões do flex
  limitação: não compila mais de um arquivo fonte de uma só vez (não precisamos disso)
  https://stackoverflow.com/questions/1480138/undefined-reference-to-yylex 
*/


%{
#include "globals.h"
#include "util.h"
#include "scan.h"
#include "parser.h"
#include "log.h"
/* lexeme of identifier or reserved word */
char tokenString[MAXTOKENLEN+1];
%}

digit       [0-9]
number      {digit}+
letter      [a-zA-Z]
identifier  {letter}+
newline     \n|\r\n
whitespace  [ \t]+
comment     "/*"

%%

"else"          {return ELSE;}
"if"            {return IF;}
"int"           {return INT;}
"return"        {return RETURN;}
"void"          {return VOID;}
"while"         {return WHILE;}

"/*"            { char c;
                  while(1)
                  { 
                    c = input();
                    if(c == '*'){
                      c = input();
                      if(c=='/')
                        break;
                    }
                    if (c == EOF) break;
                    if (c == '\n') {lineno++;
                    printLine();
                    } 
                  } }
"+"             {return PLUS;}
"-"             {return MINUS;}
"*"             {return TIMES;}
"/"             {return OVER;}
"="             {return ASSIGN;}
"=="            {return EQ;}
"!="            {return NEQ;}
"<"             {return LT;}
"<="            {return LEQ;}
">"             {return GT;}
">="            {return GEQ;}
";"             {return SEMI;}
","             {return COMMA;}
"("             {return LPAREN;}
")"             {return RPAREN;}
"["             {return LBRACKET;}
"]"             {return RBRACKET;}
"{"             {return LBRACE;}
"}"             {return RBRACE;}


{number}        { yylval.val = atoi(yytext); return NUM; }
{identifier}    { yylval.name = copyString(yytext); return ID; }
{newline}       {lineno++;
                  printLine();}
{whitespace}    {/* skip whitespace */}
.               {return ERROR;}


%%

TokenType getToken(void) {
    static int firstTime = TRUE;
    static int lastPrintedLine = 0;
    TokenType currentToken;

    if (firstTime) {
        firstTime = FALSE;
        yyin = source;
        yyout = listing;
        lineno++; // Initialize lineno to 1
        printLine();
    }

    currentToken = yylex();
    strncpy(tokenString, yytext, MAXTOKENLEN);


    if (TraceScan) {
        pc("\t%d: ", lineno);
        printToken(currentToken, tokenString);
    }

    return currentToken;
}
