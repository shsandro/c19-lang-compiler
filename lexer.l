%{

#include<string>
#include "node.hpp" 
#include "parser.hpp"

#define SAVE_TOKEN yylval.string = new std::string(yytext, yyleng);
#define TOKEN(t) yylval.token = t

%}

%option noyywrap

%%

[a-zA-Z_][a-zA-Z0-9_]* SAVE_TOKEN; return TIDENTIFIER;
[0-9]+\.[0-9]*         SAVE_TOKEN; return TDOUBLE;
[0-9]+                 SAVE_TOKEN; return TINTEGER;
"="                    return TOKEN(TASSIGN);
"("                    return TOKEN(TLPAREN);
")"                    return TOKEN(TRPAREN);
"+"                    return TOKEN(TPLUS);
"-"                    return TOKEN(TMINUS);
"*"                    return TOKEN(TMUL);
"/"                    return TOKEN(TDIV);
";"                    return TOKEN(TSEMICOLON);
","                    return TOKEN(TCOMMA);
[ \t\n]                {};
.                      printf("Unknown token!\n"); yyterminate();

%%