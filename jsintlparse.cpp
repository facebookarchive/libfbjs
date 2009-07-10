/**
* Copyright (c) 2008-2009 Facebook
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* See accompanying file LICENSE.txt.
*
* @author lklots
*/

/**
 * A simple script that parses out all tx() calls and verifies that the first
 * argument is a string literal.
 */
#include <stdio.h>
#include <iostream>
#include <set>
#include "node.hpp"
#include "parser.hpp"

using namespace std;
using namespace fbjs;

uint parse_intl_calls(Node* node) {
  if (node == NULL) {
    return 0;
  }

  if (dynamic_cast<NodeFunctionCall*>(node)) {
    assert(node->childNodes().size() == 2);
    NodeIdentifier* func =
      dynamic_cast<NodeIdentifier*>(node->childNodes().front());
    NodeArgList* args    =
      dynamic_cast<NodeArgList*>(node->childNodes().back());
    if (func && func->name() == "tx") {
      Node* arg = args->childNodes().front();
      if (!dynamic_cast<NodeStringLiteral*>(arg)) {
        cerr << "Illegal tx() invocation. The first argument to tx() has to "
             << "be a string literal, and not a variable or expression: `"
             << node->render() << "':" << endl;
        return func->lineno();
      }
    }
  }

  for (node_list_t::iterator ii = node->childNodes().begin();
       ii != node->childNodes().end(); ii++) {
    int error_line = parse_intl_calls(*ii);
    if (error_line) {
      return error_line;
    }
  }

  return 0;
}

void print_context(FILE* f, const string& filename, uint error_line) {
  rewind(f);
  char line[1024];
  uint counter = 0;
  while (fgets(line, sizeof(line), f)) {
    ++counter;
    if (counter >= error_line - 3 &&
        counter <= error_line + 3) {
      if (counter == error_line) {
        cerr << ">>>>> ";
      } else {
        cerr << "      ";
      }
      cerr << counter << "  " << string(line) << "\n";
    }
  }
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    cerr << string(argv[0]) << "takes at least one argument; a list of js files,"
         << "or `-` for stdin" << endl;
    return 1;
  }
  for (int i = 1; i < argc; i++) {
    string filename(argv[i]);
    FILE* f = NULL;
    (strcmp(argv[i], "-") == 0)
      ? f = stdin
      : f = fopen(filename.c_str(), "r");

    if (!f) {
      cerr << "failed to open " << filename << endl;
      return 1;
    }


    try {
      NodeProgram root(f);
      uint error_line = parse_intl_calls(&root);
      if (error_line) {
        print_context(f, filename, error_line);
        return error_line;
      }
    } catch (ParseException ex) {
      cerr << " parse error in `" << filename << "': " << ex.what() << endl;
      return 1;
    }

    fclose(f);
  }

  return 0;
}
