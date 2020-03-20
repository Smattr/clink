#include <climits>
#include <clink/clink.h>
#include <cstdlib>
#include <iostream>

using namespace clink;

int main(int argc, char **argv) {

  if (argc != 4) {
    std::cerr << "usage: " << argv[0] << " filename lineno colno\n";
    return EXIT_FAILURE;
  }

  const char *filename = argv[1];

  unsigned long lineno = strtoul(argv[2], nullptr, 10);
  if (lineno == 0 || lineno == ULONG_MAX) {
    std::cerr << "invalid line number " << argv[2] << "\n";
    return EXIT_FAILURE;
  }

  unsigned long colno = strtoul(argv[3], nullptr, 10);
  if (colno == 0 || colno == ULONG_MAX) {
    std::cerr << "invalid column number " << argv[3] << "\n";
    return EXIT_FAILURE;
  }

  return vim_open(filename, lineno, colno);
}
