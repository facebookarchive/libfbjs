#pragma once
#include <stdio.h>
#include <stdexcept>
#include <string>
#include <sstream>
#include <list>
#include <ext/rope>

using namespace std;
typedef __gnu_cxx::rope<char> rope_t;

namespace fbjs {
  class Node;
  typedef list<Node*> node_list_t;
  typedef enum {
    RENDER_NONE = 0,
    RENDER_PRETTY = 1,
  } node_render_enum;
  struct render_guts_t {
    int lineno;
    bool pretty;
  };

  //
  // Node
  class Node {
    protected:
      node_list_t _childNodes;
      virtual rope_t renderImplodeChildren(render_guts_t* guts, int indentation, const char* glue) const;

    public:
      Node();
      Node(const char* code);
      Node(FILE* file);
      virtual ~Node();
      virtual Node* clone(Node* node = NULL) const;

      virtual Node* identifier();
      bool empty() const;

      node_list_t& Node::childNodes();
      Node* appendChild(Node* node);
      Node* removeChild(node_list_t::iterator node_pos);
      Node* replaceChild(Node* node, node_list_t::iterator node_pos);
      Node* insertBefore(Node* node, node_list_t::iterator node_pos);

      virtual rope_t render(node_render_enum opts = RENDER_NONE) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
      virtual rope_t renderBlock(bool must, render_guts_t* guts, int indentation) const;
      virtual rope_t renderStatement(render_guts_t* guts, int indentation) const;
      virtual rope_t renderIndentedStatement(render_guts_t* guts, int indentation) const;
  };

  //
  // NodeStatementList
  class NodeStatementList: public Node {
    public:
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
      virtual rope_t renderBlock(bool must, render_guts_t* guts, int indentation) const;
      virtual rope_t renderStatement(render_guts_t* guts, int indentation) const;
      virtual rope_t renderIndentedStatement(render_guts_t* guts, int indentation) const;
  };

  //
  // NodeExpression (abstract)
  class NodeExpression: public Node {
    public:
      virtual rope_t render(render_guts_t* guts, int indentation) const = 0;
      virtual rope_t renderStatement(render_guts_t* guts, int indentation) const;
  };

  //
  // NodeNumericLiteral
  class NodeNumericLiteral: public NodeExpression {
    protected:
      double value;
    public:
      NodeNumericLiteral(double value);
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
  };

  //
  // NodeStringLiteral
  class NodeStringLiteral: public NodeExpression {
    protected:
      string value;
      bool quoted;
    public:
      NodeStringLiteral(string value, bool quoted);
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
  };

  //
  // NodeBooleanLiteral
  class NodeBooleanLiteral: public NodeExpression {
    protected:
      bool value;
    public:
      NodeBooleanLiteral(bool value);
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
  };

  //
  // NodeNullLiteral
  class NodeNullLiteral: public NodeExpression {
    public:
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
  };

  //
  // NodeThis
  class NodeThis: public NodeExpression {
    public:
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
  };

  //
  // NodeEmptyExpression
  class NodeEmptyExpression: public NodeExpression {
    public:
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
      virtual rope_t renderBlock(bool must, render_guts_t* guts, int indentation) const;
  };

  //
  // NodeOperator
  typedef enum {
    // Mathematical operators
    COMMA, RSHIFT3, RSHIFT, LSHIFT, BIT_OR, BIT_XOR, BIT_AND, PLUS, MINUS, DIV, MULT, MOD,
    // Logical operators
    OR, AND,
    // Comparison operators
    EQUAL, NOT_EQUAL, STRICT_EQUAL, STRICT_NOT_EQUAL, LESS_THAN_EQUAL, GREATER_THAN_EQUAL, LESS_THAN, GREATER_THAN,
    // Other.
    IN, INSTANCEOF
  } node_operator_t;
  class NodeOperator: public NodeExpression {
    protected:
      node_operator_t op;
    public:
      NodeOperator(node_operator_t op);
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
  };

  //
  // NodeConditionalExpression
  class NodeConditionalExpression: public NodeExpression {
    public:
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
  };

  //
  // NodeParenthetical
  class NodeParenthetical: public NodeExpression {
    public:
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
      virtual Node* identifier();
  };

  //
  // NodeAssignment
  typedef enum {
    ASSIGN,
    MULT_ASSIGN, DIV_ASSIGN, MOD_ASSIGN, PLUS_ASSIGN, MINUS_ASSIGN,
    LSHIFT_ASSIGN, RSHIFT_ASSIGN, RSHIFT3_ASSIGN,
    BIT_AND_ASSIGN, BIT_XOR_ASSIGN, BIT_OR_ASSIGN
  } node_assignment_t;
  class NodeAssignment: public NodeExpression {
    protected:
      node_assignment_t op;
    public:
      NodeAssignment(node_assignment_t op);
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
      const node_assignment_t operatorType() const;
  };

