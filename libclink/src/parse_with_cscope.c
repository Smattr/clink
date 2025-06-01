#include "../../common/ctype.h"
#include "add_symbol.h"
#include "arena.h"
#include "debug.h"
#include "get_environ.h"
#include "get_id.h"
#include "isid.h"
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
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/** run `cscope`, parsing the given file
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
  char *dash_f = NULL;

  if (ERROR((rc = fa_init(&fa))))
    return rc;

  // Hide Cscope’s stdin/stdout. Its stderr may be captured by our caller.
  if (ERROR((rc = addopen(&fa, STDIN_FILENO, "/dev/null", O_RDONLY))))
    goto done;
  if (ERROR((rc = addopen(&fa, STDOUT_FILENO, "/dev/null", O_WRONLY))))
    goto done;

  // construct a command to run Cscope
  if (ERROR(asprintf(&dash_f, "-f%s", cscope_out) < 0)) {
    rc = ENOMEM;
    goto done;
  }
  const char *argv[] = {"cscope", "-b", dash_f, "--", filename, NULL};

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

  // we should have generated a compressed database
  if (ERROR(eat_if(s, "-c ")))
    return EPROTONOSUPPORT;

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

/// is this a Cscope “dicode”?
///
/// Cscope’s compression format has the concept of “dicodes,” single bytes that
/// represent two ASCII characters.
///
/// \param byte Character to evaluate
/// \return True if this is a dicode
static bool is_dicode(char byte) { return ((unsigned char)byte >> 7) != 0; }

/// lookup table for the first half of a dicode (see `decompress`)
static const char DICODE_LUT1[] = {' ', 't', 'e', 'i', 's', 'a', 'p', 'r',
                                   'n', 'l', '(', 'o', 'f', ')', '=', 'c'};

/// lookup table for the second half of a dicode (see `decompress`)
static const char DICODE_LUT2[] = {' ', 't', 'n', 'e', 'r', 'p', 'l', 'a'};

/// expand Cscope-derived data to its uncompressed form
///
/// Cscope’s compressed format (the default it writes; when `-c` is not given)
/// uses two techniques, (1) compressing two ASCII characters into one and (2)
/// a lookup table of common keywords. This function implements decompression
/// with respect to (1). It is assumed that we are only ever decompressing
/// non-keywords and thus do not need to handle (2).
///
/// \param text Potentially compressed text
/// \param arena An allocator to use for new text
/// \return 0 on success or an errno on failure
static int decompress(span_t *text, arena_t *arena) {
  assert(text != NULL);
  assert(text->base != NULL || text->size == 0);
  assert(arena != NULL);

  // how many dicodes do we need to expand?
  size_t dicode_count = 0;
  for (size_t i = 0; i < text->size; ++i)
    dicode_count += is_dicode(text->base[i]);

  if (dicode_count == 0)
    return 0;

  // create backing space to decompress into
  char *decompressed = arena_alloc(arena, text->size + dicode_count);
  if (ERROR(decompressed == NULL))
    return ENOMEM;

  for (size_t src = 0, dst = 0; src < text->size; ++src, ++dst) {
    if (is_dicode(text->base[src])) {
      // decode the first half of the dicode
      {
        const size_t dicode1 = ((unsigned char)text->base[src] - 128) / 8;
        assert(dicode1 < sizeof(DICODE_LUT1));
        decompressed[dst] = DICODE_LUT1[dicode1];
        ++dst;
      }

      // decode the second half of the dicode
      {
        const size_t dicode2 = ((unsigned char)text->base[src] - 128) % 8;
        assert(dicode2 < sizeof(DICODE_LUT2));
        decompressed[dst] = DICODE_LUT2[dicode2];
      }

      continue;
    }
    decompressed[dst] = text->base[src];
  }

  // update span to point at the decompressed data
  text->base = decompressed;
  text->size += dicode_count;

  return 0;
}

/// swallow an assignment operator if possible
static bool eat_assign(scanner_t *s) {
  assert(s != NULL);

  // Swallow non-line-ending white space. We do not need to handle the double
  // space dicode (0b10000000) because Cscope coalesces adjacent spaces and
  // never produces this.
  while (s->offset < s->size) {
    const char c = s->base[s->offset];
    assert((unsigned char)c != 128 && "Cscope produced double-space dicode");
    if (c == '\n' || c == '\r' || !isspace_(c))
      break;
    eat_one(s);
  }

  if (s->offset == s->size)
    return false;

  const char eq1 = s->base[s->offset];

  // is this a dicode beginning with '='?
  if (is_dicode(eq1)) {
    const size_t dicode1 = ((unsigned char)eq1 - 128) / 8;
    assert(dicode1 < sizeof(DICODE_LUT1));
    assert(memchr(DICODE_LUT2, '=', sizeof(DICODE_LUT2)) == NULL);
    return DICODE_LUT1[dicode1] == '=';
  }

  const char eq2 = s->offset + 1 < s->size ? s->base[s->offset + 1] : 0;
  const bool eq2_is_eq =
      eq2 == '=' ||
      (is_dicode(eq2) && DICODE_LUT1[((unsigned char)eq2 - 128) / 8] == '=');
  const char eq3 = s->offset + 2 < s->size ? s->base[s->offset + 2] : 0;
  const bool eq3_is_eq =
      eq3 == '=' ||
      (is_dicode(eq3) && DICODE_LUT1[((unsigned char)eq3 - 128) / 8] == '=');

  // '=', but not '=='?
  if (eq1 == '=')
    return !eq2_is_eq;

  // compound assignment?
  {
    assert(memchr(DICODE_LUT1, '+', sizeof(DICODE_LUT1)) == NULL);
    assert(memchr(DICODE_LUT1, '-', sizeof(DICODE_LUT1)) == NULL);
    assert(memchr(DICODE_LUT1, '*', sizeof(DICODE_LUT1)) == NULL);
    assert(memchr(DICODE_LUT1, '/', sizeof(DICODE_LUT1)) == NULL);
    assert(memchr(DICODE_LUT1, '%', sizeof(DICODE_LUT1)) == NULL);
    assert(memchr(DICODE_LUT1, '&', sizeof(DICODE_LUT1)) == NULL);
    assert(memchr(DICODE_LUT1, '|', sizeof(DICODE_LUT1)) == NULL);
    assert(memchr(DICODE_LUT1, '^', sizeof(DICODE_LUT1)) == NULL);
    assert(memchr(DICODE_LUT1, '<', sizeof(DICODE_LUT1)) == NULL);
    assert(memchr(DICODE_LUT2, '<', sizeof(DICODE_LUT2)) == NULL);
    assert(memchr(DICODE_LUT1, '>', sizeof(DICODE_LUT1)) == NULL);
    assert(memchr(DICODE_LUT2, '>', sizeof(DICODE_LUT2)) == NULL);
    const bool is_compound =
        ((eq1 == '+' || eq1 == '-' || eq1 == '*' || eq1 == '/' || eq1 == '%' ||
          eq1 == '&' || eq1 == '|' || eq1 == '^') &&
         eq2_is_eq) ||
        (eq1 == '<' && eq2 == '<' && eq3_is_eq) ||
        (eq1 == '>' && eq2 == '>' && eq3_is_eq);
    if (is_compound)
      return true;
  }

  // post-increment/decrement
  assert(memchr(DICODE_LUT1, '+', sizeof(DICODE_LUT1)) == NULL);
  assert(memchr(DICODE_LUT2, '+', sizeof(DICODE_LUT2)) == NULL);
  if (eq1 == '+' && eq2 == '+')
    return true;
  assert(memchr(DICODE_LUT1, '-', sizeof(DICODE_LUT1)) == NULL);
  assert(memchr(DICODE_LUT2, '-', sizeof(DICODE_LUT2)) == NULL);
  if (eq1 == '-' && eq2 == '-')
    return true;

  // TODO: handle pre-increment/decrement?

  return false;
}

/// is this a valid C/C++ symbol?
static bool is_symbol(span_t text) {
  bool valid = text.size > 0 && isid0(text.base[0]);
  for (size_t i = 1; i < text.size; ++i)
    valid &= isid(text.base[i]);
  return valid;
}

/// parse a Cscope database and insert records into a Clink database
///
/// \param db Destination Clink database to insert into
/// \param cscope_out Source Cscope database to parse from
/// \param real The fully resolved path of the source file that Cscope parsed
/// \param filename The original path to the source file
/// \param id Optional known Clink database identifier of the source file
/// \return 0 on success or an errno on failure
static int parse_into(clink_db_t *db, const char *cscope_out, const char *real,
                      const char *filename, clink_record_id_t id) {
  assert(db != NULL);
  assert(cscope_out != NULL);
  assert(access(cscope_out, R_OK) == 0);
  assert(real != NULL);
  assert(filename != NULL);

  int rc = 0;
  mmap_t f = {0};

  // symbols queued to be inserted
  enum { SYMBOL_WINDOW = 1000 };
  symbol_t pending[SYMBOL_WINDOW];
  size_t pending_size = 0;

  // scratch space for decompressed symbols
  arena_t arena = {0};

  // if the caller did not give us an identifier, look it up now
  if (id < 0) {
    if (ERROR((rc = get_id(db, filename, &id))))
      goto done;
  }

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

  while (s.offset < s.size) {

    clink_category_t category = CLINK_REFERENCE;
    bool can_be_parent = false;
    bool can_be_assignment = true;
    {
      char mark = 0;
      // do we have a recognised category for this symbol?
      if (eat_mark(&s, &mark)) {
        switch (mark) {
        case '@': { // entering a new file
          span_t path = read_symbol(&s);
          if (ERROR((rc = decompress(&path, &arena))))
            goto done;
          DEBUG("%s:%lu: saw filename \"%.*s\"", cscope_out, s.lineno - 1,
                (int)path.size, path.base);
          // if we saw an empty file mark, we are done
          if (span_eq(path, ""))
            goto done;
          in_file = span_eq(path, real); // TODO $HOME handling
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
        case 'm': // global enum/struct/union member definition
        case 'p': // function parameter definition
        case 't': // typedef definition
          can_be_assignment = false;
          // fall through
        case 'g': // other global definition
        case 'l': // function/block local definition
          category = CLINK_DEFINITION;
          break;

        case '`': // function call
          category = CLINK_FUNCTION_CALL;
          can_be_assignment = false;
          break;

        case '~': // #include
          category = CLINK_INCLUDE;
          can_be_assignment = false;
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
    if (ERROR((rc = decompress(&symbol, &arena))))
      goto done;
    if (category != CLINK_INCLUDE && !is_symbol(symbol)) {
      // Parsing code that has differing semantics under C and C++ (e.g.
      // `enum class {}`) can cause Cscope’s database to contain references to
      // things with no name. Alternatively non-definition content can be just
      // punctuation/operators. Ignore these.
      DEBUG("ignoring symbol \"%.*s\"", (int)symbol.size, symbol.base);
      continue;
    }
    symbol.lineno = line;
    symbol.colno = 1; // no column information in Cscope databases

    if (in_file) {
      DEBUG("adding symbol \"%.*s\"", (int)symbol.size, symbol.base);
      symbol_t sym = {.category = category, .name = symbol, .parent = parent};
      if (pending_size == sizeof(pending) / sizeof(pending[0])) {
        // flush the pending symbols
        if (ERROR((rc = add_symbols(db, pending_size, pending, id))))
          goto done;
        pending_size = 0;
      }
      // enqueue the current symbol
      pending[pending_size] = sym;
      ++pending_size;
    }

    if (in_file && can_be_assignment) {
      if (eat_assign(&s)) {
        DEBUG("adding assignment to symbol \"%.*s\"", (int)symbol.size,
              symbol.base);
        symbol_t sym = {
            .category = CLINK_ASSIGNMENT, .name = symbol, .parent = parent};
        if (pending_size == sizeof(pending) / sizeof(pending[0])) {
          // flush the pending symbols
          if (ERROR((rc = add_symbols(db, pending_size, pending, id))))
            goto done;
          pending_size = 0;
        }
        // enqueue the current symbol
        pending[pending_size] = sym;
        ++pending_size;
      }
    }

    if (can_be_parent)
      parent = symbol;
  }

done:
  // flush any remaining pending symbols
  if (rc == 0 && pending_size > 0)
    rc = add_symbols(db, pending_size, pending, id);
  arena_reset(&arena);

  mmap_close(f);

  return rc;
}

int clink_parse_with_cscope(clink_db_t *db, const char *filename,
                            clink_record_id_t id) {

  if (ERROR(db == NULL))
    return EINVAL;

  if (ERROR(filename == NULL))
    return EINVAL;

  if (ERROR(filename[0] != '/'))
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
  char *real = NULL; // resolved path of symlink, if filename is one

  // resolve the path in case it is a symlink
  real = realpath(filename, NULL);
  if (ERROR(real == NULL)) {
    rc = errno;
    goto done;
  }

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
  if (ERROR((rc = run_cscope(real, cscope_out))))
    goto done;

  // translate Cscope’s output into insertion commands into the Clink database
  if (ERROR((rc = parse_into(db, cscope_out, real, filename, id))))
    goto done;

done:
  if (cscope_out != NULL)
    (void)unlink(cscope_out);
  free(cscope_out);
  if (dir != NULL)
    (void)rmdir(dir);
  free(dir);
  free(real);

  return rc;
}
