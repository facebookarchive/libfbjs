#include "troy.hpp"
#include <typeinfo>

int main(int argc, char* argv[]) {
  globals_set_t globals;
  node_list_t resources;
  node_list_t prelude;

  // Get resources etc from the command lines arguments
  for (int i = 1; i < argc; i++) {
    string arg = argv[i];

    // Parse a resource file
    if (arg == "-r") {
      for (++i; i < argc && argv[i][0] != '-'; i++) {
        try {
          resources.push_back(externalize(parseresource(argv[i])));
        } catch (string error) {
          cout << "Error parsing " << argv[i] << ": " << error << "\n";
          return 1;
        }
      }
      --i;

    // Parse a prelude file
    } else if (arg == "-p") {
      for (++i; i < argc && argv[i][0] != '-'; i++) {
        try {
          prelude.push_back(parseresource(argv[i]));
        } catch (string error) {
          cout << "Error parsing " << argv[i] << ": " << error << "\n";
          return 1;
        }
      }
      --i;

    // List of globals to export
    } else if (arg == "-g") {
      for (++i; i < argc && argv[i][0] != '-'; i++) {
        globals.insert(argv[i]);
      }
      --i;

    // Eh...
    } else {
      cout << "Unknown parameter: " << argv[i] << "\n";
      return 1;
    }
  }

  // Put the entire program body into a StatementList
  Node* body = new NodeStatementList();
  for (node_list_t::iterator i = resources.begin(); i != resources.end(); i++) {
    body->appendChild(*i);
  }
  for (globals_set_t::iterator i = globals.begin(); i != globals.end(); i++) {
    body->appendChild((new NodeAssignment(ASSIGN))
      ->appendChild((new NodeStaticMemberExpression())
        ->appendChild(new NodeThis())
        ->appendChild(new NodeIdentifier(*i)))
      ->appendChild(new NodeIdentifier(*i)));
  }

  // Put all of the above into an anonymous function, call $ctx() on it, and
  // then call the return value of that.
  body = (new NodeFunctionCall())
    ->appendChild((new NodeFunctionCall())
      ->appendChild(new NodeIdentifier("$ctx"))
      ->appendChild((new NodeArgList())
        ->appendChild((new NodeFunctionExpression())
          ->appendChild(NULL)
          ->appendChild(new NodeArgList())
          ->appendChild(body))))
    ->appendChild(new NodeArgList());

  // Put all of the above into another anonymous function, and insert prelude
  // as the first statements.
  Node* box = new NodeStatementList();
  for (node_list_t::iterator i = prelude.begin(); i != prelude.end(); i++) {
    box->appendChild(*i);
  }
  Node* root = (new NodeFunctionCall())
    ->appendChild((new NodeParenthetical()) // hack to cast from FunctionDeclaration to FunctionExpression
      ->appendChild((new NodeFunctionExpression())
        ->appendChild(NULL)
        ->appendChild(new NodeArgList())
        ->appendChild(box->appendChild(body))))
    ->appendChild(new NodeArgList());

  // Render it out and quit
  cout << root->render(RENDER_PRETTY).c_str();
  delete root;
  return 0;
}

Node* parseresource(char* filename) {
  FILE* file = fopen(filename, "r");
  if (!file) {
    throw string("Failed to open: ")+filename;
  }
  Node* root = new NodeProgram(file);
  fclose(file);
  return root;
}

Node* externalize(Node* node) {
  if (node == NULL) {
    return node;
  }

  if (typeid(*node) == typeid(NodeFunctionExpression) || typeid(*node) == typeid(NodeFunctionDeclaration)) {

    Node* func;
    Node* name = NULL;
    if (typeid(*node) == typeid(NodeFunctionDeclaration)) {

      // Cast FunctionDeclaration's to FunctionExpression
      name = node->removeChild(node->childNodes().begin());
      func = (new NodeFunctionExpression())
        ->appendChild(name)
        ->appendChild(node->removeChild(node->childNodes().begin()));
      func->appendChild(node->removeChild(node->childNodes().begin()));
      delete node;
    } else {
      func = node;
    }

    // Put all functions into the $ctx wrapper
    Node* ret = (new NodeFunctionCall())
      ->appendChild(new NodeIdentifier("$ctx"))
      ->appendChild((new NodeArgList())
        ->appendChild(func));

    // If the function was a declaration, it must now be a local variable.
    if (typeid(*node) == typeid(NodeFunctionDeclaration)) {
      ret = (new NodeVarDeclaration())
        ->appendChild((new NodeAssignment(ASSIGN))
          ->appendChild(name->clone())
          ->appendChild(ret));
    }
    return ret;
  }

  // Walk through the tree and externalize
  Node* tmp;
  for (node_list_t::iterator i = node->childNodes().begin(); i != node->childNodes().end(); i++) {
    tmp = externalize(*i);
    if (tmp != (*i)) {
      if (tmp == NULL) {
        node->removeChild(i);
      } else {
        node->replaceChild(tmp, i);
      }
    }
  }
  return node;
}
