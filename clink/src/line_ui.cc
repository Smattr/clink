#include <clink/clink.h>
#include <cstdlib>
#include <iostream>
#include "line_ui.h"
#include "lstrip.h"
#include <string>
#include <unistd.h>
#include <vector>

template<typename T>
static void print_leader(const std::vector<T> &vs) {
  std::cout << "cscope: " << vs.size() << " lines\n";
}

int line_ui(clink::Database &db) {

  for (;;) {

    // print prompt text
    std::cout << ">> " << std::flush;

    // read user input
    std::string line;
    if (!std::getline(std::cin, line))
      break;

    // Skip leading white space
    while (!line.empty() && isspace(line[0]))
      line = line.substr(1);

    // Ignore blank lines
    if (line == "")
      continue;

    switch (line[0]) {

      case '0': { // find symbol
        std::vector<clink::Result> vs = db.find_symbols(line.substr(1));
        print_leader(vs);
        for (const auto &s : vs) {
          std::cout << s.symbol.path << " " << s.symbol.parent << " "
            << s.symbol.lineno << " " << lstrip(s.context) << "\n";
        }
        break;
      }

      case '1': { // find definition
        std::vector<clink::Result> vs = db.find_definitions(line.substr(1));
        print_leader(vs);
        for (const auto &s : vs) {
          std::cout << s.symbol.path << " " << line.substr(1) << " "
            << s.symbol.lineno << " " << lstrip(s.context) << "\n";
        }
        break;
      }

      case '2': { // find calls
        std::vector<clink::Result> vs = db.find_calls(line.substr(1));
        print_leader(vs);
        for (const auto &s : vs) {
          std::cout << s.symbol.path << " " << s.symbol.name << " "
            << s.symbol.lineno << " " << lstrip(s.context) << "\n";
        }
        break;
      }

      case '3': { // find callers
        std::vector<clink::Result> vs = db.find_callers(line.substr(1));
        print_leader(vs);
        for (const auto &s : vs) {
          std::cout << s.symbol.path << " " << s.symbol.parent << " "
            << s.symbol.lineno << " " << lstrip(s.context) << "\n";
        }
        break;
      }

      case '7': { // find file
        std::vector<std::string> vs = db.find_files(line.substr(1));
        print_leader(vs);
        /* XXX: what kind of nonsense output is this? I don't know what value
         * Cscope is attempting to add with the trailing garbage.
         */
        for (const auto &s : vs)
          std::cout << s << " <unknown> 1 <unknown>\n";
        break;
      }

      case '8': { // find includers
        std::vector<clink::Result> vs = db.find_includers(line.substr(1));
        print_leader(vs);
        for (const auto &s : vs)
          std::cout << s.symbol.path << " " << s.symbol.parent << " "
            << s.symbol.lineno << " " << lstrip(s.context) << "\n";
        break;
      }

      // Commands we don't support. Just pretend there were no results.
      case '4': // find text
      case '5': // change text
      case '6': // find pattern
      case '9': // find assignments
        std::cout << "cscope: 0 lines\n";
        break;

      /* Bail out on any unrecognised command, under the assumption Vim would
       * never send us a malformed command.
       */
      default:
        return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
