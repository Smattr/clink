#include "build.h"
#include "../../common/compiler.h"
#include "compile_commands.h"
#include "file_queue.h"
#include "option.h"
#include "path.h"
#include "progress.h"
#include "sigint.h"
#include <assert.h>
#include <clink/clink.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/// Debug printf. This is implemented as a macro to avoid expensive varargs
/// handling when we are not in debug mode.
#define DEBUG(args...)                                                         \
  do {                                                                         \
    if (UNLIKELY(option.debug)) {                                              \
      progress_status(thread_id, args);                                        \
    }                                                                          \
  } while (0)

/// saved current working directory
static char *cur_dir;

/// use a compilation database to parse the given source with libclang
static int parse_with_comp_db(clink_db_t *db, const char *path) {

  assert(db != NULL);
  assert(path != NULL);

  size_t argc = 0;
  char **argv = NULL;
  const char **av = NULL;
  int rc = 0;

  // get the compilation command for this source
  rc = compile_commands_find(&option.compile_commands, path, &argc, &argv);
  if (UNLIKELY(rc != 0))
    goto done;
  assert(argc > 0);

  // create space to hold an argv[0] and the union of both flags
  size_t ac = argc + option.clang_argc - 1;
  assert(ac > 0);
  av = calloc(ac, sizeof(av[0]));
  if (UNLIKELY(av == NULL)) {
    rc = ENOMEM;
    goto done;
  }

  // insert the system includes prior to the rest of the options
  // because, unlike the compiler, libclang does not know these itself
  for (size_t i = 0; i < ac; ++i) {
    if (i == 0) {
      av[i] = argv[0];
    } else if (i < option.clang_argc) {
      av[i] = option.clang_argv[i];
    } else {
      av[i] = argv[i - option.clang_argc + 1];
    }
  }

  rc = clink_parse_with_clang(db, path, ac, av);
  if (rc != 0)
    goto done;

done:
  free(av);
  for (size_t i = 0; i < argc; ++i)
    free(argv[i]);
  free(argv);

  return rc;
}

static bool use_clang(const char *path) {
  if (is_c(path) && option.parse_c == CLANG)
    return true;
  if (is_cxx(path) && option.parse_cxx == CLANG)
    return true;
  return false;
}

static bool use_cscope(const char *path) {
  if (is_c(path) && option.parse_c == CSCOPE)
    return true;
  if (is_cxx(path) && option.parse_cxx == CSCOPE)
    return true;
  if (is_lex(path) && option.parse_lex == CSCOPE)
    return true;
  if (is_yacc(path) && option.parse_yacc == CSCOPE)
    return true;
  return false;
}

static const char *filetype(const char *path) {
  assert(is_c(path) || is_cxx(path) || is_lex(path) || is_yacc(path));
  if (is_c(path))
    return "C";
  if (is_cxx(path))
    return "C++";
  if (is_lex(path))
    return "Lex";
  return "Yacc";
}

