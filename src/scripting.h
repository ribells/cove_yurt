#pragma once

#include <string>
#include <vector>
using namespace std;

int python_init(string Path);
int python_call(vector<string> FuncCall);
int python_exit();

