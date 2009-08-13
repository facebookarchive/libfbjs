#include "node.hpp"
#include <stdio.h>
#include <iostream>
#include <map>
using namespace std;
using namespace fbjs;

typedef map<string, string> rename_t;

#define for_nodes(p, i) \
  for (node_list_t::iterator i  = (p)->childNodes().begin(); \
                             i != (p)->childNodes().end(); \
                           ++i)

void xminjs_build_scope(Node *node, rename_t &local_scope);

std::string xminjs_id(const char t, const rename_t &scope) {
  char buf[16];
  if (t) {
    snprintf(buf, 16, "_%c%x", t, (unsigned int)scope.size());
  } else {
    snprintf(buf, 16, "_%x", (unsigned int)scope.size());
  }
  return string(buf);
}

/**
 *  Rename identifiers to reduce wire size of a JS AST. The "use_local_scope"
 *  flag is not precisely intuitive: it's basically keeping track of whether
 *  we are in a property chain like "obj.prop.subprop".
 */
void xminjs_minify(Node*      node,
                   rename_t   &file_scope,
                   rename_t   &local_scope,
                   bool       use_local_scope,
                   bool       unsafe) {

  if (node == NULL) {
    return;
  }
  
  //  For "obj.property", we can only use rename the "obj" part with local
  //  scope replacements, not the "property" part.
  if (typeid(*node) == typeid(NodeStaticMemberExpression)) {
    for_nodes(node, ii) {
      if (ii == node->childNodes().begin()) {
        xminjs_minify(*ii, file_scope, local_scope, true, unsafe);
      } else {
        xminjs_minify(*ii, file_scope, local_scope, false, unsafe);
      }
    }
  } else if (typeid(*node) == typeid(NodeObjectLiteralProperty)) {
    //  For {prop: value}, we can't rename the property with local scope rules.
    xminjs_minify(node->childNodes().front(), file_scope, local_scope, false, unsafe);
    xminjs_minify(node->childNodes().back(), file_scope, local_scope, true, unsafe);
  } else if (typeid(*node) == typeid(NodeIdentifier)) {
    //  We've found an identifier. Rename it, if possible. The rule here is
    //  that it has to start with exactly one "_" to be a candidate for
    //  renaming.
    NodeIdentifier *n = static_cast<NodeIdentifier*>(node);
    if (unsafe &&
        n->name().length() > 1 && n->name()[0] == '_' && n->name()[1] != '_') {
      if (file_scope.find(n->name()) == file_scope.end()) {
        file_scope[n->name()] = xminjs_id(0, file_scope);
      }
      n->rename(file_scope[n->name()]);
    } else if (use_local_scope &&
               local_scope.find(n->name()) != local_scope.end()) {
      n->rename(local_scope[n->name()]);
    }
  } else if (typeid(*node) == typeid(NodeFunctionDeclaration) ||
             typeid(*node) == typeid(NodeFunctionExpression)) {
    
    node_list_t::iterator func = node->childNodes().begin();

    //  Skip function name.
    ++func;
    
    //  We're going to create a copy of the current local scope, then add all
    //  of the function's arguments and local variables to it, and then recurse
    //  into the tree using that as the new local scope. This could be made a
    //  little more efficient (but more complicated) by using a scope chain
    //  instead of making a copy.
    rename_t cur_scope(local_scope);

    //  First, add all the arguments to scope.
    for_nodes(*func, arg) {
      NodeIdentifier *arg_node = static_cast<NodeIdentifier*>(*arg);
      if (cur_scope.find(arg_node->name()) == cur_scope.end()) {
        cur_scope[arg_node->name()] = xminjs_id('L', cur_scope);
      }
    }
    
    //  Now, look ahead and find all the local varaible declarations and add
    //  them.
    xminjs_build_scope(*(++func), cur_scope);

    //  Finally, recurse with the new scope.
    for_nodes(node, ii) {
      xminjs_minify(*ii, file_scope, cur_scope, true, unsafe);
    }
  } else {
    for_nodes(node, ii) {
      xminjs_minify(*ii, file_scope, local_scope, true, unsafe);
    }
  }
}

void xminjs_build_scope(Node *node, rename_t &local_scope) {
  
  if (node == NULL) {
    return;
  }
  
  if (typeid(*node) == typeid(NodeFunctionExpression)) {
    return;
  } else if (typeid(*node) == typeid(NodeFunctionDeclaration)) {
    NodeIdentifier *decl_name = dynamic_cast<NodeIdentifier*>(node->childNodes().front());
    if (decl_name) {
      if (local_scope.find(decl_name->name()) == local_scope.end()) {
        local_scope[decl_name->name()] = xminjs_id('L', local_scope);
      }
    }
    return;
  } else if (typeid(*node) == typeid(NodeVarDeclaration)) {
    for_nodes(node, ii) {
      NodeIdentifier *n = dynamic_cast<NodeIdentifier*>(*ii);
      if (!n) {
        n = dynamic_cast<NodeIdentifier*>((*ii)->childNodes().front());
      }
      if (local_scope.find(n->name()) == local_scope.end()) {
        local_scope[n->name()] = xminjs_id('L', local_scope);
      }
    }
  } else {
    for_nodes(node, ii) {
      xminjs_build_scope(*ii, local_scope);
    }
  }
}


int main(int argc, char* argv[]) {
  NodeProgram root(stdin);
  
  rename_t file_scope;
  rename_t local_scope;
  
  bool unsafe = false;
  for (int ii = 1; ii < argc; ++ii) {
    if (strcmp(argv[ii], "--unsafe") == 0) {
      unsafe = true;
    }
  }
  
  xminjs_minify(&root, file_scope, local_scope, true, unsafe);

  cout << root.render(RENDER_NONE).c_str();
}