static int parse(unsigned long thread_id, clink_db_t *db, const char *path,
                 clink_record_id_t id) {

  assert(id >= 0);

  int rc = 0;

  // generate a friendlier name for the source path
  const char *display = disppath(cur_dir, path);

  if (use_clang(path)) {
    progress_status(thread_id, "Clang-parsing %s file %s", filetype(path),
                    display);

    do {
      // if we have a compile commands database, use it
      if (option.compile_commands.db != NULL) {
        rc = parse_with_comp_db(db, path);
        if (rc != ENOMSG)
          break;

        progress_warn(thread_id, "no compile_commands.json entry found for %s",
                      path);
      }

      // if we do not have a compile commands database or no entry was found
      // for this path, parse with our default arguments
      assert(option.clang_argc > 0 && option.clang_argv != NULL);
      const char **argv = (const char **)option.clang_argv;
      rc = clink_parse_with_clang(db, path, option.clang_argc, argv);
    } while (0);

    // parse with the preprocessor
    if (rc == 0)
      rc = clink_parse_cpp(db, path);

  } else if (use_cscope(path)) {
    progress_status(thread_id, "Cscope-parsing %s file %s", filetype(path),
                    display);
    rc = clink_parse_with_cscope(db, path, id);

  } else if (is_asm(path)) {

    progress_status(thread_id, "parsing asm file %s", display);
    rc = clink_parse_asm(db, path);

    // C++ with generic parser
  } else if (is_cxx(path) && option.parse_cxx == GENERIC) {
    progress_status(thread_id, "generic-parsing C++ file %s", display);
    rc = clink_parse_cxx(db, path);

    // C with generic parser
  } else if (is_c(path) && option.parse_c == GENERIC) {
    progress_status(thread_id, "generic-parsing C file %s", display);
    rc = clink_parse_c(db, path);

    // DEF
  } else if (is_def(path)) {
    progress_status(thread_id, "parsing DEF file %s", display);
    rc = clink_parse_def(db, path);

    // Lex/Flex
  } else if (is_lex(path)) {
    progress_status(thread_id, "generic parsing Lex file %s", display);
    rc = clink_parse_cxx(db, path); // parse as C++

    // Python
  } else if (is_python(path)) {
    progress_status(thread_id, "parsing Python file %s", display);
    rc = clink_parse_python(db, path);

    // TableGen
  } else if (is_tablegen(path)) {
    progress_status(thread_id, "parsing TableGen file %s", display);
    rc = clink_parse_tablegen(db, path);

    // Yacc/Bison
  } else {
    assert(is_yacc(path));
    progress_status(thread_id, "generic parsing Yacc file %s", display);
    rc = clink_parse_cxx(db, path); // parse as C++
  }

  if (rc != 0) {
    progress_error(thread_id, "failed to parse %s: %s", display, strerror(rc));
    goto done;
  }

  if (option.highlighting == EAGER) {
    progress_status(thread_id, "syntax highlighting %s", display);

    if (UNLIKELY((rc = clink_vim_read_into(db, path)))) {

      // If the user hit Ctrl+C, Vim may have been SIGINTed causing it to
      // fail cryptically. If it looks like this happened, give the user a
      // less confusing message.
      if (sigint_pending()) {
        progress_error(thread_id, "failed to read %s: received SIGINT",
                       display);

      } else {
        progress_error(thread_id, "failed to read %s: %s", display,
                       strerror(rc));
      }
      goto done;
    }
  }

done:
  return rc;
}

/// drain a work queue, processing its entries into the database
static int process(unsigned long thread_id, pthread_t *threads, clink_db_t *db,
                   file_queue_t *q) {

  assert(db != NULL);
  assert(q != NULL);

  int rc = 0;

  while (true) {

    // get an item from the work queue
    const char *path = NULL;
    rc = file_queue_pop(q, &path);

    // if we have exhausted the work queue, we are done
    if (rc == ENOMSG) {
      progress_status(thread_id, " ");
      rc = 0;
      break;
    }

    if (UNLIKELY(rc)) {
      progress_error(thread_id, "failed to pop work queue: %s", strerror(rc));
      break;
    }

    assert(path != NULL);

    // see if we know of this file
    uint64_t hash = 0;
    uint64_t timestamp = 0;
    {
      bool has_record = clink_db_find_record(db, path, &hash, &timestamp) == 0;
      // stat the file to see if it has changed
      struct stat st;
      bool has_file = stat(path, &st) == 0;
      if (has_record && has_file) {
        // if it has not changed since last update, skip it
        if (hash == (uint64_t)st.st_size) {
          if (timestamp == (uint64_t)st.st_mtime) {
            DEBUG("skipping unmodified file %s", path);
            progress_increment();
            continue;
          }
        }
      }
      if (has_file) {
        hash = (uint64_t)st.st_size;
        timestamp = (uint64_t)st.st_mtime;
      }
    }

    // remove anything related to the file we are about to parse
    clink_db_remove(db, path);

    // insert a new record for the file
    clink_record_id_t id = -1;
    (void)clink_db_add_record(db, path, hash, timestamp, &id);

    if (UNLIKELY((rc = parse(thread_id, db, path, id))))
      break;

    // bump the progress counter
    progress_increment();

    // check if we have been SIGINTed and should finish up
    if (UNLIKELY(sigint_pending())) {
      progress_status(thread_id, "saw SIGINT; exiting...");
      break;
    }
  }

  // Signals are delivered to one arbitrary thread in a multithreaded process.
  // So if we saw a SIGINT, signal the thread before us so that it cascades and
  // is eventually propagated to all threads.
  if (UNLIKELY(sigint_pending())) {
    unsigned long previous = (thread_id == 0 ? option.threads : thread_id) - 1;
    if (previous != thread_id)
      (void)pthread_kill(threads[previous], SIGINT);
  }

  return rc;
}

// a vehicle for passing data to process()
typedef struct {
  unsigned long thread_id;
  pthread_t *threads;
  clink_db_t *db;
  file_queue_t *q;
} process_args_t;

