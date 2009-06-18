#pragma once
#include <stdio.h>
#include <stdexcept>
#include <string>
#include <sstream>
#include <list>
#include <ext/rope>

typedef __gnu_cxx::rope<char> rope_t;

namespace fbjs {
  class Node;
  typedef std::list<Node*> node_list_t;
  enum node_render_enum {
    RENDER_NONE = 0,
    RENDER_PRETTY = 1,
    RENDER_MAINTAIN_LINENO = 2,
  };
  struct render_guts_t {
    unsigned int lineno;
    bool pretty;
    bool sanelineno;
  };

  //
  // Node
  class Node {
    protected:
      node_list_t _childNodes;
      rope_t renderImplodeChildren(render_guts_t* guts, int indentation, const char* glue) const;
      unsigned int _lineno;

    public:
      Node(const unsigned int lineno = 0);
      virtual ~Node();
      virtual Node* clone(Node* node = NULL) const;

      bool empty() const;
      unsigned int lineno() const;
      virtual Node* reduce();
      virtual bool operator== (const Node&) const;
      virtual bool operator!= (const Node&) const;

      node_list_t& Node::childNodes() const;
      Node* appendChild(Node* node);
      Node* prependChild(Node* node);
      Node* removeChild(node_list_t::iterator node_pos);
      Node* replaceChild(Node* node, node_list_t::iterator node_pos);
      Node* insertBefore(Node* node, node_list_t::iterator node_pos);

      rope_t render(node_render_enum opts = RENDER_NONE) const;
      rope_t render(int opts) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
      virtual rope_t renderBlock(bool must, render_guts_t* guts, int indentation) const;
      virtual rope_t renderStatement(render_guts_t* guts, int indentation) const;
      virtual rope_t renderIndentedStatement(render_guts_t* guts, int indentation) const;
      bool renderLinenoCatchup(render_guts_t* guts, rope_t &rope) const;
  };

  //
  // NodeProgram
  class NodeProgram: public Node {
    public:
      NodeProgram();
      NodeProgram(const char* code);
      NodeProgram(FILE* file);
      virtual Node* clone(Node* node = NULL) const;
  };

  //
  // NodeStatementList
  class NodeStatementList: public Node {
    public:
      NodeStatementList(const unsigned int lineno = 0);
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
      virtual rope_t renderBlock(bool must, render_guts_t* guts, int indentation) const;
      virtual rope_t renderStatement(render_guts_t* guts, int indentation) const;
      virtual rope_t renderIndentedStatement(render_guts_t* guts, int indentation) const;
      virtual Node* reduce();
  };

  //
  // NodeExpression (abstract)
  class NodeExpression: public Node {
    public:
      NodeExpression(const unsigned int lineno = 0);
      virtual bool isValidlVal() const;
      virtual rope_t render(render_guts_t* guts, int indentation) const = 0;
      virtual rope_t renderStatement(render_guts_t* guts, int indentation) const;
      virtual bool compare(bool val) const;
  };

  //
  // NodeNumericLiteral
  class NodeNumericLiteral: public NodeExpression {
    protected:
      double value;
    public:
      NodeNumericLiteral(double value, const unsigned int lineno = 0);
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
      virtual bool compare(bool val) const;
      virtual bool operator== (const Node&) const;
  };

  //
  // NodeStringLiteral
  class NodeStringLiteral: public NodeExpression {
    protected:
      std::string value;
      bool quoted;
    public:
      NodeStringLiteral(std::string value, bool quoted, const unsigned int lineno = 0);
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
      virtual bool operator== (const Node&) const;
  };

  //
  // NodeRegexLiteral
  class NodeRegexLiteral: public NodeExpression {
    protected:
      std::string value;
      std::string flags;
    public:
      NodeRegexLiteral(std::string value, std::string flags, const unsigned int lineno = 0);
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
      virtual bool operator== (const Node&) const;
  };

  //
  // NodeBooleanLiteral
  class NodeBooleanLiteral: public NodeExpression {
    protected:
      bool value;
    public:
      NodeBooleanLiteral(bool value, const unsigned int lineno = 0);
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
      virtual bool compare(bool val) const;
      virtual bool operator== (const Node&) const;
  };

  //
  // NodeNullLiteral
  class NodeNullLiteral: public NodeExpression {
    public:
      NodeNullLiteral(const unsigned int lineno = 0);
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
  };

  //
  // NodeThis
  class NodeThis: public NodeExpression {
    public:
      NodeThis(const unsigned int lineno = 0);
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
  };

