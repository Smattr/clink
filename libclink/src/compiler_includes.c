#include "../../common/compiler.h"
#include "debug.h"
#include "get_environ.h"
#include "posix_spawn.h"
#include <assert.h>
#include <clink/c.h>
#include <errno.h>
#include <fcntl.h>
#include <spawn.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/// parse the list of includes out of the given stderr output from the compiler
static int parse_includes(FILE *in, char ***list, size_t *list_len) {

  assert(in != NULL);
  assert(list != NULL);
  assert(list_len != NULL);

  char **l = NULL;
  size_t l_len = 0;

  int rc = 0;
  char *line = NULL;
  size_t line_size = 0;

  // read lines looking for the start of relative includes
  errno = 0;
  while (true) {

    if (ERROR(getline(&line, &line_size, in) < 0)) {
      rc = errno;
      if (rc == 0)
        rc = ENOTSUP;
      goto done;
    }

    if (strcmp(line, "#include \"...\" search starts here:\n") == 0)
      break;
  }

  // read includes
  while (true) {

    if (ERROR(getline(&line, &line_size, in) < 0)) {
      rc = errno;
      if (rc == 0)
        rc = ENOTSUP;
      goto done;
    }

    if (strcmp(line, "#include <...> search starts here:\n") == 0) {
      // into the system includes; keep reading further
      continue;
    }

    if (strcmp(line, "End of search list.\n") == 0)
      break;

    // we expect all #include lines to begin with a space
    if (ERROR(line[0] != ' ')) {
      rc = ENOTSUP;
      goto done;
    }

    const char *start = line + 1;
    size_t extent = strlen(start);

    // and we expect it to end with a newline
    if (ERROR(extent == 0 || start[extent - 1] != '\n')) {
      rc = ENOTSUP;
      goto done;
    }
    --extent;

    // macOS annotates some paths
    {
      static const char annotation[] = " (framework directory)";
      size_t len = strlen(annotation);
      if (extent >= len && strncmp(start + extent - len, annotation, len) == 0)
        extent -= len;
    }

    // record this include
    char **new_l = realloc(l, (l_len + 1) * sizeof(l[0]));
    if (ERROR(new_l == NULL)) {
      rc = ENOMEM;
      goto done;
    }
    l = new_l;
    ++l_len;
    l[l_len - 1] = strndup(start, extent);
    if (ERROR(l[l_len - 1] == NULL)) {
      rc = ENOMEM;
      goto done;
    }
  }

done:
  free(line);

  if (rc) {
    for (size_t i = 0; i < l_len; ++i)
      free(l[i]);
    free(l);
  } else {
    *list = l;
    *list_len = l_len;
  }

  return rc;
}

/// `pipe` that also sets close-on-exec
static int pipe_(int pipefd[2]) {
  assert(pipefd != NULL);

#ifdef __APPLE__
  // macOS does not have `pipe2`, so we need to fall back on `pipe`+`fcntl`.
  // This is racy, but there does not seem to be a way to avoid this.

  // create the pipe
  if (ERROR(pipe(pipefd) < 0))
    return errno;

  // set close-on-exec
  for (size_t i = 0; i < 2; ++i) {
    const int flags = fcntl(pipefd[i], F_GETFD);
    if (ERROR(fcntl(pipefd[i], F_SETFD, flags | FD_CLOEXEC) < 0)) {
      const int err = errno;
      for (size_t j = 0; j < 2; ++j) {
        (void)close(pipefd[j]);
        pipefd[j] = -1;
      }
      return err;
    }
  }

#else
  if (ERROR(pipe2(pipefd, O_CLOEXEC) < 0))
    return errno;
#endif

  return 0;
}

int clink_compiler_includes(const char *compiler, char ***includes,
                            size_t *includes_len) {

  if (includes == NULL)
    return EINVAL;

  if (includes_len == NULL)
    return EINVAL;

  // first preference, select the compiler the caller gave us
  const char *cxx = compiler;

  // if there was no provided compiler, try $CXX
  if (cxx == NULL)
    cxx = getenv("CXX");

  // if there was no $CXX, fall back to c++
  if (cxx == NULL)
    cxx = "c++";

  // construct a command to ask the compiler what its #include paths are
  const char *argv[] = {cxx, "-E", "-x", "c++", "-", "-v", NULL};

  int rc = 0;
  posix_spawn_file_actions_t fa;
  int channel[2] = {-1, -1};
  FILE *child_err = NULL;

  if (ERROR((rc = fa_init(&fa))))
    return rc;

  // wire the child’s stdin and stdout to /dev/null
  if (ERROR((rc = addopen(&fa, STDIN_FILENO, "/dev/null", O_RDONLY))))
    goto done;
  if (ERROR((rc = addopen(&fa, STDOUT_FILENO, "/dev/null", O_WRONLY))))
    goto done;

  // create a pipe for communicating with the compiler
  if (ERROR((rc = pipe_(channel))))
    goto done;

  // open the read end as a stream so we can later use getline on it
  child_err = fdopen(channel[0], "r");
  if (ERROR(child_err == NULL)) {
    rc = errno;
    goto done;
  }

  // wire the child’s stderr to the write end of the pipe
  if (ERROR((rc = adddup2(&fa, channel[1], STDERR_FILENO))))
    goto done;

  {
    // start the child
    pid_t pid;
    if (ERROR((rc = spawn(&pid, argv, &fa))))
      goto done;

    // extract the list of #include paths
    char **inc = NULL;
    size_t inc_len = 0;
    rc = parse_includes(child_err, &inc, &inc_len);

    // close our end of the child’s stderr to force a SIGPIPE and exit the child
    // if parse_includes() failed
    fclose(child_err);
    child_err = NULL;

    // clean up the child
    int ignored;
    (void)waitpid(pid, &ignored, 0);

    // if we were successful, pass back what we discovered
    if (rc == 0) {
      *includes = inc;
      *includes_len = inc_len;
    }
  }

done:
  if (child_err)
    fclose(child_err);
  if (channel[1] != -1)
    close(channel[1]);
  if (channel[0] != -1)
    close(channel[0]);
  fa_destroy(&fa);

  return rc;
}
