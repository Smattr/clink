#include "../../common/compiler.h"
#include "build.h"
#include "help.h"
#include "line_ui.h"
#include "ncurses_ui.h"
#include "option.h"
#include "path.h"
#include "sigint.h"
#include <assert.h>
#include <clink/clink.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char *xstrdup(const char *s) {
  char *p = strdup(s);
  if (p == NULL) {
    fprintf(stderr, "out of memory\n");
    exit(EXIT_FAILURE);
  }
  return p;
}

static void xappend(char ***list, size_t *len, const char *item) {

  // expand the list
  *list = realloc(*list, (*len + 1) * sizeof(**list));
  if (*list == NULL) {
    fprintf(stderr, "out of memory\n");
    exit(EXIT_FAILURE);
  }
  ++(*len);

  // append the new item
  (*list)[*len - 1] = xstrdup(item);
}

static void parse_args(int argc, char **argv) {

  while (true) {
    enum {
      OPT_COLOUR = 128,
      OPT_COMPILE_COMMANDS,
      OPT_DEBUG,
      OPT_PARSE_ASM,
      OPT_PARSE_C,
      OPT_PARSE_CXX,
      OPT_PARSE_DEF,
    };

    static const struct option opts[] = {
        // clang-format off
        {"build-only",           no_argument,       0, 'b'},
        {"color",                required_argument, 0, OPT_COLOUR},
        {"colour",               required_argument, 0, OPT_COLOUR},
        {"compile-commands",     required_argument, 0, OPT_COMPILE_COMMANDS},
        {"compile-commands-dir", required_argument, 0, OPT_COMPILE_COMMANDS},
        {"database",             required_argument, 0, 'f'},
        {"debug",                no_argument,       0, OPT_DEBUG},
        {"help",                 no_argument,       0, 'h'},
        {"jobs",                 required_argument, 0, 'j'},
        {"line-oriented",        no_argument,       0, 'l'},
        {"no-build",             no_argument,       0, 'd'},
        {"parse-asm",            required_argument, 0, OPT_PARSE_ASM},
        {"parse-c",              required_argument, 0, OPT_PARSE_C},
        {"parse-cxx",            required_argument, 0, OPT_PARSE_CXX},
        {"parse-def",            required_argument, 0, OPT_PARSE_DEF},
        {"syntax-highlighting",  required_argument, 0, 's'},
        {"version",              no_argument,       0, 'V'},
        {0, 0, 0, 0},
        // clang-format on
    };

    int index = 0;
    int c = getopt_long(argc, argv, "bdf:hj:ls:V", opts, &index);

    if (c == -1)
      break;

    switch (c) {

    case 'b': // --build-only
      option.ncurses_ui = false;
      option.line_ui = false;
      break;

    case 'd': // --no-build
      option.update_database = false;
      break;

    case 'f': // --database
      free(option.database_path);
      option.database_path = xstrdup(optarg);
      break;

    case 'h': // --help
      help();
      exit(EXIT_SUCCESS);

    case 'j': // --jobs
      if (strcmp(optarg, "auto") == 0) {
        option.threads = 0;
      } else {
        char *endptr;
        option.threads = strtoul(optarg, &endptr, 0);
        if (optarg == endptr ||
            (option.threads == ULONG_MAX && errno == ERANGE)) {
          fprintf(stderr, "illegal value to --jobs: %s\n", optarg);
          exit(EXIT_FAILURE);
        }
      }
      break;

    case 'l':
      option.ncurses_ui = false;
      option.line_ui = true;
      break;

    case 's': // --syntax-highlighting
      if (strcmp(optarg, "auto") == 0) {
        option.highlighting = BEHAVIOUR_AUTO;
      } else if (strcmp(optarg, "eager") == 0) {
        option.highlighting = EAGER;
      } else if (strcmp(optarg, "lazy") == 0) {
        option.highlighting = LAZY;
      } else {
        fprintf(stderr, "illegal value to --syntax-highlighting: %s\n", optarg);
        exit(EXIT_FAILURE);
      }
      break;

    case OPT_COLOUR: // --colour
      if (strcmp(optarg, "auto") == 0) {
        option.colour = AUTO;
      } else if (strcmp(optarg, "always") == 0) {
        option.colour = ALWAYS;
      } else if (strcmp(optarg, "never") == 0) {
        option.colour = NEVER;
      } else {
        fprintf(stderr, "illegal value to --colour: %s\n", optarg);
        exit(EXIT_FAILURE);
      }
      break;

    case OPT_COMPILE_COMMANDS: { // --compile-commands
      if (option.compile_commands.db != NULL)
        compile_commands_close(&option.compile_commands);
      int rc = compile_commands_open(&option.compile_commands, optarg);
      if (rc != 0) {
        fprintf(stderr, "failed to open compile commands directory %s: %s\n",
                optarg, strerror(rc));
        exit(EXIT_FAILURE);
      }
      break;
    }

    case OPT_DEBUG: // --debug
      option.debug = true;
      clink_debug_on();
      break;

    case OPT_PARSE_ASM: // --parse-asm
      if (strcmp(optarg, "generic") == 0) {
        option.parse_asm = GENERIC;
      } else if (strcmp(optarg, "off") == 0) {
        option.parse_asm = OFF;
      } else {
        fprintf(stderr, "illegal value to --parse-asm: %s\n", optarg);
        exit(EXIT_FAILURE);
      }
      break;

    case OPT_PARSE_C: // --parse-c
      if (strcmp(optarg, "clang") == 0) {
        option.parse_c = CLANG;
      } else if (strcmp(optarg, "generic") == 0) {
        option.parse_c = GENERIC;
      } else if (strcmp(optarg, "off") == 0) {
        option.parse_c = OFF;
      } else {
        fprintf(stderr, "illegal value to --parse-c: %s\n", optarg);
        exit(EXIT_FAILURE);
      }
      break;

    case OPT_PARSE_CXX: // --parse-cxx
      if (strcmp(optarg, "clang") == 0) {
        option.parse_c = CLANG;
      } else if (strcmp(optarg, "generic") == 0) {
        option.parse_cxx = GENERIC;
      } else if (strcmp(optarg, "off") == 0) {
        option.parse_cxx = OFF;
      } else {
        fprintf(stderr, "illegal value to --parse-cxx: %s\n", optarg);
        exit(EXIT_FAILURE);
      }
      break;

    case OPT_PARSE_DEF: // --parse-def
      if (strcmp(optarg, "generic") == 0) {
        option.parse_def = GENERIC;
      } else if (strcmp(optarg, "off") == 0) {
        option.parse_def = OFF;
      } else {
        fprintf(stderr, "illegal value to --parse-def: %s\n", optarg);
        exit(EXIT_FAILURE);
      }
      break;

    case 'V': { // --version
      clink_version_info_t version = clink_version_info();
      fprintf(stderr, "clink version %s\n", version.version);
      fprintf(stderr, " assertions: %s\n",
              version.with_assertions ? "enabled" : "disabled");
      fprintf(stderr, " optimisations: %s\n",
              version.with_optimisations ? "enabled" : "disabled");
      fprintf(stderr, " using libclang %u.%u\n", version.libclang_major_version,
              version.libclang_minor_version);
      fprintf(stderr, " using LLVM %s\n", version.llvm_version);
      exit(EXIT_SUCCESS);
    }

    default:
      exit(EXIT_FAILURE);
    }
  }

  // interpret any remaining options as sources
  for (size_t i = optind; i < (size_t)argc; ++i)
    xappend(&option.src, &option.src_len, argv[i]);

  // if the user wanted automatic parallelism, give them a thread per core
  if (option.threads == 0) {
    long r = sysconf(_SC_NPROCESSORS_ONLN);
    if (r < 1) {
      option.threads = 1;
    } else {
      option.threads = (unsigned long)r;
    }
  }

  // if the user wanted auto colour, make a decision based on whether stdout is
  // a TTY
  if (option.colour == AUTO)
    option.colour = isatty(STDOUT_FILENO) ? ALWAYS : NEVER;
  assert(option.colour == ALWAYS || option.colour == NEVER);

  // at most one user interface should have been enabled
  assert(!option.ncurses_ui || !option.line_ui);
}

