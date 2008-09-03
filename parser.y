%{
#include <stdio.h>
#include <string.h>
#include <iostream>
#include "parser.hpp"
using namespace fbjs;

// TODO:
// * --funroll-loops
extern int yydebug;
#define parsererror(str) yyerror(&yylloc, yyscanner, NULL, str)
%}

%union {
  double number;
  char* string;
  node_assignment_t assignment;
  int integer;
  Node* node;
  Node* duple[2];
}

%{

void parser_note_error(void* yyscanner, const char* str) {
  fbjs_parse_extra* extra = yyget_extra(yyscanner);
  extra->errors.push_back(str);
}

void yyerror(YYLTYPE* yyloc, void* yyscanner, void* node, const char* str) {
  char loc[1024];
  sprintf(loc, "Error on line %d: %s", yyloc->first_line, str);
  parser_note_error(yyscanner, loc);
}

%}

%locations
%pure-parser
%parse-param { void* yyscanner }
%parse-param { Node* root }
%lex-param { void* yyscanner }
%glr-parser
%error-verbose

// 6: Confusion about *_no_in reductions, no merges should be required, GLR should be able to figure it out
//%expect-rr 6
//%expect 14

%token t_LCURLY t_RCURLY
%token t_LPAREN t_RPAREN
%token t_LBRACKET t_RBRACKET
%token t_SEMICOLON t_VIRTUAL_SEMICOLON
%token t_BREAK t_CASE t_CATCH t_CONTINUE t_DEFAULT t_DO t_FINALLY t_FOR
%token t_FUNCTION t_IF t_IN t_INSTANCEOF t_RETURN t_SWITCH t_THIS t_THROW t_TRY
%token t_VAR t_WHILE t_WITH t_CONST t_NULL t_FALSE t_TRUE

%token t_OBJECT_LITERAL_HACK t_FUNCTION_HACK

%token<number> t_NUMBER
%token<string> t_IDENTIFIER t_REGEX t_STRING

%nonassoc p_IF
%nonassoc t_ELSE

%left t_COMMA
%left p_ABOVE_COMMA
%right t_RSHIFT3_ASSIGN t_RSHIFT_ASSIGN t_LSHIFT_ASSIGN t_BIT_XOR_ASSIGN t_BIT_OR_ASSIGN t_BIT_AND_ASSIGN t_MOD_ASSIGN t_DIV_ASSIGN t_MULT_ASSIGN t_MINUS_ASSIGN t_PLUS_ASSIGN t_ASSIGN p_COMPOUND_ASSIGNMENT
%right t_PLING t_COLON
%right p_TERNARY
%left t_OR
%left t_AND
%left t_BIT_OR
%left t_BIT_XOR
%left t_BIT_AND
%left t_EQUAL t_NOT_EQUAL t_STRICT_EQUAL t_STRICT_NOT_EQUAL
%left t_LESS_THAN_EQUAL t_GREATER_THAN_EQUAL t_LESS_THAN t_GREATER_THAN t_IN t_INSTANCEOF
%left t_LSHIFT t_RSHIFT t_RSHIFT3
%left t_PLUS t_MINUS
%left t_DIV t_MULT t_MOD
%left t_NOT t_BIT_NOT t_INCR t_DECR p_UNARY t_DELETE t_TYPEOF t_VOID
%nonassoc p_INCR_POST p_DECR_POST
%left t_NEW t_PERIOD

%start program