// trampoline for unpacking the calling convention used by pthreads
static void *process_entry(void *args) {

  // unpack our arguments
  const process_args_t *a = args;
  unsigned long thread_id = a->thread_id;
  pthread_t *threads = a->threads;
  clink_db_t *db = a->db;
  file_queue_t *q = a->q;

  int rc = process(thread_id, threads, db, q);

  return (void *)(intptr_t)rc;
}

// call process() multi-threaded
static int mt_process(clink_db_t *db, file_queue_t *q) {

  // the total threads is ourselves plus all the others
  assert(option.threads >= 1);
  size_t bg_threads = option.threads - 1;

  // create threads
  pthread_t *threads = calloc(option.threads, sizeof(threads[0]));
  if (UNLIKELY(threads == NULL))
    return ENOMEM;

  // create state for them
  process_args_t *args = calloc(bg_threads, sizeof(args[0]));
  if (UNLIKELY(args == NULL)) {
    free(threads);
    return ENOMEM;
  }

  // set up data for all threads
  for (size_t i = 1; i < option.threads; ++i)
    args[i - 1] =
        (process_args_t){.thread_id = i, .threads = threads, .db = db, .q = q};

  // start all threads
  size_t started = 0;
  for (size_t i = 0; i < option.threads; ++i) {
    if (i == 0) {
      threads[i] = pthread_self();
    } else {
      if (pthread_create(&threads[i], NULL, process_entry, &args[i - 1]) != 0)
        break;
    }
    started = i + 1;
  }

  // join in helping with the rest
  int rc = process(0, threads, db, q);

  // collect other threads
  for (size_t i = 0; i < started; ++i) {

    // skip ourselves
    if (i == 0)
      continue;

    void *ret;
    int r = pthread_join(threads[i], &ret);

    // none of the pthread failure reasons should be possible
    assert(r == 0);
    (void)r;

    if (ret != NULL && rc == 0)
      rc = (int)(intptr_t)ret;
  }

  // clean up memory
  free(args);
  free(threads);

  return rc;
}

int build(clink_db_t *db) {

  assert(db != NULL);

  int rc = 0;

  // setup a work queue to manage our tasks
  file_queue_t *q = NULL;
  if (UNLIKELY((rc = file_queue_new(&q)))) {
    fprintf(stderr, "failed to create work queue: %s\n", strerror(rc));
    goto done;
  }

  // add our source paths to the work queue
  for (size_t i = 0; i < option.src_len; ++i) {
    rc = file_queue_push(q, option.src[i]);

    // ignore duplicate paths
    if (rc == EALREADY)
      rc = 0;

    if (UNLIKELY(rc)) {
      fprintf(stderr, "failed to add %s to work queue: %s\n", option.src[i],
              strerror(rc));
      goto done;
    }
  }

  // learn how many files we just enqueued
  size_t total_files = file_queue_size(q);

  // find the current working directory
  if (UNLIKELY(rc = cwd(&cur_dir))) {
    fprintf(stderr, "failed to get current working directory: %s\n",
            strerror(rc));
    goto done;
  }

  // select a highlighting mode, if necessary
  if (option.highlighting == BEHAVIOUR_AUTO) {
    static const size_t LARGE = 100; // a heuristic for when things get annoying
    option.highlighting = total_files >= LARGE ? LAZY : EAGER;
  }

  // suppress SIGINT, so that we do not get interrupted midway through a
  // database write and corrupt it
  if (UNLIKELY((rc = sigint_block()))) {
    fprintf(stderr, "failed to block SIGINT: %s\n", strerror(rc));
    goto done;
  }

  if (UNLIKELY((rc = progress_init(total_files)))) {
    fprintf(stderr, "failed to setup progress output: %s\n", strerror(rc));
    goto done;
  }

  // notify the user if we cannot leverage Clang fully
  if (option.parse_c == CLANG || option.parse_cxx == CLANG) {
    if (option.compile_commands.db == NULL)
      progress_warn(0, "a compile_commands.json database was not found so "
                       "C/C++ parsing will not be fully accurate");
  }

  // open a transaction to accelerate our upcoming additions
  if (UNLIKELY((rc = clink_db_begin_transaction(db))))
    progress_warn(0, "failed to start database transaction");

  if (UNLIKELY((rc = option.threads > 1 ? mt_process(db, q)
                                        : process(0, NULL, db, q))))
    goto done;

done:
  (void)clink_db_commit_transaction(db);
  progress_free();
  (void)sigint_unblock();
  free(cur_dir);
  cur_dir = NULL;
  file_queue_free(&q);

  return rc;
}
