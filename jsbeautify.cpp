#include "fbjs.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
  NodeProgram root(stdin);
  cout << root.render(RENDER_PRETTY).c_str();
  return 0;
}