%type<node> literal null_literal boolean_literal numeric_literal regex_literal string_literal primary_expression array_literal element_list object_literal property_name_and_value_list property_name member_expression new_expression call_expression arguments argument_list left_hand_side_expression postfix_expression unary_expression multiplicative_expression additive_expression shift_expression relational_expression relational_expression_no_in equality_expression equality_expression_no_in bitwise_and_expression bitwise_and_expression_no_in bitwise_xor_expression bitwise_xor_expression_no_in bitwise_or_expression bitwise_or_expression_no_in logical_and_expression logical_and_expression_no_in logical_or_expression logical_or_expression_no_in conditional_expression conditional_expression_no_in assignment_expression assignment_expression_no_in expression expression_opt expression_no_in_opt expression_no_in statement block statement_list source_element variable_statement variable_declaration_list variable_declaration_list_no_in variable_declaration variable_declaration_no_in initializer initializer_no_in empty_statement expression_statement if_statement iteration_statement continue_statement break_statement return_statement with_statement switch_statement case_block case_clause case_clauses_with_default case_clauses_no_default default_clause labelled_statement throw_statement try_statement finally function_expression function_declaration formal_parameter_list identifier function_body
%type<duple> catch
%type<assignment> assignment_operator
%type<integer> elison
%%
program:
    statement_list {
      root->appendChild($1);
    }
;

semicolon:
    t_SEMICOLON
|   t_VIRTUAL_SEMICOLON
;

statement_list:
    source_element {
      $$ = (new NodeStatementList())->appendChild($1);
    }
|   statement_list source_element {
      $$ = $1->appendChild($2);
    }
;

source_element:
    statement
|   function_declaration
;

literal:
    null_literal
|   boolean_literal
|   numeric_literal
|   string_literal
|   regex_literal /* this isn't an expansion of literal in ECMA-262... mistake? */
;

null_literal:
    t_NULL {
      $$ = new NodeNullLiteral();
    }
;

boolean_literal:
    t_TRUE {
      $$ = new NodeBooleanLiteral(true);
    }
|   t_FALSE {
      $$ = new NodeBooleanLiteral(false);
    }
;

numeric_literal:
    t_NUMBER {
      $$ = new NodeNumericLiteral($1);
    }
;

regex_literal:
    t_REGEX {
      // note: i'm lazy here and just reuse the string_literal class
      $$ = new NodeStringLiteral($1, true);
      free($1);
    }
;

string_literal:
    t_STRING {
      $$ = new NodeStringLiteral($1, true);
      free($1);
    }
;

identifier:
    t_IDENTIFIER {
      $$ = new NodeIdentifier($1);
      free($1);
    }
;

primary_expression:
    t_THIS {
      $$ = new NodeThis();
    }
|   identifier
|   literal
|   array_literal
|   object_literal
|   t_LPAREN expression t_RPAREN {
      $$ = (new NodeParenthetical())->appendChild($2);
    }
;

array_literal:
    t_LBRACKET elison t_RBRACKET {
      $$ = (new NodeArrayLiteral());
      for (int i = 0; i < $2 + 1; i++) {
        $$->appendChild(new NodeEmptyExpression());
      }
    }
|   t_LBRACKET t_RBRACKET {
      $$ = new NodeArrayLiteral();
    }
|   t_LBRACKET element_list t_RBRACKET {
      $$ = $2;
    }
|   t_LBRACKET element_list elison t_RBRACKET {
       $$ = $2;
       for (int i = 0; i < $3; i++) {
         $$->appendChild(new NodeEmptyExpression());
       }
    }
;

element_list:
    elison assignment_expression {
      $$ = (new NodeArrayLiteral());
      for (int i = 0; i < $1; i++) {
        $$->appendChild(new NodeEmptyExpression());
      }
      $$->appendChild($2);
    }
|   assignment_expression {
      $$ = (new NodeArrayLiteral())->appendChild($1);
    }
|   element_list elison assignment_expression {
      $$ = $1;
      for (int i = 1; i < $2; i++) {
        $$->appendChild(new NodeEmptyExpression());
      }
      $$->appendChild($3);
    }
/*|   element_list assignment_expression {
      $$ = $1->appendChild($2);
    }*/
;

elison:
    t_COMMA {
      $$ = 1;
    }
|   elison t_COMMA {
      $$++;
    }
;

object_literal:
    t_OBJECT_LITERAL_HACK t_LCURLY t_RCURLY {
      $$ = new NodeObjectLiteral();
    }
