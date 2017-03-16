#pragma once

#include <cstdio>
#include <string>
#include <vector>

int vim_open(const std::string &filename, unsigned line, unsigned col);

std::vector<std::string> vim_highlight(const std::string &filename);
