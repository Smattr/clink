#include <clink/clink.h>
#include <cstdlib>
#include <iostream>
#include <string>

using namespace clink;

static int print(const std::string &line) {
  std::cout << line << "\n";
  return 0;
}

int main(int argc, char **argv) {

  if (argc != 2) {
    std::cerr << "usage: " << argv[0] << " filename\n";
    return EXIT_FAILURE;
  }

  const char *filename = argv[1];

  return vim_highlight(filename, print);
}