|   t_OBJECT_LITERAL_HACK t_LCURLY property_name_and_value_list t_VIRTUAL_SEMICOLON t_RCURLY { /* note the t_VIRTUAL_SEMICOLON hack */
      $$ = $3;
    }
;

property_name_and_value_list:
    property_name t_COLON assignment_expression {
      $$ = (new NodeObjectLiteral())->appendChild((new NodeObjectLiteralProperty())->appendChild($1)->appendChild($3));
    }
|   property_name_and_value_list t_COMMA property_name t_COLON assignment_expression {
      $$ = $1->appendChild((new NodeObjectLiteralProperty())->appendChild($3)->appendChild($5));
    }
;

property_name:
    identifier
|   string_literal
|   numeric_literal
;

member_expression:
    primary_expression
|   t_FUNCTION_HACK function_expression {
      $$ = $2;
    }
|   member_expression t_LBRACKET expression t_RBRACKET {
      $$ = (new NodeDynamicMemberExpression())->appendChild($1)->appendChild($3);
    }
|   member_expression t_PERIOD identifier {
      $$ = (new NodeStaticMemberExpression())->appendChild($1)->appendChild($3);
    }
|   t_NEW member_expression arguments {
      $$ = (new NodeFunctionConstructor())->appendChild($2)->appendChild($3);
    }
;

new_expression:
    member_expression
|   t_NEW new_expression {
      $$ = (new NodeFunctionConstructor())->appendChild($2)->appendChild(new NodeArgList());
    }
;

call_expression:
    member_expression arguments {
      $$ = (new NodeFunctionCall())->appendChild($1)->appendChild($2);
    }
|   call_expression arguments {
      $$ = (new NodeFunctionCall())->appendChild($1)->appendChild($2);
    }
|   call_expression t_LBRACKET expression t_RBRACKET {
      $$ = (new NodeDynamicMemberExpression())->appendChild($1)->appendChild($3);
    }
|   call_expression t_PERIOD identifier {
      $$ = (new NodeStaticMemberExpression())->appendChild($1)->appendChild($3);
    }
;

arguments:
    t_LPAREN t_RPAREN {
      $$ = new NodeArgList();
    }
|   t_LPAREN argument_list t_RPAREN {
      $$ = $2;
    }
;

argument_list:
    assignment_expression {
      $$ = (new NodeArgList())->appendChild($1);
    }
|   argument_list t_COMMA assignment_expression {
      $$ = $1->appendChild($3);
    }
;

left_hand_side_expression:
    new_expression
|   call_expression
;

postfix_expression:
    left_hand_side_expression
|   left_hand_side_expression t_INCR {
      $$ = (new NodePostfix(INCR_POSTFIX))->appendChild($1);
    }
|   left_hand_side_expression t_DECR {
      $$ = (new NodePostfix(DECR_POSTFIX))->appendChild($1);
    }
;

unary_expression:
    postfix_expression
|   t_DELETE unary_expression {
      $$ = (new NodeUnary(DELETE))->appendChild($2);
    }
|   t_VOID unary_expression {
      $$ = (new NodeUnary(VOID))->appendChild($2);
    }
|   t_TYPEOF unary_expression {
      $$ = (new NodeUnary(TYPEOF))->appendChild($2);
    }
|   t_INCR unary_expression {
      $$ = (new NodeUnary(INCR_UNARY))->appendChild($2);
      if ($2->identifier() == NULL) {
        parsererror("invalid increment operand");
        $$ = NULL;
      }
    }
|   t_DECR unary_expression {
      $$ = (new NodeUnary(DECR_UNARY))->appendChild($2);
      if ($2->identifier() == NULL) {
        parsererror("invalid decrement operand");
        $$ = NULL;
      }
    }
|   t_PLUS unary_expression {
      $$ = (new NodeUnary(PLUS_UNARY))->appendChild($2);
    }
|   t_MINUS unary_expression {
      $$ = (new NodeUnary(MINUS_UNARY))->appendChild($2);
    }
|   t_BIT_NOT unary_expression {
      $$ = (new NodeUnary(BIT_NOT_UNARY))->appendChild($2);
    }
