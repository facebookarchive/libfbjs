#pragma once
#include <stdio.h>
#include <stack>

//#define DEBUG_FLEX
//#define DEBUG_BISON

#define YY_EXTRA_TYPE fbjs_parse_extra*
#define YY_USER_INIT yylloc->first_line = 1

#include "node.hpp"
#include "parser.yacc.hpp"
struct fbjs_parse_extra {
  std::list<std::string> errors;
  std::stack<int> paren_stack;
  std::stack<int> curly_stack;
  int virtual_semicolon_last_state;
  int last_tok;
  int last_paren_tok;
  int last_curly_tok;
  int lineno;
  const char* strstream;
};

// Why the hell doesn't flex provide a header file?
// edit: actually I think it does I just can't find it on this damn system.
int yylex(YYSTYPE* param, YYLTYPE* yylloc, void* scanner);
int yylex_init_extra(YY_EXTRA_TYPE user_defined, void** scanner);
int yylex_destroy(void* scanner);
YY_EXTRA_TYPE yyget_extra(void* scanner);
void yyset_extra(YY_EXTRA_TYPE arbitrary_data, void* scanner);
void yyset_debug(int bdebug, void* yyscanner);
void yyrestart(FILE* input_file, void* yyscanner);
int yyparse(void* yyscanner, fbjs::Node* root);
#ifndef FLEX_SCANNER
void* yy_scan_string(const char *yy_str, void* yyscanner);
#endif
