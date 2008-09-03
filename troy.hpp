#pragma once
#include <stdio.h>
#include <iostream>
#include <set>
#include <vector>
#include "node.hpp"
#include "parser.hpp"

using namespace std;
using namespace fbjs;

typedef set<string> globals_set_t;

Node* parseresource(char* filename);
Node* externalize(Node* node);
