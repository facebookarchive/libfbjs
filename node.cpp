#include "node.hpp"
using namespace fbjs;

//
// Utility function for indenting a string
string indent(const std::string str, const int len) {
  if (!len) {
    return str;
  }
  string ret;
  ret.reserve(str.size() + len * 2);
  for (int i = 0; i < len; ++i) {
    ret += "  ";
  }
  ret += str;
  return ret;
}

//
// Node: All other node inherit from this. There should only be one instance
//       a Node -- the root node.
Node::Node() {}

Node::~Node() {

  // Delete all children of this node recursively
  for (node_list_t::iterator node = this->_childNodes.begin(); node != this->_childNodes.end(); ++node) {
    delete *node;
  }
}

Node* Node::clone(Node* node) {
  if (node == NULL) {
    node = new Node();
  }
  for (node_list_t::const_iterator i = this->childNodes().begin(); i != this->childNodes().end(); ++i) {
    node->appendChild((*i)->clone());
  }
  return node;
}

Node* Node::appendChild(Node* node) {
  this->_childNodes.push_back(node);
  return this;
}

Node* Node::removeChild(node_list_t::iterator node_pos) {
  Node* node = (*node_pos);
  this->_childNodes.erase(node_pos);
  return node;
}

Node* Node::replaceChild(Node* node, node_list_t::iterator node_pos) {
  this->insertBefore(node, node_pos);
  return this->removeChild(node_pos);
}

Node* Node::insertBefore(Node* node, node_list_t::iterator node_pos) {
  this->_childNodes.insert(node_pos, node);
  return node;
}

node_list_t& Node::childNodes() {
  return this->_childNodes;
}


bool Node::empty() const {
  return this->_childNodes.empty();
}

rope_t Node::render(node_render_opts_t* opts) const {
  return this->_childNodes.front()->render(opts);
}

rope_t Node::renderImplodeChildren(node_render_opts_t* opts, const char* glue) const {
  rope_t ret;
  node_list_t::const_iterator i = this->_childNodes.begin();
  while (i != this->_childNodes.end()) {
    if (*i != NULL) {
      ret += (*i)->render(opts);
    }
    i++;
    if (i != this->_childNodes.end()) {
      ret += glue;
    }
  }
  return ret;
}

Node* Node::identifier() {
  return NULL;
}

//
// NodeBlock: a block of statements
NodeBlock::NodeBlock() : braces(false) {}

rope_t NodeBlock::render(node_render_opts_t* opts) const {
  rope_t ret;
  ret += "{";
  ret += this->_childNodes.front()->render(opts);
  ret += "}";
/*
  if (opts->pretty) {
//    opts->indentation++;
    ret += "{\n";
    ret += this->_childNodes.front()->render(opts);
    ret += "}\n";
//    opts->indentation--;
  } else {
    bool single = this->_childNodes.front()->_childNodes.size() <= 1;
    if (!single) {
      ret += "{";
    }
    ret += this->_childNodes.front()->render(opts);
    if (!single) {
      ret += "}";
    }
  }*/
  return ret;
}

NodeBlock* NodeBlock::requireBraces() {
  this->braces = true;
  return this;
}

//
// NodeStatementList: a list of statements
// note -- the difference between this and a block is that an empty statement_list will collapse into ""
// whereas a block will collapse into ";" or "{}" depending on require_braces
rope_t NodeStatementList::render(node_render_opts_t* opts) const {
  rope_t ret = this->renderImplodeChildren(opts, ";") + ";";
  return ret;
}

//
// NodeNumericLiteral: it's a number. like 5. or 3.
NodeNumericLiteral::NodeNumericLiteral(double value) : value(value) {}

Node* NodeNumericLiteral::clone(Node* node) {
  return new NodeNumericLiteral(this->value);
}

rope_t NodeNumericLiteral::render(node_render_opts_t* opts) const {

  // Try to print out a concise number
  rope_t ret;
  char buf[64];
  int trunc = -1, point = -1;
  sprintf(buf, "%.19f", this->value);
  for (int i = 0; buf[i] != 0; i++) {
    if (point == -1) {
      if (buf[i] == '.') {
        trunc = point = i;
      }
    } else if (point + i > 18) {
      break;
    } else if (buf[i] != '0') {
      trunc = i + 1;
    }
  }
  if (trunc != -1) {
    buf[trunc] = 0;
  }
  ret += buf;
  return ret;
}

