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

  } else if (typeid(*node) == typeid(NodeProgram) || typeid(*node) == typeid(NodeFunctionDeclaration) || typeid(*node) == typeid(NodeFunctionExpression)) {

    // For each function we enter we put a new scope on the stack with all its local variables
    bool program = typeid(*node) == typeid(NodeProgram);
    scope_t scope;
    scope_t implied;
    Node* name = NULL;
    Node* statements = NULL;
    Node* args;

    if (!program) {

      // Grab the function's name
      node_list_t::iterator func = node->childNodes().begin();
      if ((*func) != NULL) {
        name = new NodeStringLiteral(static_cast<NodeIdentifier*>(*func)->name(), false);
      }

      // Formal parameters
      args = node->removeChild(++func);
      for (node_list_t::iterator arg = args->childNodes().begin(); args->childNodes().end() != arg; ++arg) {
        scope.insert(static_cast<NodeIdentifier*>(*arg)->name());
        implied.insert(static_cast<NodeIdentifier*>(*arg)->name());
      }
      statements = node->removeChild(++func);
      delete node;
    } else {
      statements = node->childNodes().front();
    }

    // Analyze local variables
    if (!program) {
      fbjs_analyze_scope(statements, &scope);
      guts->scope.push_back(&scope);
    }

    // Remove FunctionDeclaration (fbjs_declare_functions runs fbjsize on those)
    Node* vars;
    if (program) {
      vars = new NodeStatementList();
    } else {
      vars = new NodeVarDeclaration();
    }
    fbjs_declare_functions(statements, vars, &implied, guts);
    if (!program) {
      // Put each local variable that's also not a declaration into the var declaration
      for (scope_t::iterator ii = scope.begin(); ii != scope.end(); ++ii) {
        if (!implied.count(*ii)) {
          vars->appendChild(new NodeIdentifier("_$" + *ii));
        }
      }
    }

    // fbjsize everything else
    statements = fbjsize(statements, guts);
    if (!program) {
      args = fbjsize(args, guts);
      guts->scope.pop_back();
    }

    // Insert declarations at the beginning
    if (vars->childNodes().empty()) {
      delete vars;
    } else {
      if (statements->childNodes().empty()) {
        statements->appendChild(vars);
      } else {
        statements->insertBefore(vars, statements->childNodes().begin());
      }
    }

    // Abort early for programs, but there's more to do for functions
    if (program) {
      return node;
    }


    // Wrap the function in FBJS.ctx
    Node* ret;
    node = (new NodeFunctionExpression())
      ->appendChild(NULL)
      ->appendChild(args)
      ->appendChild(statements);
    if (name == NULL) {
      ret = fbjs_runtime_node("ctx", 1, node);
    } else {
      ret = fbjs_runtime_node("ctx", 2, node, name);
    }

    // If the function exists in a with() block we need to create a closure
    if (!guts->with_depth.empty()) {
      Node* arguments = new NodeArgList();
      for (size_t ii = guts->with_depth.size(); ii; --ii) {
        char buf[32];
        sprintf(buf, "ws%x", (unsigned int)(ii - 1));
        arguments->appendChild(new NodeIdentifier(buf));
      }
      ret = (new NodeFunctionCall())
        ->appendChild((new NodeFunctionExpression())
          ->appendChild(NULL)
          ->appendChild(arguments)
          ->appendChild((new NodeStatementList())
            ->appendChild((new NodeStatementWithExpression(RETURN))
              ->appendChild(ret))))
        ->appendChild(arguments->clone());
    }
    return ret;

  } else if (typeid(*node) == typeid(NodeVarDeclaration)) {

    // Special-case for `for (var ii in dict);'
    if (static_cast<NodeVarDeclaration*>(node)->iterator()) {
      Node* ret_node = node->removeChild(node->childNodes().begin());
      delete node;
      return fbjsize(ret_node, guts);
    }

    // Var declarations go away unless they have an initializer
    Node* ret_node = NULL;
    Node* tmp;
    node_list_t::iterator var = node->childNodes().begin();
    while (var != node->childNodes().end()) {
      if (typeid(**var) == typeid(NodeAssignment)) {
        tmp = fbjsize(node->removeChild(var++), guts);
        if (ret_node == NULL) {
          ret_node = tmp;
        } else {
          ret_node = (new NodeOperator(COMMA))->appendChild(ret_node)->appendChild(tmp);
        }
      } else {
        ++var;
      }
    }
    delete node;
    return ret_node;

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

  } else if (typeid(*node) == typeid(NodeWith)) {

    // Evaluate the with expression
    Node* ret = new NodeStatementList();
    char buf[32];
    sprintf(buf, "ws%x", (unsigned int)guts->with_depth.size());
    ret->appendChild((new NodeVarDeclaration())
      ->appendChild((new NodeAssignment(ASSIGN))
        ->appendChild(new NodeIdentifier(buf))
        ->appendChild(fbjsize(node->removeChild(node->childNodes().begin()), guts))));

    // Push a new (empty) scope onto the stack and fbjsize the with statement
    guts->with_depth.push_back(guts->scope.size());
    scope_t* scope = new scope_t();
    guts->scope.push_back(scope);
    ret->appendChild(fbjsize(node->removeChild(node->childNodes().begin()), guts));
    guts->with_depth.pop_back();
    guts->scope.pop_back();
    delete scope;
    delete node;
    return ret;

  } else if (typeid(*node) == typeid(NodeTry)) {

    // try \ catch blocks need a new scope for the catch block
    node_list_t::iterator lchild = node->childNodes().begin(), rchild = node->childNodes().begin();
    node->replaceChild(fbjsize(*lchild++, guts), rchild++);

    // Check for a catch block
    if (*lchild != NULL) {
      scope_t* scope = new scope_t();
      scope->insert(string(static_cast<NodeIdentifier*>(*lchild)->name()));
      guts->scope.push_back(scope);
      node->replaceChild(fbjsize(*lchild++, guts), rchild++);
      node->replaceChild(fbjsize(*lchild++, guts), rchild++);
      guts->scope.pop_back();
      delete scope;
    }

    // Check for a finally block
    if (*lchild != NULL) {
      node->replaceChild(fbjsize(*lchild, guts), rchild);
    }
    return node;

  } else if (typeid(*node) == typeid(NodeIdentifier)) {

    // If the identifier is not in local scope we put it in our virtual global scope
    string name = static_cast<NodeIdentifier*>(node)->name();
    delete node;
    if (guts->with_depth.empty()) {
      // In the typical case we can determine statically if this is a local variable
      Node* ret;
      if (!fbjs_check_scope(name, &guts->scope)) {
        ret = (new NodeStaticMemberExpression())
          ->appendChild(new NodeIdentifier("$FBJS"))
          ->appendChild(new NodeIdentifier(name));
      } else {
        ret = (new NodeIdentifier("_$" + name));
      }
      return ret;
    } else {
      // If we're inside a with block return FBJS.get(FBJS.with(...));
      return fbjs_runtime_node("get", 2, fbjs_with_identifier(name, false, guts), new NodeStringLiteral(name, false));
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

  } else if (typeid(*node) == typeid(NodeUnary)) {

    node_unary_t op = static_cast<NodeUnary*>(node)->operatorType();

    // Turn `delete` into FBJS.delete
    if (op == DELETE) {
      Node* expr = node->childNodes().front();
      if (typeid(*expr) == typeid(NodeIdentifier)) {
        // TODO: Check for with() blocks
        delete node;
        return new NodeBooleanLiteral(true);
      } else {
        node_pair_t expr = fbjs_split_member_expression(node->childNodes().front(), guts);
        if (expr.first != NULL) {
          delete node;
          return (new NodeFunctionCall())
            ->appendChild((new NodeStaticMemberExpression())
              ->appendChild(new NodeIdentifier("FBJS"))
              ->appendChild(new NodeIdentifier("expunge")))
            ->appendChild((new NodeArgList())
              ->appendChild(expr.first)
              ->appendChild(expr.second));
        }
      }
    } else if (op == INCR_UNARY || op == DECR_UNARY) {

      // If (in|de)crementing a property expression we need to call FBJS.set instead
      Node* expr = node->childNodes().front();
      if (typeid(*expr) == typeid(NodeStaticMemberExpression) || typeid(*expr) == typeid(NodeDynamicMemberExpression) || !guts->with_depth.empty()) {
        node->removeChild(node->childNodes().begin());
        delete node;
        return fbjsize((new NodeAssignment(ASSIGN))
          ->appendChild(expr)
          ->appendChild((new NodeOperator(op == INCR_UNARY ? PLUS : MINUS))
            ->appendChild(expr->clone())
            ->appendChild(new NodeNumericLiteral(1))), guts);
      }
    }

  } else if (typeid(*node) == typeid(NodePostfix)) {

    // If (in|de)crementing a property expression we need to call FBJS.set instead, with a tmp variable since it's postfix
    Node* expr = node->childNodes().front();
    if (typeid(*expr) == typeid(NodeStaticMemberExpression) || typeid(*expr) == typeid(NodeDynamicMemberExpression) || !guts->with_depth.empty()) {
      node->removeChild(node->childNodes().begin());
      delete node;
      Node* tmp = (new NodeFBJSShield())->appendChild(new NodeIdentifier("$tmp"));
       return fbjsize((new NodeParenthetical())
        ->appendChild((new NodeOperator(COMMA))
          ->appendChild((new NodeAssignment(ASSIGN))
            ->appendChild(tmp)
            ->appendChild((new NodeOperator(PLUS))
              ->appendChild(expr)
              ->appendChild(new NodeNumericLiteral(0))))
          ->appendChild((new NodeOperator(COMMA))
            ->appendChild((new NodeAssignment(ASSIGN))
              ->appendChild(expr->clone())
              ->appendChild((new NodeOperator(PLUS))
                ->appendChild(tmp->clone())
                ->appendChild(new NodeNumericLiteral(1))))
            ->appendChild(tmp->clone()))), guts);
    }

  } else if (typeid(*node) == typeid(NodeFunctionCall)) {

    // If the subject of this function call is a MemberExpression then we want to mangle it
    Node* func = node->childNodes().front();
    node_pair_t expr = fbjs_split_member_expression(func, guts);

    // property will be an Expression
    if (expr.first != NULL) {
      Node* retargs;
      Node* ret = (new NodeFunctionCall())
        ->appendChild((new NodeStaticMemberExpression())
          ->appendChild(new NodeIdentifier("FBJS"))
          ->appendChild(new NodeIdentifier("invoke")))
        ->appendChild(retargs = (new NodeArgList())
          ->appendChild(expr.first)
          ->appendChild(expr.second));
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

  } else if (typeid(*node) == typeid(NodeStaticMemberExpression) || typeid(*node) == typeid(NodeDynamicMemberExpression)) {

    // Build a node that looks like: FBJS.get(foo, 'bar');
    node_pair_t expr = fbjs_split_member_expression(node, guts);
    delete node;
    return fbjs_runtime_node("get", 2, expr.first, expr.second);

  } else if (typeid(*node) == typeid(NodeAssignment)) {

    // If we're in a with block this is a nightmare.
    Node* var = NULL;
    if (!guts->with_depth.empty() && typeid(*node->childNodes().front()) == typeid(NodeIdentifier)) {
      if (fbjs_check_scope(static_cast<NodeIdentifier*>(node->childNodes().front())->name(), &guts->scope)) {
        var = node->childNodes().front()->clone();
      }
    }

    // Look for assignments of the form foo.bar = 5
    node_pair_t expr = fbjs_split_member_expression(node->childNodes().front(), guts);
    if (expr.first != NULL) {

      // Get all the stuff we need out of the orginal node and delete it
      node_assignment_t op = dynamic_cast<NodeAssignment*>(node)->operatorType();
      Node* val = fbjsize(node->removeChild(++node->childNodes().begin()), guts);
      delete node;

      // Compound assignments are a little tricky...
      // `foo.bar += 5' becomes: FBJS.set(foo, 'bar', FBJS.get(foo, 'bar') + 5);
      if (op != ASSIGN) {

        // Create a node that looks like: FBJS.get(foo, 'bar') + 5
        val = (new NodeOperator(fbjs_assignment_op_to_expr_op(op)))
          ->appendChild(fbjs_runtime_node("get", 2, expr.first->clone(), expr.second->clone()))
          ->appendChild(val);
      }

      if (var != NULL) {
        // Return something like:
        // ($tmp = bar, FBJS.with([$FBJS], false, "foo") === false ? foo = $tmp : FBJS.set(FBJS.with([$FBJS], $FBJS, "foo"), "foo", $tmp));
        return (new NodeParenthetical())
          ->appendChild((new NodeOperator(COMMA))
            ->appendChild((new NodeAssignment(ASSIGN))
              ->appendChild(new NodeIdentifier("$tmp"))
              ->appendChild(val))
            ->appendChild((new NodeConditionalExpression())
              ->appendChild((new NodeOperator(STRICT_EQUAL))
                ->appendChild(fbjs_with_identifier(static_cast<NodeIdentifier*>(var)->name(), true, guts))
                ->appendChild(new NodeBooleanLiteral(false)))
              ->appendChild((new NodeAssignment(ASSIGN))
                ->appendChild(new NodeIdentifier("_$" + static_cast<NodeIdentifier*>(var)->name()))
                ->appendChild(new NodeIdentifier("$tmp")))
              ->appendChild(fbjs_runtime_node("set", 3, expr.first, expr.second, new NodeIdentifier("$tmp")))));
      } else {
        // Return a call to FBJS.set
        return fbjs_runtime_node("set", 3, expr.first, expr.second, val);
      }
    }
    assert(var == NULL);

  } else if (typeid(*node) == typeid(NodeObjectLiteralProperty)) {

    // ObjectLiteral keys pass through untouched
    Node* val = node->removeChild(++node->childNodes().begin());
    return node->appendChild(fbjsize(val, guts));

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

Node* NodeFBJSShield::clone(Node* node) const {
  return Node::clone(new NodeFBJSShield());
}

Node* fbjs_runtime_node(const char* fn, const unsigned int num, ...) {
  Node* args_node;
  Node* ret = (new NodeFunctionCall())
    ->appendChild((new NodeStaticMemberExpression())
      ->appendChild(new NodeIdentifier("FBJS"))
      ->appendChild(new NodeIdentifier(fn)))
    ->appendChild(args_node = new NodeArgList());

  va_list args;
  va_start(args, num);
  for (unsigned int i  = 0; i < num; ++i) {
    args_node->appendChild(va_arg(args, Node*));
  }
  va_end(args);
  return ret;
}

Node* fbjs_with_identifier(string name, bool for_assignment, fbjsize_guts_t* guts) {

  size_t si = guts->scope.size(),
    wi = guts->with_depth.size();
  bool found_local = false;
  Node* local = NULL;
  vector<Node*> scopes;
  while (si) {
    // Walk up the stack and look for a local variable that matches
    --si;
    if (guts->scope[si]->count(name)) {
      // On the first local variable match we create an object literal with one property.
      // This will act as a fake scope to pass to FBJS.with. Also finding a local variable
      // means we can stop walking up the stack since we've reached the last possible identifier.
      found_local = true;
      if (!for_assignment) {
        scopes.push_back(local = (new NodeObjectLiteral())
          ->appendChild((new NodeObjectLiteralProperty())
            ->appendChild(new NodeStringLiteral(name, false))
            ->appendChild(new NodeIdentifier("_$" + name))));
      }
      break;
    } else if (wi && guts->with_depth[wi - 1] == si) {
      // If this scope is one that was inserted by the NodeWith branch then that means we need
      // to check that with scope.
      --wi;
      char buf[32];
      sprintf(buf, "ws%x", (unsigned int)wi);
      scopes.push_back(new NodeIdentifier(buf));
    }
  }

  // If we didn't find a local variable, the last possible scope is the global scope.
  if (!found_local) {
    scopes.push_back(local = new NodeIdentifier("$FBJS"));
  }

  // Go through the scope vector in reverse and create an array literal with all scopes to search.
  Node* args = new NodeArrayLiteral();
  for (size_t ii = scopes.size(); ii; --ii) {
    args->appendChild(scopes[ii - 1]);
  }

  // Return a call to FBJS.with
  return fbjs_runtime_node("scope", 3, args, local == NULL ? new NodeBooleanLiteral(false) : local->clone(), new NodeStringLiteral(name, false));
}

node_pair_t fbjs_split_member_expression(Node* expr, fbjsize_guts_t* guts) {
  node_pair_t ret;
  if (typeid(*expr) == typeid(NodeStaticMemberExpression)) {
    // StaticMemberExpression has an identifier on the right so we cast it to string
    ret.first = fbjsize(expr->removeChild(expr->childNodes().begin()), guts);
    ret.second = new NodeStringLiteral(static_cast<NodeIdentifier*>(expr->childNodes().back())->name(), false);
  } else if (typeid(*expr) == typeid(NodeDynamicMemberExpression)) {
    // DynamicMemberExpression already have an expression on the right which is what we want
    ret.first = fbjsize(expr->removeChild(expr->childNodes().begin()), guts);
    ret.second = fbjsize(expr->removeChild(expr->childNodes().begin()), guts);
  } else if (typeid(*expr) == typeid(NodeIdentifier) && !guts->with_depth.empty()) {
    // In the case of an identifier within a with block FBJS.with returns FBJS.get. This is pretty hacky
    // but seems like the best or easiest solution at this point.
    Node* fbjs_expr = fbjsize(expr->clone(), guts);
    assert(typeid(*fbjs_expr) == typeid(NodeFunctionCall));
    Node* args = fbjs_expr->childNodes().back();
    ret.first = args->removeChild(args->childNodes().begin()); // will be FBJS.with(...);
    ret.second = args->removeChild(args->childNodes().begin()); // will be a string
    delete fbjs_expr;
  } else {
    ret.first = ret.second = NULL;
  }
  return ret;
}

node_operator_t fbjs_assignment_op_to_expr_op(node_assignment_t op) {
  switch (op) {
    case MULT_ASSIGN:
      return MULT;
    case DIV_ASSIGN:
      return DIV;
    case MOD_ASSIGN:
      return MOD;
    case PLUS_ASSIGN:
      return PLUS;
    case MINUS_ASSIGN:
      return MINUS;
    case LSHIFT_ASSIGN:
      return LSHIFT;
    case RSHIFT_ASSIGN:
      return RSHIFT;
    case RSHIFT3_ASSIGN:
      return RSHIFT3;
    case BIT_AND_ASSIGN:
      return BIT_AND;
    case BIT_XOR_ASSIGN:
      return BIT_XOR;
    case BIT_OR_ASSIGN:
      return BIT_OR;
    default:
      return EQUAL;
  }
}

void fbjs_analyze_scope(Node* node, scope_t* scope) {

  if (typeid(*node) == typeid(NodeFunctionDeclaration)) {

    // Don't recurse into more functions, but add local declarations to scope
    // NOTE: We only look at NodeFunctionDeclaration because while NodeFunctionExpression's
    //       can have names, they don't actually do much.
    scope->insert(static_cast<NodeIdentifier*>(*(node->childNodes().begin()))->name());
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
        fbjs_analyze_scope(*i, scope);
      }
    }
  }
}

bool fbjs_declare_functions(Node* node, Node* decl, scope_t* scope, fbjsize_guts_t* guts) {

  if (typeid(*node) == typeid(NodeFunctionDeclaration)) {

    // Change this to an Assignment and FunctionExpression, then fbjsize that
    Node* identifier = node->removeChild(node->childNodes().begin());
    scope->insert(static_cast<NodeIdentifier*>(identifier)->name());
    Node* func = (new NodeFunctionExpression())
      ->appendChild(NULL)
      ->appendChild(node->removeChild(node->childNodes().begin()));
    func->appendChild(node->removeChild(node->childNodes().begin()));
    decl->appendChild(fbjsize((new NodeAssignment(ASSIGN))
      ->appendChild(identifier)
      ->appendChild(func), guts));
    delete node;
    return false;

  } else if (typeid(*node) == typeid(NodeFunctionExpression)) {

    // Leave FunctionExpression's alone and also don't recurse into them
    return true;
  } else {

    // Iterate over all children, remove declared functions
    node_list_t::iterator ii = node->childNodes().begin();
    while (ii != node->childNodes().end()) {
      if (*ii != NULL && !fbjs_declare_functions(*ii, decl, scope, guts)) {
        node->removeChild(ii++);
      } else {
        ++ii;
      }
    }
    return true;
  }
}

bool fbjs_check_scope(const string identifier, scope_stack_t* scope) {
  for (scope_stack_t::const_iterator i = scope->begin(); i != scope->end(); ++i) {
    if ((*i)->count(identifier)) {
      return true;
    }
  }
  return false;
}
