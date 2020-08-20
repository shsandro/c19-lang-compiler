%{

#include <cstdio>
#include <cstdlib>
  
#include "node.hpp"
  
  NBlock *programBlock;
  
  extern int yylex();
  void yyerror(const char *s) { std::printf("Error: %s\n", s); std::exit(1); }
  
%}

%union {
  NBlock *block;
  NExpression *expr;
  NStatement *stmt;
  NIdentifier *ident;
  std::vector<NExpression*> *exprvec;
  std::string *string;
  int token;
}

 /* 
  Definição dos símbolos terminais
 */
%token <string> TIDENTIFIER TINTEGER TDOUBLE
%token <token> TLPAREN TRPAREN TPRINT TASSIGN TSEMICOLON TCOMMA
%token <token> TPLUS TMINUS TMUL TDIV

 /* 
  Definição dos símbolos não terminais
 */
%type <ident> identifier
%type <expr> numeric expr
%type <block> program stmts
%type <stmt> stmt assign stdout
%type <exprvec> call_args

/*
 Precedência de operadores
 */
%left TPLUS TMINUS
%left TMUL TDIV

/*
 Definição do símbolo inicial
 */
%start program

%%

program : stmts { programBlock = $1; }
;

stmts : stmt       { $$ = new NBlock(); $$->statements.push_back($<stmt>1); }
      | stmts stmt { $1->statements.push_back($<stmt>2);                    }
;

stmt : stdout { $$ = $1; }
     | assign { $$ = $1; }
;

stdout : identifier TLPAREN call_args TRPAREN TSEMICOLON { $$ = new NPrintCall(*$1, *$3); }
;

assign : identifier TASSIGN expr TSEMICOLON { $$ = new NAssignment(*$<ident>1, *$3); }
;

identifier : TIDENTIFIER { $$ = new NIdentifier(*$1); }
;

expr : expr TMUL expr       { $$ = new NBinaryOperator(*$1, $2, *$3); }
     | expr TDIV expr       { $$ = new NBinaryOperator(*$1, $2, *$3); }
     | expr TPLUS expr      { $$ = new NBinaryOperator(*$1, $2, *$3); }
     | expr TMINUS expr     { $$ = new NBinaryOperator(*$1, $2, *$3); }
     | TLPAREN expr TRPAREN { $$ = $2;                                 }
     | identifier           { $<ident>$ = $1;                          }
     | numeric              { $$ = $1;                                 }
;

numeric : TINTEGER { $$ = new NInteger(atol($1->c_str())); }
        | TDOUBLE { $$ = new NDouble(atof($1->c_str()));   }
;

call_args :                        { $$ = new ExpressionList();                    }
          | expr                   { $$ = new ExpressionList(); $$->push_back($1); }
          | call_args TCOMMA expr  { $1->push_back($3);                            }
;

%%