//
// NodeStringLiteral: "Hello."
NodeStringLiteral::NodeStringLiteral(string value, bool quoted) : value(value), quoted(quoted) {}

Node* NodeStringLiteral::clone(Node* node) {
  return new NodeStringLiteral(this->value, this->quoted);
}

rope_t NodeStringLiteral::render(node_render_opts_t* opts) const {
  rope_t ret;
  if (!this->quoted) {
    ret += "\"";
  }
  ret += this->value.c_str();
  if (!this->quoted) {
    ret += "\"";
  }
  return ret;
}

//
// NodeBooleanLiteral: true or false
NodeBooleanLiteral::NodeBooleanLiteral(bool value) : value(value) {}

rope_t NodeBooleanLiteral::render(node_render_opts_t* opts) const {
  rope_t ret;
  ret += this->value ? "true" : "false";
  return ret;
}

Node* NodeBooleanLiteral::clone(Node* node) {
  return new NodeBooleanLiteral(this->value);
}

//
// NodeNullLiteral: null
rope_t NodeNullLiteral::render(node_render_opts_t* opts) const {
  rope_t ret;
  ret += "null";
  return ret;
}

//
// NodeThis: this
rope_t NodeThis::render(node_render_opts_t* opts) const {
  rope_t ret;
  ret += "this";
  return ret;
}

//
// NodeEmptyExpression
rope_t NodeEmptyExpression::render(node_render_opts_t* opts) const {
  rope_t ret;
  return ret;
}

//
// NodeOperator: expression <op> expression
NodeOperator::NodeOperator(node_operator_t op) : op(op) {}

Node* NodeOperator::clone(Node* node) {
  node = new NodeOperator(this->op);
  return Node::clone(node);
}

rope_t NodeOperator::render(node_render_opts_t* opts) const {
  rope_t ret;
  ret += this->_childNodes.front()->render(opts);
  switch (this->op) {
    case COMMA:
      ret += ",";
      break;

    case RSHIFT3:
      ret += ">>>";
      break;

    case RSHIFT:
      ret += ">>";
      break;

    case LSHIFT:
      ret += "<<";
      break;

    case OR:
      ret += "||";
      break;

    case AND:
      ret += "&&";
      break;

    case BIT_XOR:
      ret += "^";
      break;

    case BIT_AND:
      ret += "&";
      break;

    case BIT_OR:
      ret += "|";
      break;

    case EQUAL:
      ret += "==";
      break;

    case NOT_EQUAL:
      ret += "!=";
      break;

    case STRICT_EQUAL:
      ret += "===";
      break;

    case STRICT_NOT_EQUAL:
      ret += "!==";
      break;

    case LESS_THAN_EQUAL:
      ret += "<=";
      break;

    case GREATER_THAN_EQUAL:
      ret += ">=";
      break;

    case LESS_THAN:
      ret += "<";
      break;

    case GREATER_THAN:
      ret += ">";
      break;

    case PLUS:
      ret += "+";
      break;

    case MINUS:
      ret += "-";
      break;

    case DIV:
      ret += "/";
      break;

    case MULT:
      ret += "*";
      break;

    case MOD:
      ret += "%";
      break;

    case IN:
      ret += " in ";
      break;

    case INSTANCEOF:
      ret += " instanceof ";
      break;
  }
  ret += this->_childNodes.back()->render(opts);
  return ret;
}

//
// NodeConditionalExpression: true ? yes() : no()
rope_t NodeConditionalExpression::render(node_render_opts_t* opts) const {
  rope_t ret;
  node_list_t::const_iterator node = this->_childNodes.begin();
  ret += (*node)->render(opts);
  ret += "?";
  ret += (*++node)->render(opts);
  ret += ":";
  ret += (*++node)->render(opts);
  return ret;
}

//
// NodeParenthetical: an expression in ()'s. this is actually implicit in the AST, but we also make it an explicit
// node. Otherwise, the renderer would have to be aware of operator precedence which would be cumbersome.
rope_t NodeParenthetical::render(node_render_opts_t* opts) const {
  rope_t ret;
  ret += "(";
  ret += this->_childNodes.front()->render(opts);
  ret += ")";
  return ret;
}