  //
  // NodeEmptyExpression
  class NodeEmptyExpression: public NodeExpression {
    public:
      NodeEmptyExpression(const unsigned int lineno = 0);
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
      virtual rope_t renderBlock(bool must, render_guts_t* guts, int indentation) const;
  };

  //
  // NodeOperator
  enum node_operator_t {
    // Mathematical operators
    COMMA, RSHIFT3, RSHIFT, LSHIFT, BIT_OR, BIT_XOR, BIT_AND, PLUS, MINUS, DIV, MULT, MOD,
    // Logical operators
    OR, AND,
    // Comparison operators
    EQUAL, NOT_EQUAL, STRICT_EQUAL, STRICT_NOT_EQUAL, LESS_THAN_EQUAL, GREATER_THAN_EQUAL, LESS_THAN, GREATER_THAN,
    // Other.
    IN, INSTANCEOF
  };
  class NodeOperator: public NodeExpression {
    protected:
      node_operator_t op;
    public:
      NodeOperator(node_operator_t op, const unsigned int lineno = 0);
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
      virtual Node* reduce();
      virtual bool operator== (const Node&) const;
  };

  //
  // NodeConditionalExpression
  class NodeConditionalExpression: public NodeExpression {
    public:
      NodeConditionalExpression(const unsigned int lineno = 0);
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
      virtual Node* reduce();
  };

  //
  // NodeParenthetical
  class NodeParenthetical: public NodeExpression {
    public:
      NodeParenthetical(const unsigned int lineno = 0);
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
      virtual bool isValidlVal() const;
      virtual bool compare(bool val) const;
  };

  //
  // NodeAssignment
  enum node_assignment_t {
    ASSIGN,
    MULT_ASSIGN, DIV_ASSIGN, MOD_ASSIGN, PLUS_ASSIGN, MINUS_ASSIGN,
    LSHIFT_ASSIGN, RSHIFT_ASSIGN, RSHIFT3_ASSIGN,
    BIT_AND_ASSIGN, BIT_XOR_ASSIGN, BIT_OR_ASSIGN
  };
  class NodeAssignment: public NodeExpression {
    protected:
      node_assignment_t op;
    public:
      NodeAssignment(node_assignment_t op, const unsigned int lineno = 0);
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
      const node_assignment_t operatorType() const;
      virtual bool operator== (const Node&) const;
  };

  //
  // NodeUnary
  enum node_unary_t {
    DELETE, VOID, TYPEOF,
    INCR_UNARY, DECR_UNARY, PLUS_UNARY, MINUS_UNARY,
    BIT_NOT_UNARY,
    NOT_UNARY
  };
  class NodeUnary: public NodeExpression {
    protected:
      node_unary_t op;
    public:
      NodeUnary(node_unary_t op, const unsigned int lineno = 0);
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
      const node_unary_t operatorType() const;
      virtual bool operator== (const Node&) const;
  };

  //
  // NodePostfix
  enum node_postfix_t {
    INCR_POSTFIX, DECR_POSTFIX
  };
  class NodePostfix: public NodeExpression {
    protected:
      node_postfix_t op;
    public:
      NodePostfix(node_postfix_t op, const unsigned int lineno = 0);
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
      virtual bool operator== (const Node&) const;
  };

  //
  // NodeIdentifier
  class NodeIdentifier: public NodeExpression {
    protected:
      std::string _name;
    public:
      NodeIdentifier(std::string name, const unsigned int lineno = 0);
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
      virtual std::string name() const;
      virtual bool isValidlVal() const;
      virtual void rename(const std::string &str);
      virtual bool operator== (const Node&) const;
  };

  //
  // NodeFunctionCall
  class NodeFunctionCall: public NodeExpression {
    public:
      NodeFunctionCall(const unsigned int lineno = 0);
      virtual rope_t render(render_guts_t* guts, int indentation) const;
      virtual Node* clone(Node* node = NULL) const;
      virtual Node* reduce();
  };

  //
  // NodeFunctionConstructor
  class NodeFunctionConstructor: public NodeExpression {
    public:
      NodeFunctionConstructor(const unsigned int lineno = 0);
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
  };

  //
  // NodeObjectLiteral
  class NodeObjectLiteral: public NodeExpression {
    public:
      NodeObjectLiteral(const unsigned int lineno = 0);
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
  };

  //
  // NodeArrayLiteral
  class NodeArrayLiteral: public NodeExpression {
    public:
      NodeArrayLiteral(const unsigned int lineno = 0);
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
  };