  //
  // NodeUnary
  typedef enum {
    DELETE, VOID, TYPEOF,
    INCR_UNARY, DECR_UNARY, PLUS_UNARY, MINUS_UNARY,
    BIT_NOT_UNARY,
    NOT_UNARY
  } node_unary_t;
  class NodeUnary: public NodeExpression {
    protected:
      node_unary_t op;
    public:
      NodeUnary(node_unary_t op);
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
  };

  //
  // NodePostfix
  typedef enum {
    INCR_POSTFIX, DECR_POSTFIX
  } node_postfix_t;
  class NodePostfix: public NodeExpression {
    protected:
      node_postfix_t op;
    public:
      NodePostfix(node_postfix_t op);
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
  };

  //
  // NodeIdentifier
  class NodeIdentifier: public NodeExpression {
    protected:
      string _name;
    public:
      NodeIdentifier(string name);
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
      virtual string name() const;
      virtual Node* identifier();
  };

  //
  // NodeFunctionCall
  class NodeFunctionCall: public NodeExpression {
    public:
      virtual rope_t render(render_guts_t* guts, int indentation) const;
      virtual Node* clone(Node* node = NULL) const;
  };

  //
  // NodeFunctionConstructor
  class NodeFunctionConstructor: public NodeExpression {
    public:
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
  };

  //
  // NodeObjectLiteral
  class NodeObjectLiteral: public NodeExpression {
    public:
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
  };

  //
  // NodeArrayLiteral
  class NodeArrayLiteral: public NodeExpression {
    public:
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
  };

  //
  // NodeStaticMemberExpression
  class NodeStaticMemberExpression: public NodeExpression {
    protected:
      bool isAssignment;
    public:
      NodeStaticMemberExpression();
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
      Node* identifier();
  };

  //
  // NodeDynamicMemberExpression
  class NodeDynamicMemberExpression: public NodeExpression {
    protected:
      bool isAssignment;
    public:
      NodeDynamicMemberExpression();
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
      Node* identifier();
  };

  //
  // NodeStatement (abstract)
  class NodeStatement: public Node {
    public:
      virtual rope_t render(render_guts_t* guts, int indentation) const = 0;
      virtual rope_t renderStatement(render_guts_t* guts, int indentation) const;
  };

  //
  // NodeStatementWithExpression
  typedef enum {
    RETURN, CONTINUE, BREAK, THROW
  } node_statement_with_expression_t;
  class NodeStatementWithExpression: public NodeStatement {
    protected:
      node_statement_with_expression_t statement;
    public:
      NodeStatementWithExpression(node_statement_with_expression_t statement);
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
  };

  //
  // NodeVarDeclaration
  class NodeVarDeclaration: public NodeStatement {
    protected:
      bool _iterator;
    public:
      NodeVarDeclaration(bool iterator = false);
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
      bool iterator() const;
      Node* NodeVarDeclaration::setIterator(bool iterator);
  };

  //
  // NodeFunction
  class NodeFunction: public Node {
    protected:
      bool _declaration;
    public:
      NodeFunction(bool declaration = false);
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
      bool declaration() const;
  };

  //
  // NodeArgList
  class NodeArgList: public Node {
    public:
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
  };

  //
  // NodeIf
  class NodeIf: public Node {
    public:
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
  };

  //
  // NodeTry
  class NodeTry: public Node {
    public:
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
  };

  //
  // NodeLabel
  class NodeLabel: public Node {
    public:
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
  };

  //
  // NodeCaseClause
  class NodeCaseClause: public Node {
    public:
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
      virtual rope_t renderStatement(render_guts_t* guts, int indentation) const;
      virtual rope_t renderIndentedStatement(render_guts_t* guts, int indentation) const;
  };

  //
  // NodeSwitch
  class NodeSwitch: public Node {
    public:
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
  };

  //
  // NodeDefaultClause
  class NodeDefaultClause: public NodeCaseClause {
    public:
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
  };

  //
  // NodeCauseClause
  class NodeCauseClause: public Node {
    public:
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
  };

  //
  // NodeCauseClauseList
  class NodeCauseClauseList: public Node {
    public:
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
  };

  //
  // NodeObjectLiteralProperty
  class NodeObjectLiteralProperty: public Node {
    public:
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
  };

  //
  // NodeForLoop
  class NodeForLoop: public Node {
    public:
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
  };

  //
  // NodeForIn
  class NodeForIn: public Node {
    public:
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
  };

  //
  // NodeWhile
  class NodeWhile: public Node {
    public:
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
  };

  //
  // NodeDoWhile
  class NodeDoWhile: public NodeStatement {
    public:
      virtual Node* clone(Node* node = NULL) const;
      virtual rope_t render(render_guts_t* guts, int indentation) const;
  };
}
