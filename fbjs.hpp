/**
* Copyright (c) 2008-2009 Facebook
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* See accompanying file LICENSE.txt.
*
* @author Marcel Laverdet 
*/

//
// ATTN: If you're just using FBJS for parsing, include node.hpp! fbjs.hpp is for Facebook Platform.
#pragma once
#include <vector>
#include <set>
#include <stack>
#include <stdarg.h>
#include <assert.h>
#include "node.hpp"

using namespace std;
using namespace fbjs;

typedef set<string> scope_t;
typedef vector<scope_t*> scope_stack_t;
typedef stack<Node*> node_stack_t;
struct fbjsize_guts_t {
  string app_id;
  scope_stack_t scope;
  node_stack_t nodes;
  unsigned int forinid;
  vector<size_t> with_depth;
};
typedef pair<Node*, Node*> node_pair_t;

class NodeFBJSShield: public Node {
  virtual Node* clone(Node* node) const;
};

Node* fbjsize(Node* node, fbjsize_guts_t* guts);
node_pair_t fbjs_split_member_expression(Node* expr, fbjsize_guts_t* guts);
Node* fbjs_runtime_node(const char* fn, const unsigned int num, ...);
Node* fbjs_with_identifier(string name, bool assignment, fbjsize_guts_t* guts);
node_operator_t fbjs_assignment_op_to_expr_op(node_assignment_t op);
void fbjs_analyze_scope(Node* node, scope_t* scope);
bool fbjs_declare_functions(Node* node, Node* decl, scope_t* scope, fbjsize_guts_t* guts);
bool fbjs_check_scope(const string identifier, scope_stack_t* scope);
bool fbjs_scope_uses_arguments(Node* node);
