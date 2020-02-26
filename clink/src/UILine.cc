#include <clink/clink.h>
#include <cstdlib>
#include "Database.h"
#include <iostream>
#include <readline/readline.h>
#include "Symbol.h"
#include "UILine.h"
#include <unistd.h>
#include "util.h"
#include <vector>

using namespace std;

template<typename T>
static void print_leader(const vector<T> &vs) {
  cout << "cscope: " << vs.size() << " lines\n";
}

int UILine::run(Database &db) {

  int ret = EXIT_SUCCESS;

  char *line;
  while ((line = readline(">> "))) {

    // Skip leading white space
    char *command = line;
    while (isspace(*command))
      command++;

    // Ignore blank lines
    if (strcmp(command, "") == 0) {
      free(line);
      continue;
    }

    switch (*command) {

      case '0': { // find symbol
        vector<clink::Result> vs = db.find_symbol(command + 1);
        print_leader(vs);
        for (const auto &s : vs) {
          cout << s.symbol.path << " " << s.symbol.parent << " "
            << s.symbol.lineno << " " << lstrip(s.context);
        }
        break;
      }

      case '1': { // find definition
        vector<clink::Result> vs = db.find_definition(command + 1);
        print_leader(vs);
        for (const auto &s : vs) {
          cout << s.symbol.path << " " << (command + 1) << " "
            << s.symbol.lineno << " " << lstrip(s.context);
        }
        break;
      }

      case '2': { // find calls
        vector<clink::Result> vs = db.find_call(command + 1);
        print_leader(vs);
        for (const auto &s : vs) {
          cout << s.symbol.path << " " << s.symbol.name << " "
            << s.symbol.lineno << " " << lstrip(s.context);
        }
        break;
      }

      case '3': { // find callers
        vector<clink::Result> vs = db.find_caller(command + 1);
        print_leader(vs);
        for (const auto &s : vs) {
          cout << s.symbol.path << " " << s.symbol.parent << " "
            << s.symbol.lineno << " " << lstrip(s.context);
        }
        break;
      }

      case '7': { // find file
        vector<string> vs = db.find_file(command + 1);
        print_leader(vs);
        /* XXX: what kind of nonsense output is this? I don't know what value
         * Cscope is attempting to add with the trailing garbage.
         */
        for (const auto &s : vs)
          cout << s << " <unknown> 1 <unknown>\n";
        break;
      }

      case '8': { // find includers
        vector<clink::Result> vs = db.find_includer(command + 1);
        print_leader(vs);
        for (const auto &s : vs)
          cout << s.symbol.path << " " << s.symbol.parent << " "
            << s.symbol.lineno << " " << lstrip(s.context);
        break;
      }

      // Commands we don't support. Just pretend there were no results.
      case '4': // find text
      case '5': // change text
      case '6': // find pattern
      case '9': // find assignments
        cout << "cscope: 0 lines\n";
        break;

      /* Bail out on any unrecognised command, under the assumption Vim would
       * never send us a malformed command.
       */
      default:
        free(line);
        ret = EXIT_FAILURE;
        goto break2;

    }

    free(line);
  }

break2:

  return ret;
}
