#include "node.hpp"
#include "jsxmin_renaming.h"
#include "jsxmin_reduction.h"

#include "gflags/gflags.h"

#include <iostream>

using namespace std;
using namespace fbjs;

DEFINE_bool(debug, false, "Turn on debug mode");
DEFINE_bool(pretty, false, "Pretty print the output");

static void jsxminify(NodeProgram* root) {
  // Code reduction should happen at the first.
  CodeReduction code_reduction;
  code_reduction.process(root);

  // Starts in the global scope. 
  VariableRenaming variable_renaming;
  variable_renaming.process(root);

  PropertyRenaming property_renaming;
  property_renaming.process(root);
}

int main(int argc, char* argv[]) {
  GFLAGS_INIT(argc, argv);

  try {
    // Create a node. 
    NodeProgram root(stdin);
    jsxminify(&root);
    cout << root.render(FLAGS_pretty ? RENDER_PRETTY : RENDER_NONE).c_str();

    if (FLAGS_pretty) {
      cout << "\n";
    }
  } catch (ParseException ex) {
    fprintf(stderr, "parsing error: %s\n", ex.what());
    return 1;
  }
  return 0;
}