|   t_NOT unary_expression {
      $$ = (new NodeUnary(NOT_UNARY))->appendChild($2);
    }
;

multiplicative_expression:
    unary_expression
|   multiplicative_expression t_MULT unary_expression {
      $$ = (new NodeOperator(MULT))->appendChild($1)->appendChild($3);
    }
|   multiplicative_expression t_DIV unary_expression {
      $$ = (new NodeOperator(DIV))->appendChild($1)->appendChild($3);
    }
|   multiplicative_expression t_MOD unary_expression {
      $$ = (new NodeOperator(MOD))->appendChild($1)->appendChild($3);
    }
;

additive_expression:
    multiplicative_expression
|   additive_expression t_PLUS multiplicative_expression {
      $$ = (new NodeOperator(PLUS))->appendChild($1)->appendChild($3);
    }
|   additive_expression t_MINUS multiplicative_expression {
      $$ = (new NodeOperator(MINUS))->appendChild($1)->appendChild($3);
    }
;

shift_expression:
    additive_expression
|   shift_expression t_LSHIFT additive_expression {
      $$ = (new NodeOperator(LSHIFT))->appendChild($1)->appendChild($3);
    }
|   shift_expression t_RSHIFT additive_expression {
      $$ = (new NodeOperator(RSHIFT))->appendChild($1)->appendChild($3);
    }
|   shift_expression t_RSHIFT3 additive_expression {
      $$ = (new NodeOperator(RSHIFT3))->appendChild($1)->appendChild($3);
    }
;

relational_expression:
    shift_expression
|   relational_expression t_LESS_THAN shift_expression {
      $$ = (new NodeOperator(LESS_THAN))->appendChild($1)->appendChild($3);
    }
|   relational_expression t_GREATER_THAN shift_expression {
      $$ = (new NodeOperator(GREATER_THAN))->appendChild($1)->appendChild($3);
    }
|   relational_expression t_LESS_THAN_EQUAL shift_expression {
      $$ = (new NodeOperator(LESS_THAN_EQUAL))->appendChild($1)->appendChild($3);
    }
|   relational_expression t_GREATER_THAN_EQUAL shift_expression {
      $$ = (new NodeOperator(GREATER_THAN_EQUAL))->appendChild($1)->appendChild($3);
    }
|   relational_expression t_INSTANCEOF shift_expression {
      $$ = (new NodeOperator(INSTANCEOF))->appendChild($1)->appendChild($3);
    }
|   relational_expression t_IN shift_expression {
      $$ = (new NodeOperator(IN))->appendChild($1)->appendChild($3);
    }
;

relational_expression_no_in:
    shift_expression
|   relational_expression t_LESS_THAN shift_expression {
      $$ = (new NodeOperator(LESS_THAN))->appendChild($1)->appendChild($3);
    }
|   relational_expression t_GREATER_THAN shift_expression {
      $$ = (new NodeOperator(GREATER_THAN))->appendChild($1)->appendChild($3);
    }
|   relational_expression t_LESS_THAN_EQUAL shift_expression {
      $$ = (new NodeOperator(LESS_THAN_EQUAL))->appendChild($1)->appendChild($3);
    }
|   relational_expression t_GREATER_THAN_EQUAL shift_expression {
      $$ = (new NodeOperator(GREATER_THAN_EQUAL))->appendChild($1)->appendChild($3);
    }
|   relational_expression t_INSTANCEOF shift_expression {
      $$ = (new NodeOperator(INSTANCEOF))->appendChild($1)->appendChild($3);
    }
;

equality_expression:
    relational_expression
|   equality_expression t_EQUAL relational_expression {
      $$ = (new NodeOperator(EQUAL))->appendChild($1)->appendChild($3);
    }
|   equality_expression t_NOT_EQUAL relational_expression {
      $$ = (new NodeOperator(NOT_EQUAL))->appendChild($1)->appendChild($3);
    }
