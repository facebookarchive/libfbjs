#include "node.hpp"
#include "parser.hpp"
#ifdef DEBUG_BISON
extern int yydebug;
#endif
using namespace std;
using namespace fbjs;

ParseException::ParseException(const string msg) {
  memcpy(error, msg.c_str(), msg.length() > 127 ? 128 : msg.length() + 1);
  error[127] = 0;
}

const char* ParseException::what() const throw() {
  return error;
}

//
// Parse from a file
NodeProgram::NodeProgram(FILE* file) : Node(1) {

  // Initialize the scanner.
  void* scanner;
  fbjs_parse_extra extra;
  yylex_init_extra(&extra, &scanner);
  extra.lineno = 1;
  extra.last_tok = 0;
  extra.last_paren_tok = 0;
  extra.strstream = NULL;

  // Debug stuff
#ifdef DEBUG_BISON
  yydebug = 1;
#endif
#ifdef DEBUG_FLEX
  yyset_debug(1, scanner);
#endif

  // Parse
  yyrestart(file, scanner);
  yyparse(scanner, this);
  yylex_destroy(scanner);
  if (!extra.errors.empty()) {
    list<string>::iterator i;
    for (i = extra.errors.begin(); i != extra.errors.end(); ++i) {
      this->~Node();
      throw ParseException(*i);
    }
  }
}

//
// Parser from a string
NodeProgram::NodeProgram(const char* str) : Node(1) {

  // Initialize the scanner.
  void* scanner;
  fbjs_parse_extra extra;
  yylex_init_extra(&extra, &scanner);
  extra.lineno = 1;
  extra.last_tok = 0;
  extra.last_paren_tok = 0;
  extra.strstream = str;

  // Debug stuff
#ifdef DEBUG_BISON
  yydebug = 1;
#endif
#ifdef DEBUG_FLEX
  yyset_debug(1, scanner);
#endif

  // Parse
  yyparse(scanner, this);
  yylex_destroy(scanner);
  if (!extra.errors.empty()) {
    list<string>::iterator i;
    for (i = extra.errors.begin(); i != extra.errors.end(); ++i) {
      throw ParseException(*i);
    }
  }
}
