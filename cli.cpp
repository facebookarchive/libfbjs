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

#include <stdio.h>
#include <iostream>
#include <string.h>
#include "fbjs.hpp"
#include "parser.hpp"

int main(int argc, char* argv[]) {

  // Mutate
  Node* root = argc > 1 ? new NodeProgram(fopen(argv[1], "r")) : new NodeProgram(stdin);
  fbjsize_guts_t guts;
  guts.app_id = argc > 1 ? argv[1] : "123";
  guts.forinid = 0;
  root = fbjsize(root, &guts);

  // Render
  rope_t rendered = root->render(RENDER_PRETTY);
  delete root;
  std::cout<<rendered;
  printf("\n");
  return 0;

}
