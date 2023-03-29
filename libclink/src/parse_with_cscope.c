#include "add_symbol.h"
#include "debug.h"
#include "get_environ.h"
#include "mmap.h"
#include "posix_spawn.h"
#include "scanner.h"
#include "span.h"
#include <assert.h>
#include <clink/cscope.h>
#include <clink/db.h>
#include <clink/symbol.h>
#include <errno.h>
#include <fcntl.h>
#include <spawn.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

/** run \p cscope, parsing the given file
 *
 * \param filename Path to source file to parse
 * \param cscope_out Path to write Cscope database to
 * \return 0 on success or an errno on failure
 */
static int run_cscope(const char *filename, const char *cscope_out) {
  assert(filename != NULL);
  assert(cscope_out != NULL);
  assert(access(cscope_out, F_OK) != 0);
  assert(clink_have_cscope());

  int rc = 0;
  posix_spawn_file_actions_t fa;
  int devnull = -1;
  char *dash_f = NULL;

  if (ERROR((rc = fa_init(&fa))))
    return rc;

  // hide all of Cscope’s stdin/stdout/stderr
  devnull = open("/dev/null", O_RDWR);
  if (ERROR(devnull < 0)) {
    rc = errno;
    goto done;
  }
  if (ERROR((rc = adddup2(&fa, devnull, STDIN_FILENO))))
    goto done;
  if (ERROR((rc = adddup2(&fa, devnull, STDOUT_FILENO))))
    goto done;
  if (ERROR((rc = adddup2(&fa, devnull, STDERR_FILENO))))
    goto done;

  // construct a command to run Cscope
  if (ERROR(asprintf(&dash_f, "-f%s", cscope_out) < 0)) {
    rc = ENOMEM;
    goto done;
  }
  const char *argv[] = {"cscope", "-b", "-c", dash_f, "--", filename, NULL};

  // run Cscope
  {
    pid_t cscope;
    if (ERROR((rc = spawn(&cscope, argv, &fa))))
      goto done;

    int status;
    do {
      if (ERROR(waitpid(cscope, &status, 0) < 0)) {
        rc = errno;
        goto done;
      }
    } while (WIFSTOPPED(status) || WIFCONTINUED(status));
    if (ERROR(!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS)) {
      rc = ENOTRECOVERABLE;
      goto done;
    }
  }

done:
  free(dash_f);
  if (devnull >= 0)
    (void)close(devnull);
  fa_destroy(&fa);

  return rc;
}

/// parse the header of a Cscope database
static int eat_header(scanner_t *s) {
  assert(s != NULL);

  if (ERROR(!eat_if(s, "cscope ")))
    return EPROTO;

  if (ERROR(!eat_if(s, "15 ")))
    return EPROTONOSUPPORT;

  // swallow the “current dir” part of the header
  if (ERROR(!eat_non_ws(s)))
    return EPROTO;
  eat_ws(s);

  // we should have generated an uncompressed database
  if (ERROR(!eat_if(s, "-c ")))
    return EPROTONOSUPPORT;
  eat_ws(s);

  // we should not have generated an inverted index
  if (ERROR(eat_if(s, "-q")))
    return EPROTONOSUPPORT;

  // we should not have used abbreviated search
  if (ERROR(eat_if(s, "-T")))
    return EPROTONOSUPPORT;

  // swallow the trailer offset field
  if (ERROR(!eat_num(s, NULL)))
    return EPROTO;
  if (ERROR(!eat_eol(s)))
    return EPROTO;

  return 0;
}

static span_t read_symbol(scanner_t *s) {
  assert(s != NULL);

  span_t symbol = {.base = &s->base[s->offset]};
  while (s->offset < s->size && !eat_eol(s)) {
    eat_one(s);
    ++symbol.size;
  }

  return symbol;
}

