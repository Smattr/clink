#include "../common/pipe.h"
#include "../libclink/src/run.h"
#include "test.h"
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

TEST("running `echo hello world` via libclink’s run() function does what we "
     "expect") {

  // create a pipe for the child’s stdout
  int fd[2];
  {
    int r = pipe_(fd);
    ASSERT_EQ(r, 0);
  }

  // create a child whose stdout we can dup over
  pid_t p = fork();
  ASSERT_GE(p, 0);

  if (p == 0) { // the child

    // replace our stdout with the write end of the pipe
    {
      int r = dup2(fd[1], STDOUT_FILENO);
      ASSERT_GE(r, 0);
    }

    // use run() to echo some text
    {
      const char *args[] = {"echo", "hello", "world", NULL};
      int r = run(args);
      ASSERT_EQ(r, EXIT_SUCCESS);
    }

    exit(EXIT_SUCCESS);
  }

  // close the end of the pipe we do not need
  (void)close(fd[1]);

  // wait for the child to exit
  {
    int status;
    pid_t child = wait(&status);
    ASSERT_EQ(child, p);
    ASSERT(WIFEXITED(status));
    ASSERT_EQ(WEXITSTATUS(status), EXIT_SUCCESS);
  }

  // read out of the pipe
  {
    char buffer[128] = {0};
    ssize_t r = read(fd[0], buffer, sizeof(buffer));
    ASSERT_EQ((size_t)r, strlen("hello world\n"));
    ASSERT_STREQ(buffer, "hello world\n");
  }

  (void)close(fd[0]);
}