|   equality_expression t_STRICT_EQUAL relational_expression {
      $$ = (new NodeOperator(STRICT_EQUAL))->appendChild($1)->appendChild($3);
    }
|   equality_expression t_STRICT_NOT_EQUAL relational_expression {
      $$ = (new NodeOperator(STRICT_NOT_EQUAL))->appendChild($1)->appendChild($3);
    }
;

equality_expression_no_in:
    relational_expression_no_in
|   equality_expression t_EQUAL relational_expression_no_in {
      $$ = (new NodeOperator(EQUAL))->appendChild($1)->appendChild($3);
    }
|   equality_expression t_NOT_EQUAL relational_expression_no_in {
      $$ = (new NodeOperator(NOT_EQUAL))->appendChild($1)->appendChild($3);
    }
|   equality_expression t_STRICT_EQUAL relational_expression_no_in {
      $$ = (new NodeOperator(STRICT_EQUAL))->appendChild($1)->appendChild($3);
    }
|   equality_expression t_STRICT_NOT_EQUAL relational_expression_no_in {
      $$ = (new NodeOperator(STRICT_NOT_EQUAL))->appendChild($1)->appendChild($3);
    }
;

bitwise_and_expression:
    equality_expression
|   bitwise_and_expression t_BIT_AND equality_expression {
      $$ = (new NodeOperator(BIT_AND))->appendChild($1)->appendChild($3);
    }
;

bitwise_and_expression_no_in:
    equality_expression_no_in
|   bitwise_and_expression_no_in t_BIT_AND equality_expression {
      $$ = (new NodeOperator(BIT_AND))->appendChild($1)->appendChild($3);
    }
;

bitwise_xor_expression:
    bitwise_and_expression
|   bitwise_xor_expression t_BIT_XOR bitwise_and_expression {
      $$ = (new NodeOperator(BIT_XOR))->appendChild($1)->appendChild($3);
    }
;

bitwise_xor_expression_no_in:
    bitwise_and_expression_no_in
|   bitwise_xor_expression_no_in t_BIT_XOR bitwise_and_expression_no_in {
      $$ = (new NodeOperator(BIT_XOR))->appendChild($1)->appendChild($3);
    }
;

bitwise_or_expression:
    bitwise_xor_expression
|   bitwise_or_expression t_BIT_OR bitwise_xor_expression {
      $$ = (new NodeOperator(BIT_OR))->appendChild($1)->appendChild($3);
    }
;

bitwise_or_expression_no_in:
    bitwise_xor_expression_no_in
|   bitwise_or_expression_no_in t_BIT_OR bitwise_xor_expression_no_in {
      $$ = (new NodeOperator(OR))->appendChild($1)->appendChild($3);
    }
;

logical_and_expression:
    bitwise_or_expression
|   logical_and_expression t_AND bitwise_or_expression {
      $$ = (new NodeOperator(AND))->appendChild($1)->appendChild($3);
    }
;

logical_and_expression_no_in:
    bitwise_or_expression_no_in
|   logical_and_expression_no_in t_AND bitwise_or_expression_no_in {
      $$ = (new NodeOperator(AND))->appendChild($1)->appendChild($3);
    }
;

logical_or_expression:
    logical_and_expression
|   logical_or_expression t_OR logical_and_expression {
      $$ = (new NodeOperator(OR))->appendChild($1)->appendChild($3);
    }
;

logical_or_expression_no_in:
    logical_and_expression_no_in
|   logical_or_expression_no_in t_OR logical_and_expression_no_in {
      $$ = (new NodeOperator(OR))->appendChild($1)->appendChild($3);
    }
;

conditional_expression:
    logical_or_expression
|   logical_or_expression t_PLING assignment_expression t_COLON assignment_expression {
      $$ = (new NodeConditionalExpression())->appendChild($1)->appendChild($3)->appendChild($5);
    }
;

conditional_expression_no_in:
    logical_or_expression_no_in
|   logical_or_expression_no_in t_PLING assignment_expression t_COLON assignment_expression {
      $$ = (new NodeConditionalExpression())->appendChild($1)->appendChild($3)->appendChild($5);
    }
