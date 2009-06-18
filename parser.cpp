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

void* fbjs_init_parser(fbjs_parse_extra* extra) {

  // Initialize the scanner.
  void* scanner;
  yylex_init_extra(extra, &scanner);
  extra->lineno = 1;
  extra->last_tok = 0;
  extra->last_paren_tok = 0;

  // Debug stuff
#ifdef DEBUG_BISON
  yydebug = 1;
#endif
#ifdef DEBUG_FLEX
  yyset_debug(1, scanner);
#endif

  return scanner;
}

void fbjs_cleanup_parser(fbjs_parse_extra* extra, void* scanner) {
  yylex_destroy(scanner);
  if (!extra->errors.empty()) {
    list<string>::iterator ii;
    for (ii = extra->errors.begin(); ii != extra->errors.end(); ++ii) {
      throw ParseException(*ii);
    }
  }
}

//
// Parse from a file
NodeProgram::NodeProgram(FILE* file) : Node(1) {
  fbjs_parse_extra extra;
  void* scanner = fbjs_init_parser(&extra);
  yyrestart(file, scanner); // read from file
  yyparse(scanner, this);
  fbjs_cleanup_parser(&extra, scanner);
}

//
// Parser from a string
NodeProgram::NodeProgram(const char* str) : Node(1) {
  fbjs_parse_extra extra;
  void* scanner = fbjs_init_parser(&extra);
  yy_scan_string(str, scanner); // read from string
  yyparse(scanner, this);
  fbjs_cleanup_parser(&extra, scanner);
}
