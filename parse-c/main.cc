#include <cstddef>
#include <clink/clink.h>
#include <cstdlib>
#include <getopt.h>
#include <iostream>
#include <string>
#include <vector>

using namespace clink;

// file to open
static std::string filename;

// list of command line arguments to pass to Clang
static std::vector<std::string> clang_args;

static void parse_args(int argc, char **argv) {

  for (;;) {

    static struct option opts[] = {
      { "include", required_argument, 0, 'I' },
      { 0, 0, 0, 0 },
    };

    int option_index = 0;
    int c = getopt_long(argc, argv, "I:", opts, &option_index);

    if (c == -1)
      break;

    switch (c) {

      case 'I':
        clang_args.emplace_back("-I");
        clang_args.emplace_back(optarg);
        break;

      case '?':
        std::cerr
          << "usage: " << argv[0] << " [-I PATH | --include PATH] filename\n"
          << " test utiltiy for Clink's C/C++ parser\n";
        exit(EXIT_FAILURE);

    }
  }

  if (optind < argc - 1) {
    std::cerr << "parsing multiple files is not supported\n";
    exit(EXIT_FAILURE);
  }

  if (optind == argc - 1)
    filename = argv[optind];

  if (filename == "") {
    std::cerr << "input file is required\n";
    exit(EXIT_FAILURE);
  }
}

int main(int argc, char **argv) {

  // parse command line arguments
  parse_args(argc, argv);

  int rc;
  try {
    rc = parse_c(filename, clang_args, [](const Symbol &symbol) {

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
    std::cerr << e.what() << " (code: " << e.code << ")\n";
    return EXIT_FAILURE;
  }

  return rc == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
