// a test that running `echo hello world` via libclink’s run() function with
// input masked generates no input

// force assertions on
#ifdef NDEBUG
  #undef NDEBUG
#endif

#include <assert.h>
#include "run.h"
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {

  // create a pipe for the child’s stdout
  int fd[2];
  {
    int r = pipe(fd);
    assert(r == 0);
  }

  // create a child whose stdout we can dup over
  pid_t p = fork();
  assert(p >= 0);

  if (p == 0) { // the child

    // replace our stdout with the write end of the pipe
    {
      int r = dup2(fd[1], STDOUT_FILENO);
      assert(r >= 0);
    }

    // use run() to echo some text
    {
      const char *args[] = { "echo", "hello", "world", NULL };
      int r = run(args, true);
      assert(r == EXIT_SUCCESS);
    }

    exit(EXIT_SUCCESS);

  }

  // close the end of the pipe we do not need
  (void)close(fd[1]);

  // wait for the child to exit
  {
    int status;
    pid_t child = wait(&status);
    assert(child == p);
    assert(WIFEXITED(status));
    assert(WEXITSTATUS(status) == EXIT_SUCCESS);
  }

  // read out of the pipe
  {
    char buffer[128] = { 0 };
    ssize_t r = read(fd[0], buffer, sizeof(buffer));
    assert(r == 0);
    assert(strcmp(buffer, "") == 0);
  }

  return 0;
}