static int parse_into(clink_db_t *db, const char *cscope_out,
                      const char *filename) {
  assert(db != NULL);
  assert(cscope_out != NULL);
  assert(access(cscope_out, R_OK) == 0);
  assert(filename != NULL);

  int rc = 0;

  mmap_t f = {0};
  if (ERROR((rc = mmap_open(&f, cscope_out))))
    goto done;

  scanner_t s = scanner(f.base, f.size);

  if (ERROR((rc = eat_header(&s))))
    goto done;

  // are we within the section of the Cscope database relating to the target
  // file?
  bool in_file = false;

  // last line marker we saw
  size_t line = 0;

  // function/macro/struct we are within
  span_t parent = {0};

  // symbols queued to be inserted
  enum { SYMBOL_WINDOW = 100 };
  symbol_t pending[SYMBOL_WINDOW];
  size_t pending_size = 0;

  while (s.offset < s.size) {

    clink_category_t category = CLINK_REFERENCE;
    bool can_be_parent = false;
    {
      char mark = 0;
      // do we have a recognised category for this symbol?
      if (eat_mark(&s, &mark)) {
        switch (mark) {
        case '@': { // entering a new file
          span_t path = read_symbol(&s);
          DEBUG("%s:%lu: saw filename \"%.*s\"", cscope_out, s.lineno - 1,
                (int)path.size, path.base);
          // if we saw an empty file mark, we are done
          if (span_eq(path, ""))
            goto done;
          in_file = span_eq(path, filename); // TODO $HOME handling
          // discard the blank line following this
          if (ERROR(!eat_eol(&s))) {
            rc = EPROTO;
            goto done;
          }
          continue;
        }

        case '$': // function definition
        case '#': // #define
        case 'c': // class definition
        case 'e': // enum definition
        case 's': // struct definition
        case 'u': // union definition
          can_be_parent = true;
          // fall through
        case 'g': // other global definition
        case 'l': // function/block local definition
        case 'm': // global enum/struct/union member definition
        case 'p': // function parameter definition
        case 't': // typedef definition
          category = CLINK_DEFINITION;
          break;

        case '`': // function call
          category = CLINK_FUNCTION_CALL;
          break;

        case '~': // #include
          category = CLINK_INCLUDE;
          break;

        case '}': // function end
        case ')': // define end
        case '=': // direct assignment, increment, or decrement
        case ';': // enum/struct/union definition end
          parent = (span_t){0};
          if (ERROR(!eat_eol(&s))) {
            rc = EPROTO;
            goto done;
          }
          continue;
        }

      } else if (eat_num(&s, &line)) { // is this a new line?
        // drain the leading text
        eat_rest_of_line(&s);
        continue;
      } else if (eat_eol(&s)) { // is this the end of this line?
        continue;
      } else { // otherwise we have a reference
        // do nothing
      }
    }

    // read the symbol itself
    span_t symbol = read_symbol(&s);
    if (ERROR(symbol.size == 0)) {
      rc = EPROTO;
      goto done;
    }
    symbol.lineno = line;
    symbol.colno = 1; // no column information in Cscope databases

    if (in_file) {
      DEBUG("adding symbol \"%.*s\"", (int)symbol.size, symbol.base);
      symbol_t sym = {.category = category,
                      .name = symbol,
                      .path = filename,
                      .parent = parent};
      if (pending_size == sizeof(pending) / sizeof(pending[0])) {
        // flush the pending symbols
        if (ERROR((rc = add_symbols(db, pending_size, pending))))
          goto done;
        pending_size = 0;
      }
      // enqueue the current symbol
      pending[pending_size] = sym;
      ++pending_size;
    }

    if (can_be_parent)
      parent = symbol;

    // drain the next context line following
    eat_rest_of_line(&s);
  }

done:
  // flush any remaining pending symbols
  if (rc == 0 && pending_size > 0)
    rc = add_symbols(db, pending_size, pending);

  mmap_close(f);

  return rc;
}

int clink_parse_with_cscope(clink_db_t *db, const char *filename) {

  if (ERROR(db == NULL))
    return EINVAL;

  if (ERROR(filename == NULL))
    return EINVAL;

  // check the file is readable
  if (ERROR(access(filename, R_OK) < 0))
    return errno;

  // fail if we do not have Cscope
  if (!clink_have_cscope())
    return ENOENT;

  int rc = 0;
  char *dir = NULL;
  char *cscope_out = NULL;

  // create a temporary directory into which to write the Cscope database
  const char *TMPDIR = getenv("TMPDIR");
  if (TMPDIR == NULL)
    TMPDIR = "/tmp";
  if (ERROR(asprintf(&dir, "%s/tmp.XXXXXX", TMPDIR) < 0)) {
    rc = ENOMEM;
    goto done;
  }
  if (ERROR(mkdtemp(dir) == NULL)) {
    rc = errno;
    goto done;
  }

  // construct a path within this directory that we know will not exist
  if (ERROR(asprintf(&cscope_out, "%s/cscope.out", dir) < 0)) {
    rc = ENOMEM;
    goto done;
  }

  // parse the given file with Cscope
  if (ERROR((rc = run_cscope(filename, cscope_out))))
    goto done;

  // translate Cscope’s output into insertion commands into the Clink database
  if (ERROR((rc = parse_into(db, cscope_out, filename))))
    goto done;

done:
  if (cscope_out != NULL)
    (void)unlink(cscope_out);
  free(cscope_out);
  if (dir != NULL)
    (void)rmdir(dir);
  free(dir);

  return rc;
}
