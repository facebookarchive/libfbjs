#include "fbjs.hpp"
#include <typeinfo>

Node* fbjsize(Node* node, fbjsize_guts_t* guts) {
  if (node == NULL) {
    return node;
  }

  if (typeid(*node) == typeid(NodeFBJSShield)) {

    // Break the shield after an fbjsize pass
    Node* ret = node->removeChild(node->childNodes().begin());
    delete node;
    return ret;

  } else if (typeid(*node) == typeid(NodeVarDeclaration)) {

    // Special-case for `for (var ii in dict);'
    if (static_cast<NodeVarDeclaration*>(node)->iterator()) {
      Node* ret_node = node->removeChild(node->childNodes().begin());
      delete node;
      return fbjsize(ret_node, guts);
    }

    // Var declarations go away unless they have an initializer
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
    if (ret_node->empty()) {
      delete ret_node;
      return NULL;
    } else {
      return ret_node;
    }

  } else if (typeid(*node) == typeid(NodeFunctionConstructor)) {

    // Turn `new foo(1, 2)' into: `fbjs.alloc(foo, [1, 2])'
    node_list_t::iterator func = node->childNodes().begin();
    Node* args = new NodeArrayLiteral();
    Node* constructor = node->removeChild(func++);
    node_list_t::iterator arg = (*func)->childNodes().begin();
    while (arg != (*func)->childNodes().end()) {
      args->appendChild((*func)->removeChild(arg++));
    }
    delete node;
    return (new NodeFunctionCall())
      ->appendChild((new NodeStaticMemberExpression())
        ->appendChild(new NodeIdentifier("FBJS"))
        ->appendChild(new NodeIdentifier("alloc")))
      ->appendChild(fbjsize((new NodeArgList())
        ->appendChild(constructor)
        ->appendChild(args),
        guts));

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

    // Extract local `var's defined inside the function body
    Node* args = *++func;
    fbjs_analyze_scope(*++func, scope);
    Node* vars = new NodeVarDeclaration();
    for (scope_t::iterator scopei = scope->begin(); scopei != scope->end(); ++scopei) {
      vars->appendChild(new NodeIdentifier("_$" + *scopei));
    }

    // Formal parameters
    for (node_list_t::iterator arg = args->childNodes().begin(); args->childNodes().end() != arg; ++arg) {
      scope->insert(static_cast<NodeIdentifier*>(*arg)->name());
    }

    // fbjsize the function body
    guts->scope.push_back(scope);
    Node* body = fbjsize(*func, guts);
    fbjsize(args, guts);
    assert(body == *func);
    guts->scope.pop_back();
    delete scope;

    // Inject the `var' preface
    if (!vars->empty()) {
      body->insertBefore(vars, body->childNodes().begin());
    } else {
      delete vars;
    }

    // First we wrap the function in fbjs.ctx
    Node* ret_node = (new NodeFunctionCall())
      ->appendChild((new NodeStaticMemberExpression())
        ->appendChild(new NodeIdentifier("FBJS"))
        ->appendChild(new NodeIdentifier("ctx")))
      ->appendChild(args = (new NodeArgList())
        ->appendChild(node));
    if (name != NULL) {
      args->appendChild(new NodeStringLiteral(*name, false));
    }

    // If it's global we return $FBJS.name = function(){}.
    // If it's a local function we change the name to $_name.
    if (static_cast<NodeFunction*>(node)->declaration()) {
      delete node->replaceChild(NULL, node->childNodes().begin());
      if (root) {
        ret_node = (new NodeAssignment(ASSIGN))
          ->appendChild((new NodeStaticMemberExpression())
            ->appendChild(new NodeIdentifier("$FBJS"))
            ->appendChild(new NodeIdentifier(*name)))
          ->appendChild(ret_node);
      } else {
        ret_node = (new NodeAssignment(ASSIGN))
          ->appendChild(new NodeIdentifier("_$" + *name))
          ->appendChild(ret_node);
      }
    } else {
      if (name != NULL) {
        delete node->replaceChild(NULL, node->childNodes().begin());
      }
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
        ->appendChild(new NodeIdentifier("$FBJS"))
        ->appendChild(new NodeIdentifier(name));
    } else {
      return (new NodeIdentifier("_$" + name));
    }

  } else if (typeid(*node) == typeid(NodeThis)) {

    // FBJS.ctx should render this unnecessary
    return node;

    // This one is always the same... FBJS.that(this)
    return (new NodeFunctionCall())
      ->appendChild((new NodeStaticMemberExpression())
        ->appendChild(new NodeIdentifier("FBJS"))
        ->appendChild(new NodeIdentifier("that")))
      ->appendChild((new NodeArgList())
        ->appendChild(node));

  } else if (typeid(*node) == typeid(NodeFunctionCall)) {

    // If the subject of this function call is a MemberExpression then we want to mangle it
    Node* func = node->childNodes().front();
    Node* property = NULL;
    if (typeid(*func) == typeid(NodeStaticMemberExpression)) {
      // StaticMemberExpression has an identifier on the right so we cast it to string
      property = new NodeStringLiteral(static_cast<NodeIdentifier*>(func->childNodes().back())->name(), false);
    } else if (typeid(*func) == typeid(NodeDynamicMemberExpression)) {
      // DynamicMemberExpression already have an expression on the right which is what we want
      node_list_t::iterator expression = func->childNodes().end();
      property = fbjsize(func->removeChild(--expression), guts);
    }

    // property will be an Expression
    if (property != NULL) {
      Node* expression = fbjsize(func->removeChild(func->childNodes().begin()), guts);
      Node* retargs;
      Node* ret = (new NodeFunctionCall())
        ->appendChild((new NodeStaticMemberExpression())
          ->appendChild(new NodeIdentifier("FBJS"))
          ->appendChild(new NodeIdentifier("invoke")))
        ->appendChild(retargs = (new NodeArgList())
          ->appendChild(expression)
          ->appendChild(property));
      Node* args = node->childNodes().back();

      // The 3rd parameter to FBJS.invoke is an array of the original arguments, or nothing if there weren't any arguments.
      if (!args->empty()) {
        Node* args_array = new NodeArrayLiteral();
        node_list_t::iterator arg = args->childNodes().begin();
        while (arg != args->childNodes().end()) {
          args_array->appendChild(fbjsize(args->removeChild(arg++), guts));
        }
        retargs->appendChild(args_array);
      }
      delete node;
      return ret;
    }

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

    // Build a node that looks like: FBJS.get(foo, 'bar');
    return (new NodeFunctionCall())
      ->appendChild((new NodeStaticMemberExpression())
        ->appendChild(new NodeIdentifier("FBJS"))
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

      // Build a node that looks like: FBJS.set(foo, 'bar', <val_place_holder>);
      Node* ret_node = (new NodeFunctionCall())
        ->appendChild((new NodeStaticMemberExpression())
          ->appendChild(new NodeIdentifier("FBJS"))
          ->appendChild(new NodeIdentifier("set")))
        ->appendChild(val_placeholder = (new NodeArgList())
          ->appendChild(identifier)
          ->appendChild(property));

      // Compound assignments are a little tricky...
      // `foo.bar += 5' becomes: FBJS.set(foo, 'bar', FBJS.get(foo, 'bar') + 5);
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

        // Create a node that looks like: FBJS.get(foo, 'bar') + 5
        val = (new NodeOperator(expr_op))
          ->appendChild((new NodeFunctionCall())
            ->appendChild((new NodeStaticMemberExpression())
              ->appendChild(new NodeIdentifier("FBJS"))
              ->appendChild(new NodeIdentifier("get")))
            ->appendChild((new NodeArgList())
              ->appendChild(identifier->clone())
              ->appendChild(property->clone())))
          ->appendChild(val);
      }

      // Add the return value to the FBJS.set node constructed above
      val_placeholder->appendChild(val);
      return ret_node;

    }
  } else if (typeid(*node) == typeid(NodeObjectLiteralProperty)) {

    // ObjectLiteral keys pass through untouched
    return node;

  } else if (typeid(*node) == typeid(NodeForIn)) {

    // for(in) loops are really complicated...
    // You can iterate over DynamicMember expression which in turn may call a getter or a setter. Lot's of mangling has to
    // happen to get these working.
    //
    // for (foo in tar) console.log(foo);
    // -->
    // var fi0 = FBJS.keys(<tar>); for (<foo> in fi0) { if (fi0[<foo>] !== $dontEnum) console.log(<foo>); }
    //
    // for (foo.bar in tar) console.log(foo.bar);
    // -->
    // var fi0 = FBJS.keys(<tar>); for (fj0 in fi0) { if (fi0[fj0] !== $dontEnum) { <foo.bar> = fj0; <console.log(foo.bar)>; } }

    node_list_t::iterator loop = node->childNodes().begin();
    char buf[32];
    sprintf(buf, "fi%x", ++guts->forinid);
    Node* keys = new NodeIdentifier(buf);

    // This builds out the var fi0 = FBJS.keys(<tar>) part, and also replaces <tar> with fi0 in the for(in) loop
    Node* ret = (new NodeStatementList())
      ->appendChild((new NodeVarDeclaration())
        ->appendChild((new NodeAssignment(ASSIGN))
          ->appendChild(keys)
          ->appendChild((new NodeFunctionCall())
            ->appendChild((new NodeStaticMemberExpression())
              ->appendChild(new NodeIdentifier("FBJS"))
              ->appendChild(new NodeIdentifier("keys")))
            ->appendChild((new NodeArgList())
              ->appendChild(fbjsize(node->removeChild(++loop), guts))))))
      ->appendChild(node);
    loop = node->childNodes().begin(); ++loop;
    node->insertBefore(keys->clone(), loop++);
    loop = node->childNodes().end();
    node->appendChild(fbjsize(node->removeChild(--loop), guts));

    // If the iterator is a MemberExpression replace it with a placeholder.
    Node* itr = node->childNodes().front();
    if (typeid(*itr) == typeid(NodeStaticMemberExpression) || typeid(*itr) == typeid(NodeDynamicMemberExpression)) {

      // In case the for loop is something like for (i in o) console.log(o); we need to convert the Expression to a StatementList
      Node* body;
      if (typeid(*node->childNodes().back()) != typeid(NodeStatementList)) {
        node_list_t::iterator loop = node->childNodes().end();
        node->appendChild(body = (new NodeStatementList())
          ->appendChild(node->removeChild(--loop)));
      } else {
        body = node->childNodes().back();
      }

      // Now that we know the 3rd piece of the loop is a Block we can put in the iterator assignment
      sprintf(buf, "fj%x", guts->forinid);
      itr = new NodeIdentifier(buf);
      body->insertBefore(fbjsize((new NodeAssignment(ASSIGN))
          ->appendChild(node->removeChild(node->childNodes().begin()))
          ->appendChild((new NodeFBJSShield())
            ->appendChild(itr)), guts), body->childNodes().begin());
      node->insertBefore((new NodeVarDeclaration())->appendChild(itr->clone()), node->childNodes().begin());
    } else {

      // If not just go ahead fbjsize it now
      itr = fbjsize(node->removeChild(node->childNodes().begin()), guts);
      node->insertBefore(itr, node->childNodes().begin());
    }

    // Now we wrap the whole for body in an if...
    loop = node->childNodes().end();
    node->appendChild((new NodeIf())
      ->appendChild((new NodeOperator(STRICT_NOT_EQUAL))
        ->appendChild((new NodeDynamicMemberExpression())
          ->appendChild(keys->clone())
          ->appendChild(itr->clone()))
        ->appendChild(new NodeIdentifier("$dontEnum")))
      ->appendChild(node->removeChild(--loop))
      ->appendChild(NULL));
    return ret;
  }

  // Default behavior is to walk through all the children and fbjsize them.
  Node* tmp;
  for (node_list_t::iterator i = node->childNodes().begin(); i != node->childNodes().end(); i++) {

    // Recursively fbjsize
    tmp = fbjsize(*i, guts);
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

void fbjs_analyze_scope(Node* node, scope_t* scope, const bool first /* = true */) {

  if (!first && typeid(*node) == typeid(NodeFunction)) {

    // Don't recurse into more functions, but add local declarations to scope
    if (static_cast<NodeFunction*>(node)->declaration()) {
      scope->insert(static_cast<NodeIdentifier*>(*(node->childNodes().begin()))->name());
    }
  } else if (typeid(*node) == typeid(NodeVarDeclaration)) {

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
      if (*i != NULL) {
        fbjs_analyze_scope(*i, scope, false);
      }
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
