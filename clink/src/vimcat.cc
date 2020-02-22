#include <cstring>
#include <iostream>
#include <string>
#include <unistd.h>
#include "Vim.h"

using namespace std;

int main(int argc, char **argv) {
  if (argc != 2 || (argc > 1 && strcmp(argv[1], "--help") == 0)) {
    cerr << "usage: " << argv[0] << " <filename>\n";
    return EXIT_FAILURE;
  }

  for (const string &line : vim_highlight(argv[1]))
    cout << line;

  return EXIT_SUCCESS;
}