Node* NodeParenthetical::identifier() {
  return this->_childNodes.front()->identifier();
}

//
// NodeAssignment: identifier = expression
NodeAssignment::NodeAssignment(node_assignment_t op) : op(op) {}

Node* NodeAssignment::clone(Node* node) {
  node = new NodeAssignment(this->op);
  return Node::clone(node);
}

rope_t NodeAssignment::render(node_render_opts_t* opts) const {
  rope_t ret;
  ret += this->_childNodes.front()->identifier()->render(opts);
  switch (this->op) {
    case ASSIGN:
      ret += "=";
      break;

    case MULT_ASSIGN:
      ret += "*=";
      break;

    case DIV_ASSIGN:
      ret += "/=";
      break;

    case MOD_ASSIGN:
      ret += "%=";
      break;

    case PLUS_ASSIGN:
      ret += "+=";
      break;

    case MINUS_ASSIGN:
      ret += "-=";
      break;

    case LSHIFT_ASSIGN:
      ret += "<<=";
      break;

    case RSHIFT_ASSIGN:
      ret += ">>=";
      break;

    case RSHIFT3_ASSIGN:
      ret += ">>>=";
      break;

    case BIT_AND_ASSIGN:
      ret += "&=";
      break;

    case BIT_XOR_ASSIGN:
      ret += "^=";
      break;

    case BIT_OR_ASSIGN:
      ret += "|=";
      break;
  }
  ret += this->_childNodes.back()->render(opts);
  return ret;
}

//
// NodeUnary
NodeUnary::NodeUnary(node_unary_t op) : op(op) {}

Node* NodeUnary::clone(Node* node) {
  node = new NodeUnary(this->op);
  return Node::clone(node);
}

rope_t NodeUnary::render(node_render_opts_t* opts) const {
  rope_t ret;
  bool need_space = false;
  switch(this->op) {
    case DELETE:
      ret += "delete";
      need_space = true;
      break;
    case VOID:
      ret += "void";
      need_space = true;
      break;
    case TYPEOF:
      ret += "typeof";
      need_space = true;
      break;
    case INCR_UNARY:
      ret += "++";
      break;
    case DECR_UNARY:
      ret += "--";
      break;
    case PLUS_UNARY:
      ret += "+";
      break;
    case MINUS_UNARY:
      ret += "-";
      break;
    case BIT_NOT_UNARY:
      ret += "~";
      break;
    case NOT_UNARY:
      ret += "!";
      break;
  }
  if (need_space && dynamic_cast<NodeParenthetical*>(this->_childNodes.front()) == NULL) {
    ret += " ";
  }
  ret += this->_childNodes.front()->render(opts);
  return ret;
}

const node_assignment_t NodeAssignment::operatorType() {
  return this->op;
}

//
// NodePostfix
NodePostfix::NodePostfix(node_postfix_t op) : op(op) {}

Node* NodePostfix::clone(Node* node) {
  node = new NodePostfix(this->op);
  return Node::clone(node);
}

rope_t NodePostfix::render(node_render_opts_t* opts) const {
  rope_t ret;
  ret += this->_childNodes.front()->render(opts);
  switch (this->op) {
    case INCR_POSTFIX:
      ret += "++";
      break;
    case DECR_POSTFIX:
      ret += "--";
      break;
  }
  return ret;
}

//
// NodeIdentifier
NodeIdentifier::NodeIdentifier(string name) : _name(name) {}

Node* NodeIdentifier::clone(Node* node) {
  node = new NodeIdentifier(this->_name);
  return Node::clone(node);
}

rope_t NodeIdentifier::render(node_render_opts_t* opts) const {
  rope_t ret;
  ret += this->_name.c_str();
  return ret;
}

string NodeIdentifier::name() const {
  return this->_name;
}

Node* NodeIdentifier::identifier() {
  return this;
}

//
// NodeArgList: list of expressions for a function call or definition
rope_t NodeArgList::render(node_render_opts_t* opts) const {
  rope_t ret;
  ret += "(";
  ret += this->renderImplodeChildren(opts, ",");
  ret += ")";
  return ret;
}