;

assignment_expression:
    conditional_expression
|   left_hand_side_expression assignment_operator assignment_expression {
      if ($1->identifier() == NULL) {
        parsererror("invalid assignment left-hand side");
        $$ = NULL;
      } else {
        $$ = (new NodeAssignment($2))->appendChild($1)->appendChild($3);
      }
    }
;

assignment_expression_no_in:
    conditional_expression_no_in
|   left_hand_side_expression assignment_operator assignment_expression_no_in {
      if ($1->identifier() == NULL) {
        parsererror("invalid assignment left-hand side");
        $$ = NULL;
      } else {
        $$ = (new NodeAssignment($2))->appendChild($1)->appendChild($3);
      }
    }
;

assignment_operator:
    t_ASSIGN {
      $$ = ASSIGN;
    }
|   t_MULT_ASSIGN {
      $$ = MULT_ASSIGN;
    }
|   t_DIV_ASSIGN {
      $$ = DIV_ASSIGN;
    }
|   t_MOD_ASSIGN {
      $$ = MOD_ASSIGN;
    }
|   t_PLUS_ASSIGN {
      $$ = PLUS_ASSIGN;
    }
|   t_MINUS_ASSIGN {
      $$ = MINUS_ASSIGN;
    }
|   t_LSHIFT_ASSIGN {
      $$ = LSHIFT_ASSIGN;
    }
|   t_RSHIFT_ASSIGN {
      $$ = RSHIFT_ASSIGN;
    }
|   t_RSHIFT3_ASSIGN {
      $$ = RSHIFT3_ASSIGN;
    }
|   t_BIT_AND_ASSIGN {
      $$ = BIT_AND_ASSIGN;
    }
|   t_BIT_XOR_ASSIGN {
      $$ = BIT_XOR_ASSIGN;
    }
|   t_BIT_OR_ASSIGN {
      $$ = BIT_OR_ASSIGN;
    }
;

expression:
    assignment_expression
|   expression t_COMMA assignment_expression {
      $$ = (new NodeOperator(COMMA))->appendChild($1)->appendChild($3);
    }
;

expression_opt:
    /* empty */ {
      $$ = new NodeEmptyExpression();
    }
|   expression
;

expression_no_in:
    assignment_expression_no_in
|   expression_no_in t_COMMA assignment_expression_no_in {
      $$ = (new NodeOperator(COMMA))->appendChild($1)->appendChild($3);
    }
;

expression_no_in_opt:
    /* empty */ {
      $$ = new NodeEmptyExpression();
    }
|   expression_no_in
;

statement:
    block
|   variable_statement
|   empty_statement
|   expression_statement
|   if_statement
|   iteration_statement
|   continue_statement
|   break_statement
|   return_statement
|   with_statement
|   labelled_statement
|   switch_statement
|   throw_statement
|   try_statement
|   error t_SEMICOLON {
      $$ = new NodeEmptyExpression();
    }
;

block:
    t_LCURLY statement_list t_RCURLY {
      $$ = (new NodeBlock())->appendChild($2);
    }
|   t_LCURLY t_RCURLY {
      $$ = (new NodeBlock())->appendChild(new NodeStatementList());
    }
;

variable_statement:
    t_VAR variable_declaration_list semicolon {
      $$ = $2;
    }
;

variable_declaration_list:
    variable_declaration {
      $$ = (new NodeVarDeclaration())->appendChild($1);
    }
|   variable_declaration_list t_COMMA variable_declaration {
      $$->appendChild($3);
    }
;

variable_declaration_list_no_in:
    variable_declaration_no_in {
      $$ = (new NodeVarDeclaration())->appendChild($1);
    }
|   variable_declaration_list_no_in t_COMMA variable_declaration_no_in {
      $$->appendChild($3);
    }
;

variable_declaration:
    identifier initializer {
      $$ = (new NodeAssignment(ASSIGN))->appendChild($1)->appendChild($2);
    }
|   identifier
;

