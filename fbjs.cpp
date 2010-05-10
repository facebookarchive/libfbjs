#include "fbjs.hpp"
#include <typeinfo>

Node* fbjsize(Node* node, fbjsize_guts_t* guts) {
  if (node == NULL) {
    return node;
  }

  if (guts->scope.empty() && typeid(*node) == typeid(NodeVarDeclaration)) {

    // Var declarations at the root go away unless they have an initializer
    Node* ret_node = new NodeStatementList();
    node_list_t::iterator var = node->childNodes().begin();
    while (var != node->childNodes().end()) {
      if (typeid(**var) == typeid(NodeAssignment)) {
        ret_node->appendChild(fbjsize(node->removeChild(var++), guts));
      } else {
        ++var;
      }
    }
    delete node;
    return ret_node;

  } else if (typeid(*node) == typeid(NodeFunctionConstructor)) {

    // Turn `new foo(1, 2)' into: `fbjs.alloc(foo, 1, 2)'
    node_list_t::iterator func = node->childNodes().begin();
    Node* args = new NodeArgList();
    args->appendChild(node->removeChild(func++));
    node_list_t::iterator arg = (*func)->childNodes().begin();
    while (arg != (*func)->childNodes().end()) {
      args->appendChild((*func)->removeChild(arg++));
    }
    delete node;
    return (new NodeFunctionCall())
      ->appendChild((new NodeStaticMemberExpression())
        ->appendChild(new NodeIdentifier("fbjs"))
        ->appendChild(new NodeIdentifier("alloc")))
      ->appendChild(fbjsize(args, guts));

  } else if (typeid(*node) == typeid(NodeFunction)) {

    // For each function we enter we put a new scope on the stack with all its local variables
    bool root = guts->scope.empty();
    scope_t* scope = new scope_t();
    string* name = NULL;
    node_list_t::iterator func = node->childNodes().begin();

    // Function name
    if ((*func) != NULL) {
      name = new string(static_cast<NodeIdentifier*>(*func)->name());
    }
    if (!root && name != NULL) {
      scope->insert(*name);
    }

    // Formal parameters
    Node* args = *++func;
    for (node_list_t::iterator arg = (*func)->childNodes().begin(); (*func)->childNodes().end() != arg; ++arg) {
      scope->insert(static_cast<NodeIdentifier*>(*arg)->name());
    }

    // `var's defined inside the function body
    fbjs_analyze_scope(*++func, scope);

    // fbjsize the function body
    guts->scope.push_back(scope);
    Node* body = fbjsize(*func, guts);
    fbjsize(args, guts);
    assert(body == *func);
    guts->scope.pop_back();
    delete scope;

    // If it's a local function we can return it as is, if it's global there's other trickery
    Node* ret_node;
    if (root && name != NULL) {
      ret_node = (new NodeAssignment(ASSIGN))
        ->appendChild((new NodeStaticMemberExpression())
          ->appendChild((new NodeDynamicMemberExpression())
            ->appendChild((new NodeStaticMemberExpression())
              ->appendChild(new NodeIdentifier("fbjs"))
              ->appendChild(new NodeIdentifier("$")))
            ->appendChild(new NodeStringLiteral(guts->app_id, false)))
          ->appendChild(new NodeIdentifier(*name)))
        ->appendChild(node);
    } else {
      ret_node = node;
    }
    if (name != NULL) {
      delete name;
    }
    return ret_node;

  } else if (typeid(*node) == typeid(NodeIdentifier)) {

    // If the identifier is not in local scope we put it in our virtual global scope
    string name = static_cast<NodeIdentifier*>(node)->name();
    delete node;
    if (!fbjs_check_scope(name, &guts->scope)) {
      return (new NodeStaticMemberExpression())
        ->appendChild((new NodeDynamicMemberExpression())
          ->appendChild((new NodeStaticMemberExpression())
            ->appendChild(new NodeIdentifier("fbjs"))
            ->appendChild(new NodeIdentifier("$")))
          ->appendChild(new NodeStringLiteral(guts->app_id, false)))
        ->appendChild(new NodeIdentifier(name));
    } else {
      return (new NodeIdentifier("_$" + name));
    }

  } else if (typeid(*node) == typeid(NodeThis)) {

    // This one is always the same... fbjs.that(this)
    return (new NodeFunctionCall())
      ->appendChild((new NodeStaticMemberExpression())
        ->appendChild(new NodeIdentifier("fbjs"))
        ->appendChild(new NodeIdentifier("that")))
      ->appendChild((new NodeArgList())
        ->appendChild(node));

  } else if (typeid(*node) == typeid(NodeStaticMemberExpression) || typeid(*left) == typeid(NodeDynamicMemberExpression)) {

    // Given an expression: foo.bar, `property' is "bar"
    Node* property = node->removeChild(++node->childNodes().begin());
    if (typeid(*node) == typeid(NodeStaticMemberExpression)) {
      Node* tmp = property;
      property = new NodeStringLiteral(static_cast<NodeIdentifier*>(tmp)->name(), false);
      delete tmp;
    }

    // Get all the stuff we need out of the orginal node and delete it
    Node* identifier = fbjsize(node->removeChild(node->childNodes().begin()), guts);
    delete node;

    // Build a node that looks like: fbjs.get(foo, 'bar');
    return (new NodeFunctionCall())
      ->appendChild((new NodeStaticMemberExpression())
        ->appendChild(new NodeIdentifier("fbjs"))
        ->appendChild(new NodeIdentifier("get")))
      ->appendChild((new NodeArgList())
        ->appendChild(identifier)
        ->appendChild(property));

  } else if (typeid(*node) == typeid(NodeAssignment)) {

    // Assignments of the form foo.bar = 5
    Node* left = node->childNodes().front();
    if (typeid(*left) == typeid(NodeStaticMemberExpression) || typeid(*left) == typeid(NodeDynamicMemberExpression)) {

      // Given an expression: foo.bar = 5, `property' is "bar"
      Node* property = left->removeChild(++left->childNodes().begin());
      if (typeid(*left) == typeid(NodeStaticMemberExpression)) {
        Node* tmp = property;
        property = new NodeStringLiteral(static_cast<NodeIdentifier*>(tmp)->name(), false);
        delete tmp;
      }

      // Get all the stuff we need out of the orginal node and delete it
      node_assignment_t op = dynamic_cast<NodeAssignment*>(node)->operatorType();
      node_operator_t expr_op;
      Node* identifier = fbjsize(left->removeChild(left->childNodes().begin()), guts);
      Node* val = fbjsize(node->removeChild(++node->childNodes().begin()), guts);
      Node* val_placeholder;
      delete node;

      // Build a node that looks like: fbjs.set(foo, 'bar', <val_place_holder>);
      Node* ret_node = (new NodeFunctionCall())
        ->appendChild((new NodeStaticMemberExpression())
          ->appendChild(new NodeIdentifier("fbjs"))
          ->appendChild(new NodeIdentifier("set")))
        ->appendChild(val_placeholder = (new NodeArgList())
          ->appendChild(identifier)
          ->appendChild(property));

      // Compound assignments are a little tricky...
      // `foo.bar += 5' becomes: fbjs.set(foo, 'bar', fbjs.get(foo, 'bar') + 5);
      if (op != ASSIGN) {
        switch (op) {
          case MULT_ASSIGN:
            expr_op = MULT;
            break;

          case DIV_ASSIGN:
            expr_op = DIV;
            break;

          case MOD_ASSIGN:
            expr_op = MOD;
            break;

          case PLUS_ASSIGN:
            expr_op = PLUS;
            break;

          case MINUS_ASSIGN:
            expr_op = MINUS;
            break;

          case LSHIFT_ASSIGN:
            expr_op = LSHIFT;
            break;

          case RSHIFT_ASSIGN:
            expr_op = RSHIFT;
            break;

          case RSHIFT3_ASSIGN:
            expr_op = RSHIFT3;
            break;

          case BIT_AND_ASSIGN:
            expr_op = BIT_AND;
            break;

          case BIT_XOR_ASSIGN:
            expr_op = BIT_XOR;
            break;

          case BIT_OR_ASSIGN:
            expr_op = BIT_OR;
            break;

          default:
            assert(false);
            break;
        }

        // Create a node that looks like: fbjs.get(foo, 'bar') + 5
        val = (new NodeOperator(expr_op))
          ->appendChild((new NodeFunctionCall())
            ->appendChild((new NodeStaticMemberExpression())
              ->appendChild(new NodeIdentifier("fbjs"))
              ->appendChild(new NodeIdentifier("get")))
            ->appendChild((new NodeArgList())
              ->appendChild(identifier->clone())
              ->appendChild(property->clone())))
          ->appendChild(val);
      }

      // Add the return value to the fbjs.set node constructed above
      val_placeholder->appendChild(val);
      return ret_node;

    }
  }

  // Default behavior is to walk through all the children and fbjsize them.
  Node* tmp;
  for (node_list_t::iterator i = node->childNodes().begin(); i != node->childNodes().end(); i++) {

    // Recursively fbjsize
    tmp = fbjsize(*i, guts);
    if (tmp != (*i)) {
      node->replaceChild(tmp, i);
    }
  }
  return node;
}