//
// NodeFunction: a function definition
rope_t NodeFunction::render(node_render_opts_t* opts) const {
  rope_t ret;
  node_list_t::const_iterator node = this->_childNodes.begin();

  ret += "function";
  if (*node != NULL) {
    ret += " ";
    ret += (*node)->render(opts);
  }

  ret += (*++node)->render(opts);
  ret += "{";
  ret += (*++node)->render(opts);
  ret += "}";
  return ret;
}

//
// NodeFunctionCall: foo(1). note: this does not cover new foo(1);
rope_t NodeFunctionCall::render(node_render_opts_t* opts) const {
  rope_t ret;
  ret += this->_childNodes.front()->render(opts);
  ret += this->_childNodes.back()->render(opts);
  return ret;
}

//
// NodeFunctionConstructor: new foo(1)
rope_t NodeFunctionConstructor::render(node_render_opts_t* opts) const {
  rope_t ret;
  ret += "new ";
  ret += this->_childNodes.front()->render(opts);
  ret += this->_childNodes.back()->render(opts);
  return ret;
}

//
// NodeIf: if (true) { honk(dazzle); };
rope_t NodeIf::render(node_render_opts_t* opts) const {
  rope_t ret;
  node_list_t::const_iterator node = this->_childNodes.begin();
  ret += "if(";
  ret += (*node)->render(opts);
  ret += ")";
  ret += (*++node)->render(opts);
  node++;
  if (*node != NULL) {
    ret += "else ";
    ret += (*node)->render(opts);
  }
  return ret;
}

//
// NodeTry
rope_t NodeTry::render(node_render_opts_t* opts) const {
  rope_t ret;
  node_list_t::const_iterator node = this->_childNodes.begin();
  ret += "try";
  ret += dynamic_cast<NodeBlock*>(*node)->requireBraces()->render(opts);
  if (*++node != NULL) {
    ret += "catch(";
    ret += (*node)->render(opts);
    ret += ")";
    ret += dynamic_cast<NodeBlock*>(*++node)->requireBraces()->render(opts);
  } else {
    node++;
  }
  if (*++node != NULL) {
    ret += "finally";
    ret += dynamic_cast<NodeBlock*>(*node)->requireBraces()->render(opts);
  }
  return ret;
}

//
// NodeStatementWithExpression: generalized node for return, throw, continue, and break. makes rendering easier and
// the rewriter doesn't really need anything from the nodes
NodeStatementWithExpression::NodeStatementWithExpression(node_statement_with_expression_t statement) : statement(statement) {}

Node* NodeStatementWithExpression::clone(Node* node) {
  node = new NodeStatementWithExpression(this->statement);
  return Node::clone(node);
}

rope_t NodeStatementWithExpression::render(node_render_opts_t* opts) const {
  rope_t ret;
  switch (this->statement) {
    case THROW:
      ret += "throw";
      break;

    case RETURN:
      ret += "return";
      break;

    case CONTINUE:
      ret += "continue";
      break;

    case BREAK:
      ret += "break";
      break;
  }
  if (this->_childNodes.back() != NULL) {
    ret += " ";
    ret += this->_childNodes.front()->render(opts);
  }
  ret += ";";
  return ret;
}

//
// NodeLabel
rope_t NodeLabel::render(node_render_opts_t* opts) const {
  rope_t ret;
  ret += this->_childNodes.front()->render(opts);
  ret += ":";
  ret += this->_childNodes.back()->render(opts);
  return ret;
}

//
// NodeSwitch
rope_t NodeSwitch::render(node_render_opts_t* opts) const {
  rope_t ret;
  ret += "switch(";
  ret += this->_childNodes.front()->render(opts);
  ret += ")";
  ret += this->_childNodes.back()->render(opts);
  return ret;
}

//
// NodeDefaultClause: default: foo();
rope_t NodeDefaultClause::render(node_render_opts_t* opts) const {
  rope_t ret;
  ret += "default:";
  if (this->_childNodes.back() != NULL) {
    ret += this->_childNodes.back()->render(opts);
  }
  return ret;
}

