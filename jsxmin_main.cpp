#include "node.hpp"
#include "jsxmin_renaming.h"

#include <iostream>

using namespace std;
using namespace fbjs;

bool g_rename_globals;
bool g_rename_properties;
node_render_enum g_render_option;

static void jsxminify(FILE* js_file) {
  // Create a node. 
  NodeProgram root(js_file);
  
  // Starts in the global scope. 
  VariableRenaming variable_renaming(g_rename_globals);
  variable_renaming.process(&root);

  if (g_rename_properties) {
    PropertyRenaming property_renaming;
    property_renaming.process(&root);
  }

  cout << root.render(g_render_option).c_str();

  if (g_render_option == RENDER_PRETTY) {
    cout << "\n";
  }
}

int main(int argc, char* argv[]) {

  // Processing command-line options.
  // TODO: use some sort of option packages.
#ifdef DEBUG
  g_render_option = RENDER_PRETTY;
#else
  g_render_option = RENDER_NONE;
#endif
  g_rename_globals = false;
  g_rename_properties = false;

  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "--rename-globals") == 0) {
      g_rename_globals = true;

    } else if (strcmp(argv[i], "--rename-properties") == 0) {
      g_rename_properties = true;

    } else if (strcmp(argv[i], "--pretty") == 0) {
      g_render_option = RENDER_PRETTY;
    }
  }

  try {
    jsxminify(stdin);
  } catch (ParseException ex) {
    fprintf(stderr, "parsing error: %s\n", ex.what());
    return 1;
  }
  return 0;
}