int main(int argc, char **argv) {

  // parse command line arguments
  parse_args(argc, argv);

  // figure out where to create (or re-open) .clink.db
  int rc = set_db_path();
  if (rc) {
    fprintf(stderr, "failed to configure path to database: %s\n", strerror(rc));
    goto done;
  }
  assert(option.database_path != NULL);

  // figure out what source paths we should scan
  if ((rc = set_src())) {
    fprintf(stderr, "failed to set source files/directories to scan: %s\n",
            strerror(rc));
    goto done;
  }
  assert(option.src != NULL && option.src_len > 0);

  // setup Clang command line
  if (option.update_database) {
    if (option.parse_c == CLANG || option.parse_cxx == CLANG) {
      rc = set_clang_flags();
      if (UNLIKELY(rc)) {
        fprintf(stderr, "failed to set Clang flags: %s\n", strerror(rc));
        goto done;
      }
    }
  }

  // setup out connection to compile_commands.json
  if (option.update_database) {
    if (option.parse_c == CLANG || option.parse_cxx == CLANG) {
      int r = set_compile_commands();
      // ignore failure here
      if (option.debug && r != 0)
        fprintf(stderr, "setting up compile commands failed: %s\n",
                strerror(r));
    }
  }

  if (option.update_database) {
    for (size_t i = 0; i < option.src_len; ++i) {

      // check the source path exists, to avoid later complications
      if (access(option.src[i], R_OK) < 0) {
        rc = errno;
        fprintf(stderr, "%s not accessible: %s\n", option.src[i], strerror(rc));
        goto done;
      }

      // make the path absolute to ease later work
      char *absolute = realpath(option.src[i], NULL);
      if (absolute == NULL) {
        rc = errno;
        fprintf(stderr, "failed to make %s absolute: %s\n", option.src[i],
                strerror(rc));
        goto done;
      }
      free(option.src[i]);
      option.src[i] = absolute;
    }
  }

  // ensure SQLite is safe to use multi-threaded
  if (option.threads > 1) {
    if (!sqlite3_threadsafe()) {
      fprintf(stderr, "your SQLite library does not support multi-threading\n");
      rc = -1;
      goto done;
    }
    int r = sqlite3_config(SQLITE_CONFIG_SERIALIZED);
    if (r != SQLITE_OK) {
      fprintf(stderr, "failed to set SQLite serialized mode: %s\n",
              sqlite3_errstr(r));
      rc = -1;
      goto done;
    }
  }

  // block SIGINT while we open (and possibly construct) the database, so we do
  // not end up corrupting the file if we are interrupted
  if ((rc = sigint_block())) {
    fprintf(stderr, "failed to block SIGINT: %s\n", strerror(rc));
    goto done;
  }

  // open the database
  clink_db_t *db = NULL;
  if ((rc = clink_db_open(&db, option.database_path))) {
    fprintf(stderr, "failed to open database: %s\n", strerror(rc));
    goto done;
  }

  // now that the database exists, we can use realpath() on its path to make
  // sure it is absolute to simplify some later operation
  {
    char *a = realpath(option.database_path, NULL);
    if (a == NULL) {
      rc = errno;
      fprintf(stderr, "failed to make %s absolute: %s\n", option.database_path,
              strerror(rc));
      goto done1;
    }
    free(option.database_path);
    option.database_path = a;
  }

  // we can now be safely interrupted
  (void)sigint_unblock();

  // build/update the database, if requested
  if (option.update_database) {
    if ((rc = build(db)))
      goto done1;
  }

  // run line-oriented interface, if requested
  if (option.line_ui) {
    if ((rc = line_ui(db)))
      goto done1;
  }

  // Ncurses interface, if requested
  if (option.ncurses_ui) {
    if ((rc = ncurses_ui(db)))
      goto done1;
  }

done1:
  clink_db_close(&db);
done:
  clean_up_options();

  return rc ? EXIT_FAILURE : EXIT_SUCCESS;
}
