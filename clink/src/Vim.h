#pragma once

#include <cstdio>
#include <string>
#include <vector>

int vim_open(const std::string &filename, unsigned long line, unsigned long col);

std::vector<std::string> vim_highlight(const std::string &filename);