  //
  // NodeStaticMemberExpression
  class NodeStaticMemberExpression: public NodeExpression {
    public:
      NodeStaticMemberExpression(const unsigned int lineno = 0);
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
      virtual bool isValidlVal() const;
  };

  //
  // NodeDynamicMemberExpression
  class NodeDynamicMemberExpression: public NodeExpression {
    public:
      NodeDynamicMemberExpression(const unsigned int lineno = 0);
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
      virtual bool isValidlVal() const;
  };

  //
  // NodeStatement (abstract)
  class NodeStatement: public Node {
    public:
      NodeStatement(const unsigned int lineno = 0);
      virtual rope_t render(render_guts_t* guts, int indentation) const = 0;
      virtual rope_t renderStatement(render_guts_t* guts, int indentation) const;
  };

  //
  // NodeStatementWithExpression
  enum node_statement_with_expression_t {
    RETURN, CONTINUE, BREAK, THROW
  };
  class NodeStatementWithExpression: public NodeStatement {
    protected:
      node_statement_with_expression_t statement;
    public:
      NodeStatementWithExpression(node_statement_with_expression_t statement, const unsigned int lineno = 0);
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
      virtual bool operator== (const Node&) const;
  };

  //
  // NodeVarDeclaration
  class NodeVarDeclaration: public NodeStatement {
    protected:
      bool _iterator;
    public:
      NodeVarDeclaration(bool iterator = false, const unsigned int lineno = 0);
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
      bool iterator() const; // TODO: kill this
      Node* NodeVarDeclaration::setIterator(bool iterator);
  };

  //
  // NodeFunctionDeclaration
  class NodeFunctionDeclaration: public Node {
    public:
      NodeFunctionDeclaration(const unsigned int lineno = 0);
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
  };

  //
  // NodeFunctionExpression
  class NodeFunctionExpression: public NodeExpression {
    public:
      NodeFunctionExpression(const unsigned int lineno = 0);
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
  };

  //
  // NodeArgList
  class NodeArgList: public Node {
    public:
      NodeArgList(const unsigned int lineno = 0);
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
  };

  //
  // NodeIf
  class NodeIf: public Node {
    public:
      NodeIf(const unsigned int lineno = 0);
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
      virtual Node* reduce();
  };

  //
  // NodeWith
  class NodeWith: public Node {
    public:
      NodeWith(const unsigned int lineno = 0);
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
  };

  //
  // NodeTry
  class NodeTry: public Node {
    public:
      NodeTry(const unsigned int lineno = 0);
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
  };

  //
  // NodeLabel
  class NodeLabel: public Node {
    public:
      NodeLabel(const unsigned int lineno = 0);
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
      virtual rope_t renderStatement(render_guts_t* guts, int indentation) const;
  };

  //
  // NodeCaseClause
  class NodeCaseClause: public Node {
    public:
      NodeCaseClause(const unsigned int lineno = 0);
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
      virtual rope_t renderStatement(render_guts_t* guts, int indentation) const;
      virtual rope_t renderIndentedStatement(render_guts_t* guts, int indentation) const;
  };

  //
  // NodeSwitch
  class NodeSwitch: public Node {
    public:
      NodeSwitch(const unsigned int lineno = 0);
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
  };

  //
  // NodeDefaultClause
  class NodeDefaultClause: public NodeCaseClause {
    public:
      NodeDefaultClause(const unsigned int lineno = 0);
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
  };

  //
  // NodeObjectLiteralProperty
  class NodeObjectLiteralProperty: public Node {
    public:
      NodeObjectLiteralProperty(const unsigned int lineno = 0);
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
  };

  //
  // NodeForLoop
  class NodeForLoop: public Node {
    public:
      NodeForLoop(const unsigned int lineno = 0);
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
  };

  //
  // NodeForIn
  class NodeForIn: public Node {
    public:
      NodeForIn(const unsigned int lineno = 0);
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
  };

  //
  // NodeWhile
  class NodeWhile: public Node {
    public:
      NodeWhile(const unsigned int lineno = 0);
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
  };

  //
  // NodeDoWhile
  class NodeDoWhile: public NodeStatement {
    public:
      NodeDoWhile(const unsigned int lineno = 0);
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
  };

  //
  // Parser exception
  class ParseException: public std::exception {
    public:
      char error[128];
      ParseException(const std::string msg);
      const char* what() const throw();
  };
}