variable_declaration_no_in:
    identifier initializer_no_in {
      $$ = (new NodeAssignment(ASSIGN))->appendChild($1)->appendChild($2);
    }
|   identifier
;

initializer:
    t_ASSIGN assignment_expression {
      $$ = $2;
    }
;

initializer_no_in:
    t_ASSIGN assignment_expression_no_in {
      $$ = $2;
    }
;

empty_statement:
    semicolon {
      $$ = new NodeEmptyExpression();
    }
;

expression_statement:
    expression semicolon {
      $$ = $1;
    }
;

if_statement:
    t_IF t_LPAREN expression t_RPAREN statement t_ELSE statement {
      $$ = (new NodeIf())->appendChild($3)->appendChild($5)->appendChild($7);
    }
|   t_IF t_LPAREN expression t_RPAREN statement %prec p_IF {
      $$ = (new NodeIf())->appendChild($3)->appendChild($5)->appendChild(NULL);
    }
;

iteration_statement:
    t_DO statement t_WHILE t_LPAREN expression t_RPAREN semicolon {
      $$ = (new NodeDoWhile())->appendChild($2)->appendChild($5);
    }
|   t_WHILE t_LPAREN expression t_RPAREN statement {
      $$ = (new NodeWhile())->appendChild($3)->appendChild($5);
    }
|   t_FOR t_LPAREN expression_no_in_opt t_SEMICOLON expression_opt t_SEMICOLON expression_opt t_RPAREN statement {
      $$ = (new NodeForLoop())->appendChild($3)->appendChild($5)->appendChild($7)->appendChild($9);
    }
|   t_FOR t_LPAREN t_VAR variable_declaration_list_no_in t_SEMICOLON expression_opt t_SEMICOLON expression_opt t_RPAREN statement {
      $$ = (new NodeForLoop())->appendChild($4)->appendChild($6)->appendChild($8)->appendChild($10);
    }
|   t_FOR t_LPAREN left_hand_side_expression t_IN expression t_RPAREN statement {
      $$ = (new NodeForIn())->appendChild($3)->appendChild($5)->appendChild($7);
    }
|   t_FOR t_LPAREN t_VAR variable_declaration_list_no_in t_IN expression t_RPAREN statement {
      $$ = (new NodeForIn())->appendChild($4)->appendChild($6)->appendChild($8);
    }
;

continue_statement:
    t_CONTINUE identifier semicolon {
      $$ = (new NodeStatementWithExpression(CONTINUE))->appendChild($2);
    }
|   t_CONTINUE semicolon {
      $$ = (new NodeStatementWithExpression(CONTINUE))->appendChild(NULL);
    }
;

break_statement:
    t_BREAK identifier semicolon {
      $$ = (new NodeStatementWithExpression(BREAK))->appendChild($2);
    }
|   t_BREAK semicolon {
      $$ = (new NodeStatementWithExpression(BREAK))->appendChild(NULL);
    }
;

return_statement:
    t_RETURN expression semicolon {
      $$ = (new NodeStatementWithExpression(RETURN))->appendChild($2);
    }
|   t_RETURN semicolon {
      $$ = (new NodeStatementWithExpression(RETURN))->appendChild(NULL);
    }
;

with_statement:
    t_WITH t_LPAREN expression t_RPAREN statement {
      parsererror("wat.");
    }
;

switch_statement:
    t_SWITCH t_LPAREN expression t_RPAREN case_block {
      $$ = (new NodeSwitch())->appendChild($3)->appendChild($5);
    }
;

case_block:
    t_LCURLY case_clauses_with_default t_RCURLY {
      $$ = $2;
    }
|   t_LCURLY case_clauses_no_default t_RCURLY {
      $$ = $2;
    }
|   t_LCURLY t_RCURLY {
      $$ = new NodeCaseClauseList();
    }
;

case_clauses_with_default:
    case_clauses_no_default default_clause {
      $$ = $1->appendChild($2);
    }
|   default_clause {
      $$ = (new NodeCaseClause())->appendChild($1);
    }
