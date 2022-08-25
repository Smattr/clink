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

    // generate a friendlier name for the source path
    char *display = NULL;
    if (UNLIKELY((rc = disppath(path, &display)))) {
      progress_error(thread_id, "failed to make %s relative: %s", path,
                     strerror(rc));
      break;
    }

    // remove anything related to the file we are about to parse
    clink_db_remove(db, path);

    // assembly
    if (is_asm(path)) {

      progress_status(thread_id, "parsing asm file %s", display);
      rc = clink_parse_asm(db, path);

      // C++ with libclang
    } else if (is_cxx(path) && option.parse_cxx == CLANG) {
      progress_status(thread_id, "Clang-parsing C++ file %s", display);

      // if we have a compile commands database, use it
      if (option.compile_commands.db != NULL) {
        rc = parse_with_comp_db(db, path);

      } else {
        assert(option.clang_argc > 0 && option.clang_argv != NULL);
        const char **argv = (const char **)option.clang_argv;
        rc = clink_parse_with_clang(db, path, option.clang_argc, argv);
      }

      // parse with the preprocessor
      if (rc == 0) {
        rc = clink_parse_cpp(db, path);
      }

      // C++ with generic parser
    } else if (is_cxx(path) && option.parse_cxx == GENERIC) {
      progress_status(thread_id, "generic-parsing C++ file %s", display);
      rc = clink_parse_cxx(db, path);

      // C with libclang
    } else if (is_c(path) && option.parse_c == CLANG) {
      progress_status(thread_id, "Clang-parsing C file %s", display);

      // if we have a compile commands database, use it
      if (option.compile_commands.db != NULL) {
        rc = parse_with_comp_db(db, path);

      } else {
        assert(option.clang_argc > 0 && option.clang_argv != NULL);
        const char **argv = (const char **)option.clang_argv;
        rc = clink_parse_with_clang(db, path, option.clang_argc, argv);
      }

      // parse with the preprocessor
      if (rc == 0) {
        rc = clink_parse_cpp(db, path);
      }

      // C with generic parser
    } else if (is_c(path) && option.parse_c == GENERIC) {
      progress_status(thread_id, "generic-parsing C file %s", display);
      rc = clink_parse_c(db, path);

      // DEF
    } else {
      assert(is_def(path));
      progress_status(thread_id, "parsing DEF file %s", display);
      rc = clink_parse_def(db, path);
    }

    if (UNLIKELY(rc))
      progress_error(thread_id, "failed to parse %s: %s", display,
                     strerror(rc));

    if (LIKELY(rc == 0)) {

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
        }
      }

      // now we can insert a record for the file
      if (rc == 0)
        (void)clink_db_add_record(db, path, hash, timestamp);
    }

    free(display);

    if (UNLIKELY(rc))
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

  if (UNLIKELY((rc = option.threads > 1 ? mt_process(db, q)
                                        : process(0, NULL, db, q))))
    goto done;

done:
  progress_free();
  (void)sigint_unblock();
  file_queue_free(&q);

  return rc;
}
