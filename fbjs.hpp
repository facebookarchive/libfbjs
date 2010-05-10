#pragma once
#include <vector>
#include <set>
#include "node.hpp"

using namespace std;
using namespace fbjs;

typedef set<string> scope_t;
typedef vector<scope_t*> scope_stack_t;
struct fbjsize_guts_t {
  string app_id;
  scope_stack_t scope;
};

Node* fbjsize(Node* node, fbjsize_guts_t* guts);
void fbjs_analyze_scope(Node* node, scope_t* scope, const bool first = true);
bool fbjs_check_scope(const string identifier, scope_stack_t* scope);
