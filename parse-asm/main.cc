#include <cstddef>
#include <clink/clink.h>
#include <cstdlib>
#include <iostream>

using namespace clink;

int main(int argc, char **argv) {

  if (argc != 2) {
    std::cerr << "usage: " << argv[0] << " filename\n"
                 " test utility for Clink's assembly parser\n";
    return EXIT_FAILURE;
  }

  int rc;
  try {
    rc = parse_asm(argv[1], [](const Symbol &symbol) {

      std::cout << symbol.path << ":" << symbol.lineno << ":" << symbol.colno
        << ": ";

      switch (symbol.category) {

        case Symbol::DEFINITION:
          std::cout << "definition";
          break;

        case Symbol::INCLUDE:
          std::cout << "include";
          break;

        case Symbol::FUNCTION_CALL:
          std::cout << "function call";
          break;

        case Symbol::REFERENCE:
          std::cout << "reference";
          break;

      }

      std::cout << " of " << symbol.name << "\n";

      return 0;
    });
  } catch (Error &e) {
    std::cerr << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return rc == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
