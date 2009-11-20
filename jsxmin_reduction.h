#ifndef _JSXMIN_REDUCTION_H_
#define _JSXMIN_REDUCTION_H_

#include "abstract_compiler_pass.h"
#include <map>

using namespace fbjs;
using namespace std;

typedef map<string, string> replacement_t;

namespace fbjs {
class NodeProgram;
class NodeExpression;
class Node;
}

class CodeReduction : public AbstractCompilerPass {
public:
  CodeReduction() {}
  virtual ~CodeReduction() {}
  virtual void process(NodeProgram* root);
private:
  Node* replace(Node* haystack, const Node* needle, const Node* rep);
  const NodeExpression* find_expression(const Node* node);
  bool parse_patterns(const string& s);
  replacement_t _replacement;
};

#endif
