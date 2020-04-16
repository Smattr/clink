#include <assert.h>
#include <clink/c.h>
#include <errno.h>
#include "get_environ.h"
#include <fcntl.h>
#include <spawn.h>
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
  for (;;) {

    if (getline(&line, &line_size, in) < 0) {
      rc = errno;
      if (rc == 0)
        rc = ENOTSUP;
      goto done;
    }

    if (strcmp(line, "#include \"...\" search starts here:\n") == 0)
      break;

  }

  // read includes
  for (;;) {

    if (getline(&line, &line_size, in) < 0) {
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
    if (line[0] != ' ') {
      rc = ENOTSUP;
      goto done;
    }

    const char *start = line + 1;
    size_t extent = strlen(start);

    // and we expect it to end with a newline
    if (extent == 0 || start[extent - 1] != '\n') {
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
    if (new_l == NULL) {
      rc = ENOMEM;
      goto done;
    }
    l = new_l;
    ++l_len;
    l[l_len - 1] = strndup(start, extent);
    if (l[l_len - 1] == NULL) {
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
  const char *argv[] = { cxx, "-E", "-x", "c++", "-", "-v", NULL };

  int rc = 0;
  posix_spawn_file_actions_t fa;
  int devnull = -1;
  int channel[2] = { -1, -1 };
  FILE *child_err = NULL;


  if ((rc = posix_spawn_file_actions_init(&fa)))
    return rc;

  // wire the child’s stdin and stdout to /dev/null
  if ((devnull = open("/dev/null", O_RDWR)) < 0) {
    rc = errno;
    goto done;
  }
  if ((rc = posix_spawn_file_actions_adddup2(&fa, devnull, STDIN_FILENO)))
    goto done;
  if ((rc = posix_spawn_file_actions_adddup2(&fa, devnull, STDOUT_FILENO)))
    goto done;

  // create a pipe for communicating with the compiler
  if (pipe(channel)) {
    rc = errno;
    goto done;
  }

  // open the read end as a stream so we can later use getline on it
  if ((child_err = fdopen(channel[0], "r")) == NULL) {
    rc = errno;
    goto done;
  }

  // wire the child’s stderr to the write end of the pipe
  if ((rc = posix_spawn_file_actions_adddup2(&fa, channel[1], STDERR_FILENO)))
    goto done;

  {
    // start the child
    pid_t pid;
    char *const *args = (char*const*)argv;
    if ((rc = posix_spawnp(&pid, argv[0], &fa, NULL, args, get_environ())))
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
  if (devnull != -1)
    close(devnull);
  (void)posix_spawn_file_actions_destroy(&fa);

  return rc;
}