;

case_clauses_no_default:
    case_clause {
      $$ = (new NodeCaseClauseList())->appendChild($1);
    }
|   case_clauses_no_default case_clause {
      $$ = $1->appendChild($2);
    }
;

case_clause:
    t_CASE expression t_COLON statement_list {
      $$ = (new NodeCaseClause())->appendChild($2)->appendChild($4);
    }
|   t_CASE expression t_COLON {
      $$ = (new NodeCaseClause())->appendChild($2)->appendChild(NULL);
    }
;

default_clause:
    t_DEFAULT t_COLON statement_list {
      $$ = (new NodeDefaultClause())->appendChild($3);
    }
|   t_DEFAULT t_COLON {
      $$ = (new NodeDefaultClause())->appendChild(NULL);
    }
;

labelled_statement:
    identifier t_COLON statement {
      $$ = (new NodeLabel())->appendChild($1)->appendChild($3);
    }
;

throw_statement:
    t_THROW expression semicolon {
      $$ = (new NodeStatementWithExpression(THROW))->appendChild($2);
    }
;

try_statement:
    t_TRY block catch {
      $$ = (new NodeTry())->appendChild($2)->appendChild($3[0])->appendChild($3[1])->appendChild(NULL);
    }
|   t_TRY block finally {
      $$ = (new NodeTry())->appendChild($2)->appendChild(NULL)->appendChild(NULL)->appendChild($3);
    }
|   t_TRY block catch finally {
      $$ = (new NodeTry())->appendChild($2)->appendChild($3[0])->appendChild($3[1])->appendChild($4);
    }
;

catch:
    t_CATCH t_LPAREN identifier t_RPAREN block {
      $$[0] = $3;
      $$[1] = $5;
    }
;

finally:
    t_FINALLY block {
      $$ = $2;
    }
;

// there's a bug in Firefox and IE that will parse the following code correctly:
// function(){}function(){}
// according to ECMA-262, this should be a syntax error, however it seems those browsers don't differentiate
// between function_expression and function_declaration. the syntax for function_declaration is looser than
// function_expression, for instance according to ECMA, this is NOT a syntax error:
// function foo(){}function bar(){}
// here, we go with ECMA-262.
function_declaration:
    t_FUNCTION identifier t_LPAREN formal_parameter_list t_RPAREN t_LCURLY function_body t_RCURLY {
      $$ = (new NodeFunction(true))->appendChild($2)->appendChild($4)->appendChild($7);
    }
|   t_FUNCTION identifier t_LPAREN t_RPAREN t_LCURLY function_body t_RCURLY {
      $$ = (new NodeFunction(true))->appendChild($2)->appendChild(new NodeArgList())->appendChild($6);
    }
;
function_expression:
    t_FUNCTION identifier t_LPAREN formal_parameter_list t_RPAREN t_LCURLY function_body t_RCURLY {
      $$ = (new NodeFunction())->appendChild($2)->appendChild($4)->appendChild($7);
    }
|   t_FUNCTION identifier t_LPAREN t_RPAREN t_LCURLY function_body t_RCURLY {
      $$ = (new NodeFunction())->appendChild($2)->appendChild(new NodeArgList())->appendChild($6);
    }
|   t_FUNCTION t_LPAREN formal_parameter_list t_RPAREN t_LCURLY function_body t_RCURLY {
      $$ = (new NodeFunction())->appendChild(NULL)->appendChild($3)->appendChild($6);
    }
|   t_FUNCTION t_LPAREN t_RPAREN t_LCURLY function_body t_RCURLY {
      $$ = (new NodeFunction())->appendChild(NULL)->appendChild(new NodeArgList())->appendChild($5);
    }
;

formal_parameter_list:
    identifier {
      $$ = (new NodeArgList())->appendChild($1);
    }
|   formal_parameter_list t_COMMA identifier {
      $$ = $1->appendChild($3);
    }
;

function_body:
    /* empty */ {
      $$ = new NodeStatementList();
    }
|   statement_list;
;
