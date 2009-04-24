#include <stdio.h>
#include <iostream>
#include <set>
#include "node.hpp"
#include "parser.hpp"
using namespace std;
using namespace fbjs;

void exportsjs(Node* node, set<string>* globals) {
  if (node == NULL) {
    return;
  }
  if (typeid(*node) == typeid(NodeFunctionDeclaration)) {
    globals->insert(static_cast<NodeIdentifier*>(node->childNodes().front())->name());
    return; // don't enter functions because they don't have globals
  } else if (typeid(*node) == typeid(NodeFunctionExpression)) {
    return;
  } else if (typeid(*node) == typeid(NodeVarDeclaration)) {
    for (node_list_t::iterator ii = node->childNodes().begin(); ii != node->childNodes().end(); ii++) {
      if (typeid(**ii) == typeid(NodeIdentifier)) {
        globals->insert(static_cast<NodeIdentifier*>(*ii)->name());
      } else if (typeid(**ii) == typeid(NodeAssignment)) {
        globals->insert(static_cast<NodeIdentifier*>((*ii)->childNodes().front())->name());
      }
      exportsjs(*ii, globals);
    }
  }

  for (node_list_t::iterator ii = node->childNodes().begin(); ii != node->childNodes().end(); ii++) {
    exportsjs(*ii, globals);
  }
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    fprintf(stderr, "%s takes exactly one argument; a js file, or `-` for stdin\n", argv[0]);
    return 1;
  }

  FILE* f = strcmp(argv[1], "-") == 0 ? stdin : fopen(argv[1], "r");
  if (!f) {
    fprintf(stderr, "failed to open %s\n", argv[1]);
    return 1;
  }

  try {
    NodeProgram root(f);
    set<string> globals;
    exportsjs(&root, &globals);
    for (set<string>::iterator ii = globals.begin(); ii != globals.end(); ++ii) {
      printf("%s\n", (*ii).c_str());
    }
  } catch (ParseException ex) {
    fprintf(stderr, "parse error: %s\n", ex.what());
    return 1;
  }

  fclose(f);
}
