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
* @author Marcel Laverdet 
*/

#include "node.hpp"
#include <iostream>
#include <string.h>
using namespace fbjs;
using namespace std;

// Replaces instances of `needle` in `haystack` with `rep`.
Node* replace(Node* haystack, const Node* needle, const Node* rep) {
  if (haystack == NULL) {
    return NULL;
  } else if (*haystack == *needle) {
    delete haystack;
    return rep->clone();
  }

  Node* tmp;
  node_list_t::iterator ii = haystack->childNodes().begin();
  while (ii != haystack->childNodes().end()) {
    tmp = replace(*ii, needle, rep);
    if (tmp != (*ii)) {
      haystack->replaceChild(tmp, ii++);
    } else {
      ++ii;
    }
  }
  return haystack;
}

// Finds the first NodeExpression in a Node tree
const NodeExpression* findExpression(const Node* node) {
  if (dynamic_cast<const NodeExpression*>(node) != NULL) {
    return static_cast<const NodeExpression*>(node);
  }
  const NodeExpression* tmp;
  for (node_list_t::iterator ii = node->childNodes().begin(); ii != node->childNodes().end(); ++ii) {
    tmp = findExpression(*ii);
    if (tmp != NULL) {
      return tmp;
    }
  }
  return NULL;
}

int main(int argc, char* argv[]) {
  try {
    NodeProgram root(stdin); // parses
    bool optimize = false;
    bool crush = false;

    // Perform expression substitution
    for (int ii = 1; ii < argc; ++ii) {
      if (strcmp(argv[ii], "-r") == 0) {
        NodeProgram left(argv[++ii]);
        NodeProgram right(argv[++ii]);
        replace(&root, findExpression(&left), findExpression(&right));
      } else if (strcmp(argv[ii], "-o") == 0) {
        optimize = true;
      } else if (strcmp(argv[ii], "-c") == 0) {
        crush = true;
      }
    }

    // Simplify the AST
    if (optimize) {
      root.reduce();
    }

    // Print
    cout << root.render(crush ? RENDER_NONE : RENDER_PRETTY).c_str() << "\n";
    return 0;

  } catch (ParseException ex) {
    printf("Parse Error: %s", ex.what());
    return 1;
  }
}
