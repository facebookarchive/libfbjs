#include "node.hpp"
using namespace std;
using namespace fbjs;

//
// Node: All other nodes inherit from this. There should only be one instance
//       of a Node -- the root node.
Node::Node(const unsigned int lineno /* = 0 */) : _lineno(lineno) {}

Node::~Node() {

  // Delete all children of this node recursively
  for (node_list_t::iterator node = this->_childNodes.begin(); node != this->_childNodes.end(); ++node) {
    delete *node;
  }
}

Node* Node::clone(Node* node) const {
  if (node == NULL) {
    node = new Node();
  }
  for (node_list_t::const_iterator i = const_cast<Node*>(this)->childNodes().begin(); i != const_cast<Node*>(this)->childNodes().end(); ++i) {
    node->appendChild((*i) == NULL ? NULL : (*i)->clone());
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

rope_t Node::render(node_render_enum opts /* = RENDER_NONE */) const {
  return this->render((int)opts);
}

rope_t Node::render(int opts) const {
  render_guts_t guts;
  guts.pretty = opts & RENDER_PRETTY;
  guts.sanelineno = opts & RENDER_MAINTAIN_LINENO;
  guts.lineno = 1;
  return this->render(&guts, 0);
}

rope_t Node::render(render_guts_t* guts, int indentation) const {
  return this->_childNodes.front()->render(guts, indentation);
}

rope_t Node::renderBlock(bool must, render_guts_t* guts, int indentation) const {
  if (!must && !guts->pretty) {
    rope_t ret;
    if (guts->sanelineno) {
      this->renderLinenoCatchup(guts, ret);
    }
    ret += this->renderStatement(guts, indentation);
    return ret;
  } else {
    rope_t ret(guts->pretty ? " {" : "{");
    ret += this->renderIndentedStatement(guts, indentation + 1);
    if (guts->pretty || guts->sanelineno) {
      bool newline;
      if (guts->sanelineno) {
        newline = this->renderLinenoCatchup(guts, ret);
      } else {
        ret += "\n";
        newline = true;
      }
      if (guts->pretty && newline) {
        for (int i = 0; i < indentation; ++i) {
          ret += "  ";
        }
      }
    }
    ret += "}";
    return ret;
  }
}

rope_t Node::renderIndentedStatement(render_guts_t* guts, int indentation) const {
  if (guts->pretty || guts->sanelineno) {
    rope_t ret;
    bool newline;
    if (guts->sanelineno) {
      newline = this->renderLinenoCatchup(guts, ret);
    } else {
      ret += "\n";
      newline = true;
    }
    if (guts->pretty && newline) {
      for (int i = 0; i < indentation; ++i) {
        ret += "  ";
      }
    }
    return ret + this->renderStatement(guts, indentation);
  } else {
    return this->renderStatement(guts, indentation);
  }
}

rope_t Node::renderStatement(render_guts_t* guts, int indentation) const {
  return this->render(guts, indentation);
}

rope_t Node::renderImplodeChildren(render_guts_t* guts, int indentation, const char* glue) const {
  rope_t ret;
  node_list_t::const_iterator i = this->_childNodes.begin();
  while (i != this->_childNodes.end()) {
    if (*i != NULL) {
      ret += (*i)->render(guts, indentation);
    }
    i++;
    if (i != this->_childNodes.end()) {
      ret += glue;
    }
  }
  return ret;
}

bool Node::renderLinenoCatchup(render_guts_t* guts, rope_t &rope) const {
  if (!this->lineno() || guts->lineno >= this->lineno()) {
    return false;
  }
  rope += rope_t(this->lineno() - guts->lineno, '\n');
  guts->lineno = this->lineno();
  return true;
}

Node* Node::identifier() {
  return NULL;
}

unsigned int  Node::lineno() const {
  return this->_lineno;
}

//
// NodeStatementList: a list of statements
NodeStatementList::NodeStatementList(const unsigned int lineno /* = 0 */) : Node(lineno) {}
Node* NodeStatementList::clone(Node* node) const {
  return Node::clone(new NodeStatementList());
}

rope_t NodeStatementList::render(render_guts_t* guts, int indentation) const {
  rope_t ret;
  for (node_list_t::const_iterator i = this->_childNodes.begin(); i != this->_childNodes.end(); ++i) {
    if (*i != NULL) {
      ret += (*i)->renderIndentedStatement(guts, indentation);
    }
  }
  return ret;
}

rope_t NodeStatementList::renderBlock(bool must, render_guts_t* guts, int indentation) const {
  if (!must && this->empty()) {
    return rope_t(";");
  } else if (!must && !guts->pretty && this->_childNodes.front() == this->_childNodes.back()) {
    rope_t ret;
    if (guts->sanelineno) {
      this->renderLinenoCatchup(guts, ret);
    }
    ret += this->_childNodes.front()->renderStatement(guts, indentation);
    return ret;
  } else {
    rope_t ret(guts->pretty ? " {" : "{");
    ret += this->renderIndentedStatement(guts, indentation + 1);
    if (guts->pretty || guts->sanelineno) {
      bool newline;
      if (guts->sanelineno) {
        newline = this->renderLinenoCatchup(guts, ret);
      } else {
        ret += "\n";
        newline = true;
      }
      if (guts->pretty && newline) {
        for (int i = 0; i < indentation; ++i) {
          ret += "  ";
        }
      }
    }
    ret += "}";
    return ret;
  }
}

rope_t NodeStatementList::renderIndentedStatement(render_guts_t* guts, int indentation) const {
  return this->render(guts, indentation);
}

rope_t NodeStatementList::renderStatement(render_guts_t* guts, int indentation) const {
  return this->render(guts, indentation);
}

//
// NodeExpression
NodeExpression::NodeExpression(const unsigned int lineno /* = 0 */) : Node(lineno) {}
rope_t NodeExpression::renderStatement(render_guts_t* guts, int indentation) const {
  return this->render(guts, indentation) + ";";
}

//
// NodeNumericLiteral: it's a number. like 5. or 3.
NodeNumericLiteral::NodeNumericLiteral(double value, const unsigned int lineno /* = 0 */) : NodeExpression(lineno), value(value) {}

Node* NodeNumericLiteral::clone(Node* node) const {
  return new NodeNumericLiteral(this->value);
}

rope_t NodeNumericLiteral::render(render_guts_t* guts, int indentation) const {

  // Try to print out a concise number
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
  return rope_t(buf);
}

//
// NodeStringLiteral: "Hello."
NodeStringLiteral::NodeStringLiteral(string value, bool quoted, const unsigned int lineno /* = 0 */) : NodeExpression(lineno), value(value), quoted(quoted) {}

Node* NodeStringLiteral::clone(Node* node) const {
  return new NodeStringLiteral(this->value, this->quoted);
}

rope_t NodeStringLiteral::render(render_guts_t* guts, int indentation) const {
  if (this->quoted) {
    return rope_t(this->value.c_str());
  } else {
    return rope_t("\"") + this->value.c_str() + "\"";
  }
}

//
// NodeRegexLiteral: /foo|bar/
NodeRegexLiteral::NodeRegexLiteral(string value, const unsigned int lineno /* = 0 */) : NodeExpression(lineno), value(value) {}

Node* NodeRegexLiteral::clone(Node* node) const {
  return new NodeRegexLiteral(this->value);
}

rope_t NodeRegexLiteral::render(render_guts_t* guts, int indentation) const {
  return rope_t("/") + this->value.c_str() + "/";
}

//
// NodeBooleanLiteral: true or false
NodeBooleanLiteral::NodeBooleanLiteral(bool value, const unsigned int lineno /* = 0 */) : NodeExpression(lineno), value(value) {}

rope_t NodeBooleanLiteral::render(render_guts_t* guts, int indentation) const {
  return rope_t(this->value ? "true" : "false");
}

Node* NodeBooleanLiteral::clone(Node* node) const {
  return new NodeBooleanLiteral(this->value);
}

//
// NodeNullLiteral: null
NodeNullLiteral::NodeNullLiteral(const unsigned int lineno /* = 0 */) : NodeExpression(lineno) {}
Node* NodeNullLiteral::clone(Node* node) const {
  return Node::clone(new NodeNullLiteral());
}

rope_t NodeNullLiteral::render(render_guts_t* guts, int indentation) const {
  return rope_t("null");
}

//
// NodeThis: this
NodeThis::NodeThis(const unsigned int lineno /* = 0 */) : NodeExpression(lineno) {}
Node* NodeThis::clone(Node* node) const {
  return Node::clone(new NodeThis());
}

rope_t NodeThis::render(render_guts_t* guts, int indentation) const {
  return rope_t("this");
}

//
// NodeEmptyExpression
NodeEmptyExpression::NodeEmptyExpression(const unsigned int lineno /* = 0 */) : NodeExpression(lineno) {}
Node* NodeEmptyExpression::clone(Node* node) const {
  return Node::clone(new NodeEmptyExpression());
}

rope_t NodeEmptyExpression::render(render_guts_t* guts, int indentation) const {
  return rope_t();
}

rope_t NodeEmptyExpression::renderBlock(bool must, render_guts_t* guts, int indentation) const {
  return rope_t(";");
}

//
// NodeOperator: expression <op> expression
NodeOperator::NodeOperator(node_operator_t op, const unsigned int lineno /* = 0 */) : NodeExpression(lineno), op(op) {}

Node* NodeOperator::clone(Node* node) const {
  return Node::clone(new NodeOperator(this->op));
}

rope_t NodeOperator::render(render_guts_t* guts, int indentation) const {
  rope_t ret;
  bool padding = true;
  ret += this->_childNodes.front()->render(guts, indentation);
  if (guts->pretty) {
    padding = false;
    ret += " ";
  }
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
      ret += padding ? " in " : "in";
      break;

    case INSTANCEOF:
      ret += padding ? " instanceof " : "instanceof";
      break;
  }
  if (!padding) {
    ret += " ";
  }
  ret += this->_childNodes.back()->render(guts, indentation);
  return ret;
}

//
// NodeConditionalExpression: true ? yes() : no()
NodeConditionalExpression::NodeConditionalExpression(const unsigned int lineno /* = 0 */) : NodeExpression(lineno) {}
Node* NodeConditionalExpression::clone(Node* node) const {
  return Node::clone(new NodeConditionalExpression());
}

rope_t NodeConditionalExpression::render(render_guts_t* guts, int indentation) const {
  node_list_t::const_iterator node = this->_childNodes.begin();
  rope_t ret((*node)->render(guts, indentation));
  ret += rope_t(guts->pretty ? " ? " : "?") +
    (*++node)->render(guts, indentation) +
    (guts->pretty ? " : " : ":");
  ret += (*++node)->render(guts, indentation);
  return ret;
}

//
// NodeParenthetical: an expression in ()'s. this is actually implicit in the AST, but we also make it an explicit
// node. Otherwise, the renderer would have to be aware of operator precedence which would be cumbersome.
NodeParenthetical::NodeParenthetical(const unsigned int lineno /* = 0 */) : NodeExpression(lineno) {}
Node* NodeParenthetical::clone(Node* node) const {
  return Node::clone(new NodeParenthetical());
}

rope_t NodeParenthetical::render(render_guts_t* guts, int indentation) const {
  return rope_t("(") + this->_childNodes.front()->render(guts, indentation) + ")";
}

Node* NodeParenthetical::identifier() {
  return this->_childNodes.front()->identifier();
}

//
// NodeAssignment: identifier = expression
NodeAssignment::NodeAssignment(node_assignment_t op, const unsigned int lineno /* = 0 */) : NodeExpression(lineno), op(op) {}

Node* NodeAssignment::clone(Node* node) const {
  return Node::clone(new NodeAssignment(this->op));
}

rope_t NodeAssignment::render(render_guts_t* guts, int indentation) const {
  rope_t ret;
  ret += this->_childNodes.front()->identifier()->render(guts, indentation);
  if (guts->pretty) {
    ret += " ";
  }
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
  if (guts->pretty) {
    ret += " ";
  }
  ret += this->_childNodes.back()->render(guts, indentation);
  return ret;
}

const node_assignment_t NodeAssignment::operatorType() const {
  return this->op;
}

//
// NodeUnary
NodeUnary::NodeUnary(node_unary_t op, const unsigned int lineno /* = 0 */) : NodeExpression(lineno), op(op) {}

Node* NodeUnary::clone(Node* node) const {
  return Node::clone(new NodeUnary(this->op));
}

rope_t NodeUnary::render(render_guts_t* guts, int indentation) const {
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
  ret += this->_childNodes.front()->render(guts, indentation);
  return ret;
}

const node_unary_t NodeUnary::operatorType() const {
  return this->op;
}

//
// NodePostfix
NodePostfix::NodePostfix(node_postfix_t op, const unsigned int lineno /* = 0 */) : NodeExpression(lineno), op(op) {}

Node* NodePostfix::clone(Node* node) const {
  return Node::clone(new NodePostfix(this->op));
}

rope_t NodePostfix::render(render_guts_t* guts, int indentation) const {
  rope_t ret(this->_childNodes.front()->render(guts, indentation));
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
NodeIdentifier::NodeIdentifier(string name, const unsigned int lineno /* = 0 */) : NodeExpression(lineno), _name(name) {}

Node* NodeIdentifier::clone(Node* node) const {
  return Node::clone(new NodeIdentifier(this->_name));
}

rope_t NodeIdentifier::render(render_guts_t* guts, int indentation) const {
  return rope_t(this->_name.c_str());
}

string NodeIdentifier::name() const {
  return this->_name;
}

Node* NodeIdentifier::identifier() {
  return this;
}

//
// NodeArgList: list of expressions for a function call or definition
NodeArgList::NodeArgList(const unsigned int lineno /* = 0 */) : Node(lineno) {}
Node* NodeArgList::clone(Node* node) const {
  return Node::clone(new NodeArgList());
}

rope_t NodeArgList::render(render_guts_t* guts, int indentation) const {
  return rope_t("(") + this->renderImplodeChildren(guts, indentation, guts->pretty ? ", " : ",") + ")";
}

//
// NodeFunction: a function definition
NodeFunction::NodeFunction(bool declaration /* = false */, const unsigned int lineno /* = 0 */) : Node(lineno), _declaration(declaration) {}

Node* NodeFunction::clone(Node* node) const {
  return Node::clone(new NodeFunction());
}

bool NodeFunction::declaration() const {
  return this->_declaration;
}

rope_t NodeFunction::render(render_guts_t* guts, int indentation) const {
  rope_t ret;
  node_list_t::const_iterator node = this->_childNodes.begin();

  ret += "function";
  if (*node != NULL) {
    ret += " ";
    ret += (*node)->render(guts, indentation);
  }

  ret += (*++node)->render(guts, indentation);
  ret += (*++node)->renderBlock(true, guts, indentation);
  return ret;
}

//
// NodeFunctionCall: foo(1). note: this does not cover new foo(1);
NodeFunctionCall::NodeFunctionCall(const unsigned int lineno /* = 0 */) : NodeExpression(lineno) {}
Node* NodeFunctionCall::clone(Node* node) const {
  return Node::clone(new NodeFunctionCall());
}

rope_t NodeFunctionCall::render(render_guts_t* guts, int indentation) const {
  return rope_t(this->_childNodes.front()->render(guts, indentation)) + this->_childNodes.back()->render(guts, indentation);
}

//
// NodeFunctionConstructor: new foo(1)
NodeFunctionConstructor::NodeFunctionConstructor(const unsigned int lineno /* = 0 */) : NodeExpression(lineno) {}
Node* NodeFunctionConstructor::clone(Node* node) const {
  return Node::clone(new NodeFunctionConstructor());
}

rope_t NodeFunctionConstructor::render(render_guts_t* guts, int indentation) const {
  return rope_t("new ") + this->_childNodes.front()->render(guts, indentation) + this->_childNodes.back()->render(guts, indentation);
}

//
// NodeIf: if (true) { honk(dazzle); };
NodeIf::NodeIf(const unsigned int lineno /* = 0 */) : Node(lineno) {}
Node* NodeIf::clone(Node* node) const {
  return Node::clone(new NodeIf());
}

rope_t NodeIf::render(render_guts_t* guts, int indentation) const {
  rope_t ret;
  node_list_t::const_iterator node = this->_childNodes.begin();
  ret += guts->pretty ? "if (" : "if(";
  ret += (*node)->render(guts, indentation);
  ret += ")";
  ret += (*++node)->renderBlock(false, guts, indentation);
  if (*++node != NULL) {
    ret += guts->pretty ? " else " : "else";

    // Special-case for rendering else if's
    if (typeid(**node) == typeid(NodeIf)) {
      if (guts->sanelineno) {
        (*node)->renderLinenoCatchup(guts, ret);
      }
      ret += (*node)->render(guts, indentation);
    } else {
      ret += (*node)->renderBlock(false, guts, indentation);
    }
  }
  return ret;
}

//
// NodeWith: with (foo) { bar(); };
NodeWith::NodeWith(const unsigned int lineno /* = 0 */) : Node(lineno) {}
Node* NodeWith::clone(Node* node) const {
  return Node::clone(new NodeWith());
}

rope_t NodeWith::render(render_guts_t* guts, int indentation) const {
  rope_t ret;
  node_list_t::const_iterator node = this->_childNodes.begin();
  ret += guts->pretty ? "with (" : "with(";
  ret += (*node)->render(guts, indentation);
  ret += ")";
  ret += (*++node)->renderBlock(false, guts, indentation);
  return ret;
}

//
// NodeTry
NodeTry::NodeTry(const unsigned int lineno /* = 0 */) : Node(lineno) {}
Node* NodeTry::clone(Node* node) const {
  return Node::clone(new NodeTry());
}

rope_t NodeTry::render(render_guts_t* guts, int indentation) const {
  rope_t ret;
  node_list_t::const_iterator node = this->_childNodes.begin();
  ret += "try";
  ret += (*node)->renderBlock(true, guts, indentation);
  if (*++node != NULL) {
    ret += (guts->pretty ? " catch (" : "catch(");
    ret += (*node)->render(guts, indentation) + ")";
    ret += (*++node)->renderBlock(true, guts, indentation);
  } else {
    node++;
  }
  if (*++node != NULL) {
    ret += (guts->pretty ? " finally" : "finally");
    ret += (*node)->renderBlock(true, guts, indentation);
  }
  return ret;
}

//
// NodeStatement
NodeStatement::NodeStatement(const unsigned int lineno /* = 0 */) : Node(lineno) {}
rope_t NodeStatement::renderStatement(render_guts_t* guts, int indentation) const {
  return this->render(guts, indentation) + ";";
}

//
// NodeStatementWithExpression: generalized node for return, throw, continue, and break. makes rendering easier and
// the rewriter doesn't really need anything from the nodes
NodeStatementWithExpression::NodeStatementWithExpression(node_statement_with_expression_t statement, const unsigned int lineno /* = 0 */) : NodeStatement(lineno), statement(statement) {}

Node* NodeStatementWithExpression::clone(Node* node) const {
  return Node::clone(new NodeStatementWithExpression(this->statement));
}

rope_t NodeStatementWithExpression::render(render_guts_t* guts, int indentation) const {
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
    ret += this->_childNodes.front()->render(guts, indentation);
  }
  return ret;
}

//
// NodeLabel
NodeLabel::NodeLabel(const unsigned int lineno /* = 0 */) : Node(lineno) {}
Node* NodeLabel::clone(Node* node) const {
  return Node::clone(new NodeLabel());
}

rope_t NodeLabel::render(render_guts_t* guts, int indentation) const {
  return rope_t(this->_childNodes.front()->render(guts, indentation)) + (guts->pretty ? ": " : ":") + this->_childNodes.back()->render(guts, indentation);
}

rope_t NodeLabel::renderStatement(render_guts_t* guts, int indentation) const {
  return this->render(guts, indentation) + ";";
}

//
// NodeSwitch
NodeSwitch::NodeSwitch(const unsigned int lineno /* = 0 */) : Node(lineno) {}
Node* NodeSwitch::clone(Node* node) const {
  return Node::clone(new NodeSwitch());
}

rope_t NodeSwitch::render(render_guts_t* guts, int indentation) const {
  return rope_t("switch(") + this->_childNodes.front()->render(guts, indentation) + ")" +
    // Render this with extra indentation, and then in NodeCaseClause we drop lower by 1.
    this->_childNodes.back()->renderBlock(true, guts, indentation + 1);
}

//
// NodeCaseClause: case: bar();
NodeCaseClause::NodeCaseClause(const unsigned int lineno /* = 0 */) : Node(lineno) {}
Node* NodeCaseClause::clone(Node* node) const {
  return Node::clone(new NodeCaseClause());
}

rope_t NodeCaseClause::render(render_guts_t* guts, int indentation) const {
  return rope_t("case ") + this->_childNodes.front()->render(guts, indentation) + ":";
}

rope_t NodeCaseClause::renderStatement(render_guts_t* guts, int indentation) const {
  return this->render(guts, indentation);
}

rope_t NodeCaseClause::renderIndentedStatement(render_guts_t* guts, int indentation) const {
  return Node::renderIndentedStatement(guts, indentation - 1);
}

//
// NodeDefaultClause: default: foo();
NodeDefaultClause::NodeDefaultClause(const unsigned int lineno /* = 0 */) : NodeCaseClause(lineno) {}
Node* NodeDefaultClause::clone(Node* node) const {
  return Node::clone(new NodeDefaultClause());
}

rope_t NodeDefaultClause::render(render_guts_t* guts, int indentation) const {
  return rope_t("default:");
}

//
// NodeVarDeclaration: a list of identifiers with optional assignments
NodeVarDeclaration::NodeVarDeclaration(bool iterator /* = false */, const unsigned int lineno /* = 0 */) : NodeStatement(lineno), _iterator(iterator) {}
Node* NodeVarDeclaration::clone(Node* node) const {
  return Node::clone(new NodeVarDeclaration());
}

rope_t NodeVarDeclaration::render(render_guts_t* guts, int indentation) const {
  return rope_t("var ") + this->renderImplodeChildren(guts, indentation, guts->pretty ? ", " : ",");
}

bool NodeVarDeclaration::iterator() const {
  return this->_iterator;
}

Node* NodeVarDeclaration::setIterator(bool iterator) {
  this->_iterator = iterator;
  return this;
}

//
// NodeObjectLiteral
NodeObjectLiteral::NodeObjectLiteral(const unsigned int lineno /* = 0 */) : NodeExpression(lineno) {}
Node* NodeObjectLiteral::clone(Node* node) const {
  return Node::clone(new NodeObjectLiteral());
}

rope_t NodeObjectLiteral::render(render_guts_t* guts, int indentation) const {
  return rope_t("{") + this->renderImplodeChildren(guts, indentation, guts->pretty ? ", " : ",") + "}";
}

//
// NodeObjectLiteralProperty
NodeObjectLiteralProperty::NodeObjectLiteralProperty(const unsigned int lineno /* = 0 */) : Node(lineno) {}
Node* NodeObjectLiteralProperty::clone(Node* node) const {
  return Node::clone(new NodeObjectLiteralProperty());
}

rope_t NodeObjectLiteralProperty::render(render_guts_t* guts, int indentation) const {
  return rope_t(this->_childNodes.front()->render(guts, indentation)) + (guts->pretty ? ": " : ":") +
    this->_childNodes.back()->render(guts, indentation);
}

//
// NodeArrayLiteral
NodeArrayLiteral::NodeArrayLiteral(const unsigned int lineno /* = 0 */) : NodeExpression(lineno) {}
Node* NodeArrayLiteral::clone(Node* node) const {
  return Node::clone(new NodeArrayLiteral());
}

rope_t NodeArrayLiteral::render(render_guts_t* guts, int indentation) const {
  return rope_t("[") + this->renderImplodeChildren(guts, indentation, guts->pretty ? ", " : ",") + "]";
}

//
// NodeStaticMemberExpression: object access via foo.bar
NodeStaticMemberExpression::NodeStaticMemberExpression(const unsigned int lineno /* = 0 */) : NodeExpression(lineno), isAssignment(false) {}
rope_t NodeStaticMemberExpression::render(render_guts_t* guts, int indentation) const {
  return rope_t(this->_childNodes.front()->render(guts, indentation)) + "." + this->_childNodes.back()->render(guts, indentation);
}

Node* NodeStaticMemberExpression::clone(Node* node) const {
  return Node::clone(new NodeStaticMemberExpression());
}

Node* NodeStaticMemberExpression::identifier() {
  this->isAssignment = true;
  return this;
}

//
// NodeDynamicMemberExpression: object access via foo['bar']
NodeDynamicMemberExpression::NodeDynamicMemberExpression(const unsigned int lineno /* = 0 */) : NodeExpression(lineno), isAssignment(false) {}

Node* NodeDynamicMemberExpression::clone(Node* node) const {
  return Node::clone(new NodeDynamicMemberExpression());
}

rope_t NodeDynamicMemberExpression::render(render_guts_t* guts, int indentation) const {
  return rope_t(this->_childNodes.front()->render(guts, indentation)) +
    "[" + this->_childNodes.back()->render(guts, indentation) + "]";
}

Node* NodeDynamicMemberExpression::identifier() {
  this->isAssignment = true;
  return this;
}

//
// NodeForLoop: only for(;;); loops, not for in
NodeForLoop::NodeForLoop(const unsigned int lineno /* = 0 */) : Node(lineno) {}
Node* NodeForLoop::clone(Node* node) const {
  return Node::clone(new NodeForLoop());
}

rope_t NodeForLoop::render(render_guts_t* guts, int indentation) const {
  node_list_t::const_iterator node = this->_childNodes.begin();
  rope_t ret(guts->pretty ? "for (" : "for(");
  ret += (*node)->render(guts, indentation) + (guts->pretty ? "; " : ";");
  ret += (*++node)->render(guts, indentation) + (guts->pretty ? "; " : ";");
  ret += (*++node)->render(guts, indentation) + ")";
  ret += (*++node)->renderBlock(false, guts, indentation);
  return ret;
}

//
// NodeForIn
NodeForIn::NodeForIn(const unsigned int lineno /* = 0 */) : Node(lineno) {}
Node* NodeForIn::clone(Node* node) const {
  return Node::clone(new NodeForIn());
}

rope_t NodeForIn::render(render_guts_t* guts, int indentation) const {
  node_list_t::const_iterator node = this->_childNodes.begin();
  rope_t ret(guts->pretty ? "for (" : "for(");
  ret += (*node)->render(guts, indentation) + " in ";
  ret += (*++node)->render(guts, indentation) + ")";
  ret += (*++node)->renderBlock(false, guts, indentation);
  return ret;
}

//
// NodeWhile
NodeWhile::NodeWhile(const unsigned int lineno /* = 0 */) : Node(lineno) {}
Node* NodeWhile::clone(Node* node) const {
  return Node::clone(new NodeWhile());
}

rope_t NodeWhile::render(render_guts_t* guts, int indentation) const {
  return rope_t(guts->pretty ? "while (" : "while(") +
    this->_childNodes.front()->render(guts, indentation) + ")" +
    this->_childNodes.back()->renderBlock(false, guts, indentation);
}

//
// NodeDoWhile
NodeDoWhile::NodeDoWhile(const unsigned int lineno /* = 0 */) : NodeStatement(lineno) {}
Node* NodeDoWhile::clone(Node* node) const {
  return Node::clone(new NodeDoWhile());
}

rope_t NodeDoWhile::render(render_guts_t* guts, int indentation) const {
  rope_t ret("do");
  // Technically this shouldn't be renderBlock(true, ...) but requiring braces makes it easier to render it all...
  ret += this->_childNodes.front()->renderBlock(true, guts, indentation);
  if (guts->sanelineno) {
    this->_childNodes.back()->renderLinenoCatchup(guts, ret);
  }
  ret += (guts->pretty ? " while (" : "while(") ;
  ret += this->_childNodes.back()->render(guts, indentation) + ")";
  return ret;
}