void fbjs_analyze_scope(Node* node, scope_t* scope, const bool first /* = true */) {

  // Don't recurse into more functions
  if (!first && typeid(*node) == typeid(NodeFunction)) {
    return;
  }

  if (typeid(*node) == typeid(NodeVarDeclaration)) {

    // Mark all `var' declarations as local to this scope
    for (node_list_t::iterator i = node->childNodes().begin(); i != node->childNodes().end(); ++i) {
      if (typeid(**i) == typeid(NodeIdentifier)) {
        scope->insert(static_cast<NodeIdentifier*>(*i)->name());
      } else if (typeid(**i) == typeid(NodeAssignment)) {
        scope->insert(static_cast<NodeIdentifier*>((*i)->childNodes().front())->name());
      } else {
        assert(false);
      }
    }
  } else {

    // Iterate over all children and look for identifiers
    for (node_list_t::const_iterator i = node->childNodes().begin(); i != node->childNodes().end(); ++i) {
      fbjs_analyze_scope(*i, scope, false);
    }
  }
}

bool fbjs_check_scope(const string identifier, scope_stack_t* scope) {
  for (scope_stack_t::const_iterator i = scope->begin(); i != scope->end(); ++i) {
    if ((*i)->find(identifier) != (*i)->end()) {
      return true;
    }
  }
  return false;
}
