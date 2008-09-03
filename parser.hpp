#pragma once
#include <stdio.h>
#include <stack>

//#define DEBUG_BISON
//#define DEBUG_FLEX

#include "node.hpp"
#include "parser.yacc.hpp"
struct fbjs_parse_extra {
  list<string> errors;
  stack<int> paren_stack;
  stack<int> curly_stack;
  int virtual_semicolon_last_state;
  int last_tok;
  int last_paren_tok;
  int last_curly_tok;
  int lineno;
};
#define YY_EXTRA_TYPE fbjs_parse_extra*
#define YY_USER_INIT yylloc->first_line = 1

// Why the hell doesn't flex provide a header file?
int yylex(YYSTYPE* param, YYLTYPE* yylloc, void* scanner);
int yylex_init_extra(YY_EXTRA_TYPE user_defined, void** scanner);
int yylex_destroy(void* scanner);
YY_EXTRA_TYPE yyget_extra(void* scanner);
void yyset_extra(YY_EXTRA_TYPE arbitrary_data, void* scanner);
void yyset_debug(int bdebug, void* yyscanner);
void yyrestart(FILE* input_file, void* yyscanner);

// Yawn.
int yyparse(void* yyscanner, Node* root);
