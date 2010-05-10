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
  typedef struct {
    bool indentation;
    int pretty;
  } node_render_opts_t;

  //
  // Node
  class Node {
    protected:
      node_list_t _childNodes;

    public:
      Node();
      virtual ~Node();
      virtual Node* clone(Node* node = NULL);

      virtual Node* identifier();
      bool empty() const;

      node_list_t& childNodes();
      Node* appendChild(Node* node);
      Node* removeChild(node_list_t::iterator node_pos);
      Node* replaceChild(Node* node, node_list_t::iterator node_pos);
      Node* insertBefore(Node* node, node_list_t::iterator node_pos);

      virtual rope_t render(node_render_opts_t* opts) const;
      virtual rope_t renderImplodeChildren(node_render_opts_t* opts, const char* glue) const;
  };

  //
  // NodeBlock
  class NodeBlock: public Node {
    protected:
      bool braces;

    public:
      NodeBlock();
      virtual rope_t render(node_render_opts_t* opts) const;
      NodeBlock* requireBraces();
  };

  //
  // NodeStatementList
  class NodeStatementList: public Node {
    public:
      virtual rope_t render(node_render_opts_t* opts) const;
  };


  //
  // NodeNumericLiteral
  class NodeNumericLiteral: public Node {
    protected:
      double value;
    public:
      NodeNumericLiteral(double value);
      virtual Node* clone(Node* node = NULL);
      virtual rope_t render(node_render_opts_t* opts) const;
  };

  //
  // NodeStringLiteral
  class NodeStringLiteral: public Node {
    protected:
      string value;
      bool quoted;
    public:
      NodeStringLiteral(string value, bool quoted);
      virtual Node* clone(Node* node = NULL);
      virtual rope_t render(node_render_opts_t* opts) const;
  };

  //
  // NodeBooleanLiteral
  class NodeBooleanLiteral: public Node {
    protected:
      bool value;
    public:
      NodeBooleanLiteral(bool value);
      virtual Node* clone(Node* node = NULL);
      virtual rope_t render(node_render_opts_t* opts) const;
  };

  //
  // NodeNullLiteral
  class NodeNullLiteral: public Node {
    public:
      virtual rope_t render(node_render_opts_t* opts) const;
  };

  //
  // NodeThis
  class NodeThis: public Node {
    public:
      virtual rope_t render(node_render_opts_t* opts) const;
  };

  //
  // NodeEmptyExpression
  class NodeEmptyExpression: public Node {
    public:
      virtual rope_t render(node_render_opts_t* opts) const;
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
  class NodeOperator: public Node {
    protected:
      node_operator_t op;
    public:
      NodeOperator(node_operator_t op);
      virtual Node* clone(Node* node = NULL);
      virtual rope_t render(node_render_opts_t* opts) const;
  };

  //
  // NodeConditionalExpression
  class NodeConditionalExpression: public Node {
    public:
      virtual rope_t render(node_render_opts_t* opts) const;
  };

  //
  // NodeParenthetical
  class NodeParenthetical: public Node {
    public:
      virtual rope_t render(node_render_opts_t* opts) const;
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
  class NodeAssignment: public Node {
    protected:
      node_assignment_t op;
    public:
      NodeAssignment(node_assignment_t op);
      virtual Node* clone(Node* node = NULL);
      virtual rope_t render(node_render_opts_t* opts) const;
      const node_assignment_t operatorType();
  };

  //
  // NodeUnary
  typedef enum {
    DELETE, VOID, TYPEOF,
    INCR_UNARY, DECR_UNARY, PLUS_UNARY, MINUS_UNARY,
    BIT_NOT_UNARY,
    NOT_UNARY
  } node_unary_t;
  class NodeUnary: public Node {
    protected:
      node_unary_t op;
    public:
      NodeUnary(node_unary_t op);
      virtual Node* clone(Node* node = NULL);
      virtual rope_t render(node_render_opts_t* opts) const;
  };

  //
  // NodePostfix
  typedef enum {
    INCR_POSTFIX, DECR_POSTFIX
  } node_postfix_t;
  class NodePostfix: public Node {
    protected:
      node_postfix_t op;
    public:
      NodePostfix(node_postfix_t op);
      virtual Node* clone(Node* node = NULL);
      virtual rope_t render(node_render_opts_t* opts) const;
  };

  //
  // NodeIdentifier
  class NodeIdentifier: public Node {
    protected:
      string _name;
    public:
      NodeIdentifier(string name);
      virtual Node* clone(Node* node = NULL);
      virtual rope_t render(node_render_opts_t* opts) const;
      virtual string name() const;
      virtual Node* identifier();
  };

  //
  // NodeArgList
  class NodeArgList: public Node {
    virtual rope_t render(node_render_opts_t* opts) const;
  };

  //
  // NodeFunction
  class NodeFunction: public Node {
    virtual rope_t render(node_render_opts_t* opts) const;
  };

  //
  // NodeFunctionCall
  class NodeFunctionCall: public Node {
    virtual rope_t render(node_render_opts_t* opts) const;
  };

  //
  // NodeFunctionConstructor
  class NodeFunctionConstructor: public Node {
    virtual rope_t render(node_render_opts_t* opts) const;
  };

  //
  // NodeIf
  class NodeIf: public Node {
    virtual rope_t render(node_render_opts_t* opts) const;
  };

  //
  // NodeTry
  class NodeTry: public Node {
    virtual rope_t render(node_render_opts_t* opts) const;
  };

  //
  // NodeStatementWithExpression
  typedef enum {
    RETURN, CONTINUE, BREAK, THROW
  } node_statement_with_expression_t;
  class NodeStatementWithExpression: public Node {
    protected:
      node_statement_with_expression_t statement;
    public:
      NodeStatementWithExpression(node_statement_with_expression_t statement);
      virtual Node* clone(Node* node = NULL);
      virtual rope_t render(node_render_opts_t* opts) const;
  };

  //
  // NodeLabel
  class NodeLabel: public Node {
    virtual rope_t render(node_render_opts_t* opts) const;
  };

  //
  // NodeCaseClause
  class NodeCaseClause: public Node {
    virtual rope_t render(node_render_opts_t* opts) const;
  };

  //
  // NodeCaseClauseList
  class NodeCaseClauseList: public Node {
    virtual rope_t render(node_render_opts_t* opts) const;
  };

  //
  // NodeSwitch
  class NodeSwitch: public Node {
    virtual rope_t render(node_render_opts_t* opts) const;
  };

  //
  // NodeDefaultClause
  class NodeDefaultClause: public Node {
    virtual rope_t render(node_render_opts_t* opts) const;
  };

  //
  // NodeCauseClause
  class NodeCauseClause: public Node {
    virtual rope_t render(node_render_opts_t* opts) const;
  };

  //
  // NodeCauseClauseList
  class NodeCauseClauseList: public Node {
    virtual rope_t render(node_render_opts_t* opts) const;
  };

  //
  // NodeVarDeclaration
  class NodeVarDeclaration: public Node {
    public:
      virtual rope_t render(node_render_opts_t* opts) const;
  };

  //
  // NodeObjectLiteral
  class NodeObjectLiteral: public Node {
    public:
      virtual rope_t render(node_render_opts_t* opts) const;
  };

  //
  // NodeObjectLiteralProperty
  class NodeObjectLiteralProperty: public Node {
    public:
      virtual rope_t render(node_render_opts_t* opts) const;
  };

  //
  // NodeArrayLiteral
  class NodeArrayLiteral: public Node {
    public:
      virtual rope_t render(node_render_opts_t* opts) const;
  };

  //
  // NodeStaticMemberExpression
  class NodeStaticMemberExpression: public Node {
    protected:
      bool isAssignment;
    public:
      NodeStaticMemberExpression();
      virtual rope_t render(node_render_opts_t* opts) const;
      Node* identifier();
  };

  //
  // NodeDynamicMemberExpression
  class NodeDynamicMemberExpression: public Node {
    protected:
      bool isAssignment;
    public:
      NodeDynamicMemberExpression();
      virtual rope_t render(node_render_opts_t* opts) const;
      Node* identifier();
  };

  //
  // NodeForLoop
  class NodeForLoop: public Node {
    public:
      virtual rope_t render(node_render_opts_t* opts) const;
  };

  //
  // NodeForIn
  class NodeForIn: public Node {
    public:
      virtual rope_t render(node_render_opts_t* opts) const;
  };

  //
  // NodeWhile
  class NodeWhile: public Node {
    public:
      virtual rope_t render(node_render_opts_t* opts) const;
  };

  //
  // NodeDoWhile
  class NodeDoWhile: public Node {
    public:
      virtual rope_t render(node_render_opts_t* opts) const;
  };
}