//
// NodeCaseClause: case: bar();
rope_t NodeCaseClause::render(node_render_opts_t* opts) const {
  rope_t ret;
  ret += "case ";
  ret += this->_childNodes.front()->render(opts);
  ret += ":";
  if (this->_childNodes.back() != NULL) {
    ret += this->_childNodes.back()->render(opts);
  }
  return ret;
}

//
// NodeCaseClauseList: list of NodeCaseClause and NodeDefaultClause
rope_t NodeCaseClauseList::render(node_render_opts_t* opts) const {
  rope_t ret;
  ret += "{";
  ret += this->renderImplodeChildren(opts, "");
  ret += "}";
  return ret;
}

//
// NodeVarDeclaration: a list of identifiers with optional assignments
rope_t NodeVarDeclaration::render(node_render_opts_t* opts) const {
  rope_t ret;
  ret += "var ";
  ret += this->renderImplodeChildren(opts, ",");
  return ret;
}

//
// NodeObjectLiteral
rope_t NodeObjectLiteral::render(node_render_opts_t* opts) const {
  rope_t ret;
  ret += "{";
  ret += this->renderImplodeChildren(opts, ",");
  ret += "}";
  return ret;
}

//
// NodeObjectLiteralProperty
rope_t NodeObjectLiteralProperty::render(node_render_opts_t* opts) const {
  rope_t ret;
  ret += this->_childNodes.front()->render(opts);
  ret += ":";
  ret += this->_childNodes.back()->render(opts);
  return ret;
}

//
// NodeArrayLiteral
rope_t NodeArrayLiteral::render(node_render_opts_t* opts) const {
  rope_t ret;
  ret += "[";
  ret += this->renderImplodeChildren(opts, ",");
  ret += "]";
  return ret;
}

//
// NodeStaticMemberExpression: object access via foo.bar
NodeStaticMemberExpression::NodeStaticMemberExpression() : isAssignment(false) {}
rope_t NodeStaticMemberExpression::render(node_render_opts_t* opts) const {
  rope_t ret;
  ret += this->_childNodes.front()->render(opts);
  ret += ".";
  ret += this->_childNodes.back()->render(opts);
  return ret;
}

Node* NodeStaticMemberExpression::identifier() {
  this->isAssignment = true;
  return this;
}

//
// NodeDynamicMemberExpression: object access via foo['bar']
NodeDynamicMemberExpression::NodeDynamicMemberExpression() : isAssignment(false) {}
rope_t NodeDynamicMemberExpression::render(node_render_opts_t* opts) const {
  rope_t ret;
  ret += this->_childNodes.front()->render(opts);
  ret += "[";
  ret += this->_childNodes.back()->render(opts);
  ret += "]";
  return ret;
}

Node* NodeDynamicMemberExpression::identifier() {
  this->isAssignment = true;
  return this;
}

//
// NodeForLoop: only for(;;); loops, not for in
rope_t NodeForLoop::render(node_render_opts_t* opts) const {
  rope_t ret;
  node_list_t::const_iterator node = this->_childNodes.begin();
  ret += "for(";
  ret += (*node)->render(opts);
  ret += ";";
  ret += (*++node)->render(opts);
  ret += ";";
  ret += (*++node)->render(opts);
  ret += ")";
  ret += (*++node)->render(opts);
  return ret;
}

//
// NodeForIn
rope_t NodeForIn::render(node_render_opts_t* opts) const {
  rope_t ret;
  node_list_t::const_iterator node = this->_childNodes.begin();
  ret += "for(";
  ret += (*node)->render(opts);
  ret += " in ";
  ret += (*++node)->render(opts);
  ret += ")";
  ret += (*++node)->render(opts);
  return ret;
}

//
// NodeWhile
rope_t NodeWhile::render(node_render_opts_t* opts) const {
  rope_t ret;
  ret += "while(";
  ret += this->_childNodes.front()->render(opts);
  ret += ")";
  ret += this->_childNodes.back()->render(opts);
  return ret;
}

//
// NodeDoWhile
rope_t NodeDoWhile::render(node_render_opts_t* opts) const {
  rope_t ret;
  ret += "do ";
  ret += this->_childNodes.front()->render(opts);
  ret += "while(";
  ret += this->_childNodes.back()->render(opts);
  ret += ");";
  return ret;
}
